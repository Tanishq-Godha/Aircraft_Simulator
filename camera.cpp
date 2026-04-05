#include "camera.h"
#include "globals.h"
#include "math_utils.h"
#include "terrain.h"
#include <GL/glut.h>
#include <cmath>

namespace {

struct Vec3 {
    float x;
    float y;
    float z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float px, float py, float pz) : x(px), y(py), z(pz) {}
};

struct CameraState {
    bool initialized;
    int lastTickMs;
    int lastMode;
    Vec3 position;
    Vec3 velocity;
    Vec3 lookAt;
    Vec3 up;
    float fov;

    CameraState()
        : initialized(false),
          lastTickMs(0),
          lastMode(-1),
          position(),
          velocity(),
          lookAt(),
          up(0.0f, 1.0f, 0.0f),
          fov(80.0f) {}
};

struct CameraTarget {
    Vec3 position;
    Vec3 lookAt;
    Vec3 up;
    float fov;
    float stiffness;
    float damping;
    float lookRate;
    float upRate;
    float fovRate;
    float collisionRadius;
    float clearance;
};

struct AircraftBasis {
    Vec3 forward;
    Vec3 up;
    Vec3 right;
    Vec3 bankUp;
    Vec3 bankRight;
};

Vec3 operator+(const Vec3& a, const Vec3& b) {
    return Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

Vec3 operator*(const Vec3& v, float scalar) {
    return Vec3(v.x * scalar, v.y * scalar, v.z * scalar);
}


Vec3 operator*(float scalar, const Vec3& v) {
    return v * scalar;
}

Vec3 operator/(const Vec3& v, float scalar) {
    return Vec3(v.x / scalar, v.y / scalar, v.z / scalar);
}

float approach(float current, float target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

Vec3 approachVec(const Vec3& current, const Vec3& target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

Vec3 mixVec(const Vec3& a, const Vec3& b, float t) {
    return a + (b - a) * t;
}

float dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
    return Vec3(a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
}

float lengthSq(const Vec3& v) {
    return dot(v, v);
}

float length(const Vec3& v) {
    return std::sqrt(lengthSq(v));
}

Vec3 normalizeOr(const Vec3& v, const Vec3& fallback) {
    float len = length(v);
    if (len < 0.0001f) {
        return fallback;
    }
    return v / len;
}

Vec3 rotateLocalVector(const Vec3& local, float yawDeg, float pitchDeg, float rollDeg) {
    float yRad = degToRad(yawDeg);
    float pRad = degToRad(pitchDeg);
    float rRad = degToRad(rollDeg);

    float cr = std::cos(rRad);
    float sr = std::sin(rRad);
    float cp = std::cos(pRad);
    float sp = std::sin(pRad);
    float cy = std::cos(yRad);
    float sy = std::sin(yRad);

    Vec3 rolled(local.x * cr - local.y * sr,
                local.x * sr + local.y * cr,
                local.z);

    Vec3 pitched(rolled.x,
                 rolled.y * cp - rolled.z * sp,
                 rolled.y * sp + rolled.z * cp);

    return Vec3(pitched.x * cy - pitched.z * sy,
                pitched.y,
                pitched.x * sy + pitched.z * cy);
}

AircraftBasis getAircraftBasis() {
    AircraftBasis basis;

    basis.forward = normalizeOr(rotateLocalVector(Vec3(0.0f, 0.0f, -1.0f), yaw, pitch, 0.0f),
                                Vec3(0.0f, 0.0f, -1.0f));
    basis.up = normalizeOr(rotateLocalVector(Vec3(0.0f, 1.0f, 0.0f), yaw, pitch, 0.0f),
                           Vec3(0.0f, 1.0f, 0.0f));
    basis.right = normalizeOr(cross(basis.forward, basis.up), Vec3(1.0f, 0.0f, 0.0f));
    basis.up = normalizeOr(cross(basis.right, basis.forward), basis.up);

    basis.bankUp = normalizeOr(rotateLocalVector(Vec3(0.0f, 1.0f, 0.0f), yaw, pitch, roll), basis.up);
    basis.bankRight = normalizeOr(cross(basis.forward, basis.bankUp), basis.right);
    basis.bankUp = normalizeOr(cross(basis.bankRight, basis.forward), basis.bankUp);

    return basis;
}

float getInflatedSceneHeight(float x, float z, float radius) {
    float diag = radius * 0.70710678f;
    float maxHeight = getSceneHeight(x, z);

    float sampleXs[8] = {
        x + radius, x - radius, x, x,
        x + diag, x + diag, x - diag, x - diag
    };
    float sampleZs[8] = {
        z, z, z + radius, z - radius,
        z + diag, z - diag, z + diag, z - diag
    };

    for (int i = 0; i < 8; ++i) {
        float sampleHeight = getSceneHeight(sampleXs[i], sampleZs[i]);
        if (sampleHeight > maxHeight) {
            maxHeight = sampleHeight;
        }
    }

    return maxHeight;
}

Vec3 resolveCameraCollision(const Vec3& anchor,
                            const Vec3& desired,
                            float collisionRadius,
                            float clearance) {
    Vec3 delta = desired - anchor;
    float travel = length(delta);
    if (travel < 0.001f) {
        return desired;
    }

    const int steps = 28;
    float safeT = 1.0f;

    for (int i = 1; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        Vec3 sample = anchor + delta * t;
        float minHeight = getInflatedSceneHeight(sample.x, sample.z, collisionRadius) + clearance;
        if (sample.y < minHeight) {
            safeT = static_cast<float>(i - 1) / static_cast<float>(steps);
            break;
        }
    }

    Vec3 resolved = anchor + delta * safeT;
    float resolvedFloor = getInflatedSceneHeight(resolved.x, resolved.z, collisionRadius) + clearance;
    if (resolved.y < resolvedFloor) {
        resolved.y = resolvedFloor;
    }

    return resolved;
}

CameraTarget buildCameraTarget(int cameraMode, int nowMs, float speedRatio) {
    CameraTarget target;

    Vec3 planePos(planeX, planeY, planeZ);
    Vec3 planeVelocity(vX, vY, vZ);
    AircraftBasis basis = getAircraftBasis();

    Vec3 velocityDir = normalizeOr(planeVelocity, basis.forward);
    float velocityBlend = clampf(currentSpeed / 520.0f, 0.0f, 0.7f);
    Vec3 leadDir = normalizeOr(mixVec(basis.forward, velocityDir, velocityBlend), basis.forward);
    Vec3 focusPoint = planePos + basis.up * 2.8f;

    if (cameraMode == 0) {
        // Yaw is stored in degrees in this project, so convert before using sin/cos.
        const float offsetDistance = 12.0f;
        const float offsetHeight = 4.0f;
        const float pitchHeightResponse = 2.0f;
        const float minGroundClearance = 1.0f;

        float yawRad = degToRad(yaw);
        float pitchLift = degToRad(pitch) * pitchHeightResponse;

        float targetCamX = planeX - offsetDistance * std::sin(yawRad);
        float targetCamZ = planeZ + offsetDistance * std::cos(yawRad);
        float targetCamY = planeY + offsetHeight + pitchLift;

        float groundLimit = getInflatedSceneHeight(targetCamX, targetCamZ, 2.0f) + minGroundClearance;
        if (groundLimit < 1.0f) {
            groundLimit = 1.0f;
        }
        if (targetCamY < groundLimit) {
            targetCamY = groundLimit;
        }

        target.position = Vec3(targetCamX, targetCamY, targetCamZ);
        target.lookAt = planePos;
        target.up = Vec3(0.0f, 1.0f, 0.0f);
        target.fov = 72.0f;
        target.stiffness = 0.0f;
        target.damping = 0.0f;
        target.lookRate = 0.0f;
        target.upRate = 0.0f;
        target.fovRate = 0.0f;
        target.collisionRadius = 2.0f;
        target.clearance = minGroundClearance;
    } else if (cameraMode == 1) {
        target.up = normalizeOr(mixVec(basis.up, basis.bankUp, 0.55f), basis.up);
        target.position = planePos + basis.forward * 3.4f + target.up * 1.25f;
        target.lookAt = target.position + leadDir * 320.0f + target.up * 10.0f;
        target.fov = 76.0f + speedRatio * 4.0f;
        target.stiffness = 22.0f;
        target.damping = 2.0f * std::sqrt(target.stiffness);
        target.lookRate = 12.0f;
        target.upRate = 10.0f;
        target.fovRate = 7.0f;
        target.collisionRadius = 10.0f;
        target.clearance = 8.0f;
    } else if (cameraMode == 2) {
        target.up = normalizeOr(mixVec(basis.up, basis.bankUp, 0.20f), basis.up);
        target.position = focusPoint
                        - leadDir * (78.0f + speedRatio * 38.0f)
                        + basis.up * (24.0f + speedRatio * 7.0f)
                        + basis.right * 12.0f;
        target.position = resolveCameraCollision(focusPoint, target.position, 28.0f, 12.0f);
        target.lookAt = focusPoint + leadDir * (120.0f + speedRatio * 140.0f) + basis.up * 4.0f;
        target.fov = 82.0f + speedRatio * 10.0f;
        target.stiffness = 7.0f;
        target.damping = 2.0f * std::sqrt(target.stiffness);
        target.lookRate = 6.0f;
        target.upRate = 6.0f;
        target.fovRate = 4.0f;
        target.collisionRadius = 28.0f;
        target.clearance = 12.0f;
    } else {
        target.up = normalizeOr(mixVec(basis.up, basis.bankUp, 0.15f), basis.up);
        target.position = focusPoint
                        + basis.right * 42.0f
                        + basis.up * 10.0f
                        - leadDir * 10.0f;
        target.position = resolveCameraCollision(focusPoint, target.position, 24.0f, 10.0f);
        target.lookAt = focusPoint + leadDir * (55.0f + speedRatio * 60.0f);
        target.fov = 78.0f + speedRatio * 8.0f;
        target.stiffness = 10.5f;
        target.damping = 2.0f * std::sqrt(target.stiffness);
        target.lookRate = 7.5f;
        target.upRate = 6.5f;
        target.fovRate = 4.5f;
        target.collisionRadius = 24.0f;
        target.clearance = 10.0f;
    }

    if (cameraMode == 3) {
        float orbit = 0.5f + 0.5f * std::sin(nowMs * 0.00035f);
        target.position = target.position + basis.forward * (orbit * 18.0f - 9.0f);
    }

    return target;
}

} // namespace

void setupCamera() {
    static CameraState state;

    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = 0.016f;
    if (state.lastTickMs != 0) {
        dt = (nowMs - state.lastTickMs) * 0.001f;
        dt = clampf(dt, 0.001f, 0.05f);
    }
    state.lastTickMs = nowMs;

    float speedRatio = clampf(currentSpeed / 920.0f, 0.0f, 1.15f);
    CameraTarget target = buildCameraTarget(cameraMode, nowMs, speedRatio);
    bool modeChanged = state.lastMode != cameraMode;

    bool shouldSnap = !state.initialized ||
                      length(state.position - target.position) > 1800.0f ||
                      length(state.lookAt - target.lookAt) > 2500.0f;

    if (shouldSnap) {
        state.position = target.position;
        state.velocity = Vec3();
        state.lookAt = target.lookAt;
        state.up = target.up;
        state.fov = target.fov;
        state.initialized = true;
    } else {
        if (cameraMode == 0) {
            const float followAlpha = 0.10f;

            state.velocity = Vec3();
            state.position = mixVec(state.position, target.position, followAlpha);
            state.lookAt = target.lookAt;
            state.up = target.up;
            state.fov += (target.fov - state.fov) * followAlpha;

            float groundLimit = getInflatedSceneHeight(state.position.x,
                                                       state.position.z,
                                                       target.collisionRadius) + target.clearance;
            if (groundLimit < 1.0f) {
                groundLimit = 1.0f;
            }
            if (state.position.y < groundLimit) {
                state.position.y = groundLimit;
            }

            Vec3 planePos(planeX, planeY, planeZ);
            Vec3 cameraToPlane = state.position - planePos;
            float currentDistance = length(cameraToPlane);
            if (currentDistance < 4.0f) {
                state.position = planePos + normalizeOr(cameraToPlane, Vec3(0.0f, 1.0f, 1.0f)) * 4.0f;
            }
        } else {
            if (modeChanged) {
                state.velocity = state.velocity * 0.25f;
            }

            Vec3 accel = (target.position - state.position) * target.stiffness
                       - state.velocity * target.damping;
            state.velocity = state.velocity + accel * dt;
            state.position = state.position + state.velocity * dt;

            float minHeight = getInflatedSceneHeight(state.position.x,
                                                     state.position.z,
                                                     target.collisionRadius) + target.clearance;
            if (state.position.y < minHeight) {
                state.position.y = minHeight;
                if (state.velocity.y < 0.0f) {
                    state.velocity.y = 0.0f;
                }
            }

            state.lookAt = approachVec(state.lookAt, target.lookAt, target.lookRate, dt);
            state.up = normalizeOr(approachVec(state.up, target.up, target.upRate, dt), target.up);
            state.fov = approach(state.fov, target.fov, target.fovRate, dt);
        }
    }

    Vec3 viewDir = normalizeOr(state.lookAt - state.position, Vec3(0.0f, 0.0f, -1.0f));
    if (std::fabs(dot(viewDir, state.up)) > 0.985f) {
        state.up = target.up;
    }

    state.lastMode = cameraMode;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(state.fov,
                   float(screenW) / float(screenH),
                   1.0f,
                   30000.0f);

    glMatrixMode(GL_MODELVIEW);
    gluLookAt(state.position.x, state.position.y, state.position.z,
              state.lookAt.x, state.lookAt.y, state.lookAt.z,
              state.up.x, state.up.y, state.up.z);
}
