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

    basis.bankUp = normalizeOr(rotateLocalVector(Vec3(0.0f, 1.0f, 0.0f), yaw, pitch, -roll), basis.up);
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
        const float baseDistance = 25.0f;
        const float baseHeight = 12.0f;
        const float pitchHeightResponse = 2.5f;
        const float minGroundClearance = 1.0f;

        // Dynamic distance based on speed (pulls back at high speed)
        float offsetDistance = baseDistance + (speedRatio * 12.0f);
        float offsetHeight = baseHeight + (speedRatio * 4.0f);

        float yawRad = degToRad(yaw);
        float pitchLift = degToRad(pitch) * pitchHeightResponse;

        float targetCamX = planeX - offsetDistance * std::sin(yawRad);
        float targetCamZ = planeZ + offsetDistance * std::cos(yawRad);
        float targetCamY = planeY + offsetHeight + pitchLift;

        // Dynamic Shake effect at high speeds (starts above 85% top speed)
        if (speedRatio > 0.85f) {
            float shakeTime = nowMs * 0.045f;
            float shakeMag = (speedRatio - 0.85f) * 0.35f;
            targetCamX += std::sin(shakeTime) * shakeMag;
            targetCamY += std::cos(shakeTime * 1.1f) * shakeMag;
            targetCamZ += std::sin(shakeTime * 0.8f) * shakeMag;
        }

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
        target.fov = 72.0f + (speedRatio * 16.0f); // Lens expansion effect
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

CameraState g_cameraState;
} // namespace

void updateCamera(float dt) {
    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float speedRatio = clampf(currentSpeed / 920.0f, 0.0f, 1.15f);
    CameraTarget target = buildCameraTarget(cameraMode, nowMs, speedRatio);
    bool modeChanged = g_cameraState.lastMode != cameraMode;

    bool shouldSnap = !g_cameraState.initialized ||
                      length(g_cameraState.position - target.position) > 1800.0f ||
                      length(g_cameraState.lookAt - target.lookAt) > 2500.0f;

    if (shouldSnap) {
        g_cameraState.position = target.position;
        g_cameraState.velocity = Vec3();
        g_cameraState.lookAt = target.lookAt;
        g_cameraState.up = target.up;
        g_cameraState.fov = target.fov;
        g_cameraState.initialized = true;
    } else {
        if (cameraMode == 0) {
            const float followRate = 12.0f;
            Vec3 planeVelocity(vX, vY, vZ);
            
            // Step camera with plane
            g_cameraState.position = g_cameraState.position + planeVelocity * dt;
            g_cameraState.velocity = Vec3();
            g_cameraState.position = approachVec(g_cameraState.position, target.position, followRate, dt);
            g_cameraState.lookAt = target.lookAt;
            g_cameraState.up = target.up;
            g_cameraState.fov = approach(g_cameraState.fov, target.fov, 4.0f, dt);

            float groundLimit = getInflatedSceneHeight(g_cameraState.position.x, 
                                                       g_cameraState.position.z, 
                                                       target.collisionRadius) + target.clearance;
            if (g_cameraState.position.y < groundLimit) g_cameraState.position.y = groundLimit;

            Vec3 planePos(planeX, planeY, planeZ);
            Vec3 cameraToPlane = g_cameraState.position - planePos;
            if (length(cameraToPlane) < 4.0f) {
                g_cameraState.position = planePos + normalizeOr(cameraToPlane, Vec3(0.0f, 1.0f, 1.0f)) * 4.0f;
            }
        } else {
            if (modeChanged) g_cameraState.velocity = g_cameraState.velocity * 0.25f;
            Vec3 accel = (target.position - g_cameraState.position) * target.stiffness
                       - g_cameraState.velocity * target.damping;
            g_cameraState.velocity = g_cameraState.velocity + accel * dt;
            g_cameraState.position = g_cameraState.position + g_cameraState.velocity * dt;

            // Ground collision
            float minHeight = getInflatedSceneHeight(g_cameraState.position.x,
                                                     g_cameraState.position.z,
                                                     target.collisionRadius) + target.clearance;
            if (g_cameraState.position.y < minHeight) {
                g_cameraState.position.y = minHeight;
                if (g_cameraState.velocity.y < 0.0f) g_cameraState.velocity.y = 0.0f;
            }

            g_cameraState.lookAt = approachVec(g_cameraState.lookAt, target.lookAt, target.lookRate, dt);
            g_cameraState.up = normalizeOr(approachVec(g_cameraState.up, target.up, target.upRate, dt), target.up);
            g_cameraState.fov = approach(g_cameraState.fov, target.fov, target.fovRate, dt);
        }
    }
    
    Vec3 viewDir = normalizeOr(g_cameraState.lookAt - g_cameraState.position, Vec3(0.0f, 0.0f, -1.0f));
    if (std::fabs(dot(viewDir, g_cameraState.up)) > 0.985f) {
        g_cameraState.up = target.up;
    }

    // Decay trauma over time
    if (cameraTrauma > 0.0f) {
        cameraTrauma -= 0.8f * dt;
        if (cameraTrauma < 0.0f) cameraTrauma = 0.0f;
    }

    g_cameraState.lastMode = cameraMode;
}

void setupCamera() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(g_cameraState.fov,
                   float(screenW) / float(screenH),
                   1.0f,
                   30000.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Camera Trauma (Shake Effect)
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    float shakeZ = 0.0f;
    
    if (cameraTrauma > 0.0f) {
        float shake = cameraTrauma * cameraTrauma;
        int t = glutGet(GLUT_ELAPSED_TIME);
        // Simple fast pseudo-random shake based on time
        shakeX = (std::sin(t * 0.05f) * 2.0f + std::sin(t * 0.02f)) * 10.0f * shake;
        shakeY = (std::cos(t * 0.04f) * 2.0f + std::sin(t * 0.03f)) * 10.0f * shake;
        shakeZ = (std::sin(t * 0.06f) * 2.0f + std::cos(t * 0.01f)) * 10.0f * shake;
        
        // Decay trauma in updateCamera usually, but doing it safely here or in physics is fine
    }

    gluLookAt(g_cameraState.position.x + shakeX, g_cameraState.position.y + shakeY, g_cameraState.position.z + shakeZ,
              g_cameraState.lookAt.x + shakeX * 0.5f, g_cameraState.lookAt.y + shakeY * 0.5f, g_cameraState.lookAt.z + shakeZ * 0.5f,
              g_cameraState.up.x, g_cameraState.up.y, g_cameraState.up.z);
}
