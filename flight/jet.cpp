#include "jet.h"
#include "globals.h"
#include "atmosphere.h"
#include "terrain.h"
#include "model_loader.h"
#include "shader_loader.h"
#include <GL/glut.h>
#include <GL/glu.h>
#include <math.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern GLUquadric* quadric;

// Global loaded model
LoadedModel g_loadedJetModel = {};
bool g_useLoadedModel = false;

namespace {

float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

float approach(float current, float target, float rate, float dt) {
    float alpha = clampf(rate * dt, 0.0f, 1.0f);
    return current + (target - current) * alpha;
}

struct ControlSurfaceState {
    bool initialized;
    int lastTickMs;
    float flapAngle;
    float aileronAngle;
    float elevatorAngle;
    float rudderAngle;
};

ControlSurfaceState& getControlSurfaceState() {
    static ControlSurfaceState state = { false, 0, 0.0f, 0.0f, 0.0f, 0.0f };
    return state;
}

void setMaterial(const GLfloat* mat) {
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat);
}

void drawHingedSurface(const GLfloat* mat,
                       float r, float g, float b,
                       float hingeX, float hingeY, float hingeZ,
                       float angleX, float angleY,
                       float offsetX, float offsetY, float offsetZ,
                       float sizeX, float sizeY, float sizeZ) {
    setMaterial(mat);
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(hingeX, hingeY, hingeZ);
    if (angleY != 0.0f) {
        glRotatef(angleY, 0.0f, 1.0f, 0.0f);
    }
    if (angleX != 0.0f) {
        glRotatef(angleX, 1.0f, 0.0f, 0.0f);
    }
    glTranslatef(offsetX, offsetY, offsetZ);
    glScalef(sizeX, sizeY, sizeZ);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void updateControlSurfaceAnimation(float& flapAngle,
                                   float& aileronAngle,
                                   float& elevatorAngle,
                                   float& rudderAngle) {
    ControlSurfaceState& state = getControlSurfaceState();

    int nowMs = glutGet(GLUT_ELAPSED_TIME);
    float dt = 0.016f;
    if (state.lastTickMs != 0) {
        dt = (nowMs - state.lastTickMs) * 0.001f;
        dt = clampf(dt, 0.001f, 0.05f);
    }
    state.lastTickMs = nowMs;

    float targetFlap = 0.0f;
    float targetAileron = clampf((-roll * 0.32f) +
                                 (keys['a'] ? 8.0f : 0.0f) -
                                 (keys['d'] ? 8.0f : 0.0f),
                                 -18.0f, 18.0f);
    float targetElevator = clampf((-pitch * 0.35f) +
                                  (keys['w'] ? -9.0f : 0.0f) +
                                  (keys['s'] ? 9.0f : 0.0f),
                                  -16.0f, 16.0f);
    float targetRudder = clampf((keys['q'] ? 14.0f : 0.0f) -
                                (keys['e'] ? 14.0f : 0.0f) +
                                (roll / 45.0f) * 4.0f,
                                -18.0f, 18.0f);

    if (!state.initialized) {
        state.flapAngle = targetFlap;
        state.aileronAngle = targetAileron;
        state.elevatorAngle = targetElevator;
        state.rudderAngle = targetRudder;
        state.initialized = true;
    } else {
        state.flapAngle = approach(state.flapAngle, targetFlap, 5.0f, dt);
        state.aileronAngle = approach(state.aileronAngle, targetAileron, 9.0f, dt);
        state.elevatorAngle = approach(state.elevatorAngle, targetElevator, 8.0f, dt);
        state.rudderAngle = approach(state.rudderAngle, targetRudder, 8.0f, dt);
    }

    flapAngle = state.flapAngle;
    aileronAngle = state.aileronAngle;
    elevatorAngle = state.elevatorAngle;
    rudderAngle = state.rudderAngle;
}

void drawDoorLeaf(float side, float width, float length, float openAngle) {
    glPushMatrix();
    glRotatef(side * openAngle, 0.0f, 0.0f, 1.0f);
    glTranslatef(side * width * 0.5f, 0.0f, 0.0f);
    glScalef(width, 0.035f, length);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawWheel(float tireRadius, float wheelRadius, float hubRadius) {
    GLfloat mat_tire[] = { 0.08f, 0.08f, 0.09f, 1.0f };
    GLfloat mat_hub[] = { 0.72f, 0.72f, 0.76f, 1.0f };

    glPushMatrix();
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glRotatef(wheelRotation, 0.0f, 0.0f, 1.0f);

    glColor3f(0.08f, 0.08f, 0.09f);
    setMaterial(mat_tire);
    glutSolidTorus(tireRadius, wheelRadius, 12, 24);

    glColor3f(0.72f, 0.72f, 0.76f);
    setMaterial(mat_hub);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.03f);
    gluDisk(quadric, 0.0f, hubRadius, 16, 1);
    glTranslatef(0.0f, 0.0f, 0.06f);
    gluDisk(quadric, 0.0f, hubRadius, 16, 1);
    glPopMatrix();

    glPopMatrix();
}

void drawEngineFan(float radius) {
    GLfloat mat_dark[] = { 0.10f, 0.10f, 0.12f, 1.0f };
    GLfloat mat_blade[] = { 0.74f, 0.74f, 0.78f, 1.0f };
    GLfloat mat_spinner[] = { 0.92f, 0.92f, 0.92f, 1.0f };

    glColor3f(0.10f, 0.10f, 0.12f);
    setMaterial(mat_dark);
    gluDisk(quadric, 0.0f, radius, 28, 1);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.01f);
    glRotatef(engineFanRotation, 0.0f, 0.0f, 1.0f);
    glColor3f(0.74f, 0.74f, 0.78f);
    setMaterial(mat_blade);

    for (int i = 0; i < 12; ++i) {
        glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
        glBegin(GL_TRIANGLES);
        glNormal3f(0.0f, 0.0f, -1.0f);
        glVertex3f(radius * 0.12f, 0.0f, 0.02f);
        glVertex3f(radius * 0.92f, radius * 0.11f, 0.02f);
        glVertex3f(radius * 0.48f, radius * 0.02f, 0.02f);
        glEnd();
    }
    glPopMatrix();

    glColor3f(0.92f, 0.92f, 0.92f);
    setMaterial(mat_spinner);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.10f);
    glutSolidSphere(radius * 0.18f, 14, 14);
    glPopMatrix();
}

void drawGearBayCavity(float x, float y, float z, float sx, float sy, float sz) {
    GLfloat mat_bay[] = { 0.07f, 0.07f, 0.08f, 1.0f };
    glColor3f(0.07f, 0.07f, 0.08f);
    setMaterial(mat_bay);
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(sx, sy, sz);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawGearDoorPanels(float doorAngle) {
    GLfloat mat_door[] = { 0.85f, 0.10f, 0.15f, 1.0f };
    setMaterial(mat_door);
    glColor3f(0.85f, 0.10f, 0.15f);

    glPushMatrix();
    glTranslatef(0.0f, -0.505f, -2.92f);
    drawDoorLeaf(-1.0f, 0.18f, 1.15f, doorAngle);
    drawDoorLeaf(1.0f, 0.18f, 1.15f, doorAngle);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(1.15f, -0.305f, 1.20f);
    drawDoorLeaf(1.0f, 0.24f, 1.65f, doorAngle * 0.85f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.15f, -0.305f, 1.20f);
    drawDoorLeaf(-1.0f, 0.24f, 1.65f, doorAngle * 0.85f);
    glPopMatrix();
}

void drawNoseGear(float deploy) {
    if (deploy <= 0.02f) {
        return;
    }

    GLfloat mat_grey[] = { 0.72f, 0.72f, 0.76f, 1.0f };
    GLfloat mat_dark[] = { 0.12f, 0.12f, 0.13f, 1.0f };

    float retractBlend = 1.0f - deploy;
    float noseLength = 1.18f - (suspension * 0.45f);
    if (noseLength < 0.82f) noseLength = 0.82f;

    glColor3f(0.72f, 0.72f, 0.76f);
    setMaterial(mat_grey);
    glPushMatrix();
    glTranslatef(0.0f, -0.47f + retractBlend * 0.08f, -2.88f + retractBlend * 0.20f);
    glRotatef(108.0f * deploy, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.065f, 0.045f, noseLength, 16, 1);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, noseLength * 0.42f);
    glRotatef(62.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.025f, 0.025f, 0.48f, 12, 1);
    glPopMatrix();

    glTranslatef(0.0f, 0.0f, noseLength);
    glPushMatrix();
    glScalef(1.0f, 0.12f, 1.0f);
    glutSolidCube(0.22f);
    glPopMatrix();

    glColor3f(0.12f, 0.12f, 0.13f);
    setMaterial(mat_dark);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.16f);
    drawWheel(0.045f, 0.14f, 0.07f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.16f);
    drawWheel(0.045f, 0.14f, 0.07f);
    glPopMatrix();
    glPopMatrix();
}

void drawMainGear(float side, float deploy) {
    if (deploy <= 0.02f) {
        return;
    }

    GLfloat mat_grey[] = { 0.72f, 0.72f, 0.76f, 1.0f };
    GLfloat mat_dark[] = { 0.12f, 0.12f, 0.13f, 1.0f };

    float retractBlend = 1.0f - deploy;
    float mainLength = 1.48f - (suspension * 0.55f);
    if (mainLength < 1.05f) mainLength = 1.05f;
    float hingeSplay = side * (5.0f + retractBlend * 10.0f);
    float strutAngle = 96.0f * deploy;

    glColor3f(0.72f, 0.72f, 0.76f);
    setMaterial(mat_grey);
    glPushMatrix();
    glTranslatef(side * (1.06f + retractBlend * 0.10f),
                 -0.16f + retractBlend * 0.04f,
                 1.14f - retractBlend * 0.22f);
    glRotatef(hingeSplay, 0.0f, 0.0f, 1.0f);
    glRotatef(strutAngle, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.095f, 0.075f, mainLength, 16, 1);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, mainLength * 0.48f);
    glRotatef(26.0f, 1.0f, 0.0f, 0.0f);
    gluCylinder(quadric, 0.035f, 0.025f, 0.52f, 12, 1);
    glPopMatrix();

    glTranslatef(0.0f, 0.0f, mainLength);
    glRotatef(-strutAngle + (12.0f * retractBlend), 1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glScalef(0.14f, 0.14f, 0.80f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.12f, 0.12f, 0.13f);
    setMaterial(mat_dark);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.24f);
    drawWheel(0.055f, 0.18f, 0.09f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -0.24f);
    drawWheel(0.055f, 0.18f, 0.09f);
    glPopMatrix();
    glPopMatrix();
}

void drawLandingGear() {
    float deploy = clamp01((gearAnimation - 0.06f) / 0.94f);
    float doorAngle = gearDoorAnim * 88.0f;

    if (gearDoorAnim > 0.02f) {
        drawGearBayCavity(0.0f, -0.53f, -2.90f, 0.34f, 0.08f, 1.20f);
        drawGearBayCavity(1.14f, -0.33f, 1.18f, 0.30f, 0.08f, 1.70f);
        drawGearBayCavity(-1.14f, -0.33f, 1.18f, 0.30f, 0.08f, 1.70f);
    }

    drawNoseGear(deploy);
    drawMainGear(-1.0f, deploy);
    drawMainGear(1.0f, deploy);

    drawGearDoorPanels(doorAngle);
}

}  // namespace

void drawSideWindowsAndLogo() {
    GLfloat mat_window[] = { 0.05f, 0.05f, 0.10f, 1.0f };
    GLfloat mat_red[] = { 0.85f, 0.10f, 0.15f, 1.0f };

    setMaterial(mat_window);
    glColor3f(0.05f, 0.05f, 0.10f);

    for (float z = -2.5f; z < 5.5f; z += 0.4f) {
        glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);
        glVertex3f(0.81f, 0.15f, z);        glVertex3f(0.81f, 0.25f, z);
        glVertex3f(0.81f, 0.25f, z + 0.2f); glVertex3f(0.81f, 0.15f, z + 0.2f);
        glEnd();

        glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-0.81f, 0.15f, z);        glVertex3f(-0.81f, 0.25f, z);
        glVertex3f(-0.81f, 0.25f, z + 0.2f); glVertex3f(-0.81f, 0.15f, z + 0.2f);
        glEnd();
    }

    setMaterial(mat_red);
    glColor3f(0.85f, 0.10f, 0.15f);
    glPushMatrix();
    glTranslatef(0.83f, 0.0f, 3.0f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glScalef(0.0075f, 0.0075f, 0.0075f);

    const char* text = "AIR INDIA";
    for (const char* p = text; *p; ++p) {
        glLineWidth(1.35f);
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        glLineWidth(1.0f);
    }
    glPopMatrix();

    glBegin(GL_QUADS);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-0.82f, 0.35f, -2.5f); glVertex3f(-0.82f, 0.55f, -2.5f);
    glVertex3f(-0.82f, 0.55f, 0.5f);  glVertex3f(-0.82f, 0.35f, 0.5f);
    glEnd();
}

void drawCockpitView() {
    GLfloat mat_black[] = { 0.02f, 0.02f, 0.02f, 1.0f };
    GLfloat mat_glass[] = { 0.30f, 0.40f, 0.50f, 1.0f };

    setMaterial(mat_black);
    glColor3f(0.02f, 0.02f, 0.02f);

    glPushMatrix();
    glTranslatef(0.0f, 0.38f, -4.45f);
    glRotatef(18.0f, 1.0f, 0.0f, 0.0f);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, -0.5f);
    glVertex3f(-0.55f, 0.12f, 0.0f); glVertex3f(0.55f, 0.12f, 0.0f);
    glVertex3f(0.50f, -0.12f, 0.15f); glVertex3f(-0.50f, -0.12f, 0.15f);
    glEnd();

    setMaterial(mat_glass);
    glColor3f(0.30f, 0.40f, 0.50f);
    glTranslatef(0.0f, 0.0f, -0.01f);
    glBegin(GL_QUADS);
    glVertex3f(-0.52f, 0.08f, 0.02f); glVertex3f(-0.28f, 0.10f, 0.01f);
    glVertex3f(-0.28f, -0.08f, 0.12f); glVertex3f(-0.52f, -0.08f, 0.12f);

    glVertex3f(-0.25f, 0.10f, 0.01f); glVertex3f(0.25f, 0.10f, 0.01f);
    glVertex3f(0.25f, -0.08f, 0.12f); glVertex3f(-0.25f, -0.08f, 0.12f);

    glVertex3f(0.28f, 0.10f, 0.01f); glVertex3f(0.52f, 0.08f, 0.02f);
    glVertex3f(0.52f, -0.08f, 0.12f); glVertex3f(0.28f, -0.08f, 0.12f);
    glEnd();
    glPopMatrix();
}

void drawDetailedJet() {
    if (cameraMode == 1) return;

    glPushMatrix();
    glTranslatef(planeX, planeY, planeZ);
    glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(-roll, 0.0f, 0.0f, 1.0f);

    GLfloat mat_white[] = { 0.95f, 0.95f, 0.95f, 1.0f };
    GLfloat mat_red[] = { 0.85f, 0.10f, 0.15f, 1.0f };
    GLfloat mat_gold[] = { 0.85f, 0.65f, 0.15f, 1.0f };
    GLfloat mat_grey[] = { 0.70f, 0.70f, 0.75f, 1.0f };
    GLfloat mat_dark[] = { 0.15f, 0.15f, 0.15f, 1.0f };
    float flapAngle = 0.0f;
    float aileronAngle = 0.0f;
    float elevatorAngle = 0.0f;
    float rudderAngle = 0.0f;

    updateControlSurfaceAnimation(flapAngle, aileronAngle, elevatorAngle, rudderAngle);

    setMaterial(mat_white);
    glColor3f(0.95f, 0.95f, 0.95f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -3.0f);
    gluCylinder(quadric, 0.8f, 0.8f, 9.0f, 48, 1);
    glPopMatrix();

    // Nose cone — cone geometry (avoids GL_CLIP_PLANE0 eye-space issues)
    // Rotates 180° around X so gluCylinder extends in the -Z (forward) direction.
    // Base radius 0.8 matches fuselage; tip radius ~0 for smooth point.
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -3.0f);
    glRotatef(180.0f, 1.0f, 0.0f, 0.0f);   // flip: local +Z → world -Z (nose forward)
    gluCylinder(quadric, 0.8f, 0.025f, 2.2f, 36, 12); // nose cone body
    glPopMatrix();

    // Rounded nose tip (small sphere at the very front)
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.2f);
    glutSolidSphere(0.08f, 14, 14);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 6.0f);
    gluCylinder(quadric, 0.8f, 0.2f, 3.5f, 48, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 9.5f);
    gluSphere(quadric, 0.2f, 16, 16);
    glPopMatrix();

    setMaterial(mat_red);
    glColor3f(0.85f, 0.10f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, -0.2f, -2.8f);
    glScalef(0.98f, 0.78f, 1.0f);
    gluCylinder(quadric, 0.8f, 0.8f, 8.5f, 48, 1);
    glPopMatrix();

    setMaterial(mat_grey);
    glColor3f(0.70f, 0.70f, 0.75f);
    glPushMatrix();
    glTranslatef(0.0f, -0.12f, 1.8f);
    glScalef(0.95f, 0.10f, 4.6f);
    glutSolidCube(1.0f);
    glPopMatrix();

    drawSideWindowsAndLogo();
    drawCockpitView();

    setMaterial(mat_grey);
    glColor3f(0.70f, 0.70f, 0.75f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 1.0f, 0.1f);
    glVertex3f(0.7f, -0.2f, -0.5f);
    glVertex3f(8.5f, -0.1f, 4.0f);
    glVertex3f(0.7f, -0.2f, 3.0f);

    glNormal3f(0.0f, 1.0f, 0.1f);
    glVertex3f(-0.7f, -0.2f, -0.5f);
    glVertex3f(-0.7f, -0.2f, 3.0f);
    glVertex3f(-8.5f, -0.1f, 4.0f);
    glEnd();

    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, -1.0f, -0.1f);
    glVertex3f(0.7f, -0.28f, -0.4f);
    glVertex3f(0.7f, -0.28f, 2.8f);
    glVertex3f(8.5f, -0.18f, 3.9f);

    glNormal3f(0.0f, -1.0f, -0.1f);
    glVertex3f(-0.7f, -0.28f, -0.4f);
    glVertex3f(-8.5f, -0.18f, 3.9f);
    glVertex3f(-0.7f, -0.28f, 2.8f);
    glEnd();

    setMaterial(mat_white);
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(0.0f, -0.16f, 2.05f);
    glRotatef(-flapAngle, 1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.3f, 1.0f);
    glVertex3f(0.85f, 0.0f, 0.0f); glVertex3f(4.35f, 0.10f, 0.65f);
    glVertex3f(4.35f, -0.08f, 1.35f); glVertex3f(0.85f, -0.10f, 0.95f);

    glVertex3f(-0.85f, 0.0f, 0.0f); glVertex3f(-0.85f, -0.10f, 0.95f);
    glVertex3f(-4.35f, -0.08f, 1.35f); glVertex3f(-4.35f, 0.10f, 0.65f);
    glEnd();
    glPopMatrix();

    drawHingedSurface(mat_white, 0.95f, 0.95f, 0.95f,
                      5.35f, -0.16f, 2.70f,
                      aileronAngle, 0.0f,
                      0.0f, 0.0f, 0.55f,
                      2.20f, 0.08f, 1.10f);
    drawHingedSurface(mat_white, 0.95f, 0.95f, 0.95f,
                      -5.35f, -0.16f, 2.70f,
                      -aileronAngle, 0.0f,
                      0.0f, 0.0f, 0.55f,
                      2.20f, 0.08f, 1.10f);

    setMaterial(mat_gold);
    glColor3f(0.85f, 0.65f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(8.5f, -0.1f, 3.5f); glVertex3f(8.7f, 1.4f, 4.2f);
    glVertex3f(8.7f, 1.4f, 4.8f); glVertex3f(8.5f, -0.1f, 4.0f);

    glVertex3f(-8.5f, -0.1f, 3.5f); glVertex3f(-8.5f, -0.1f, 4.0f);
    glVertex3f(-8.7f, 1.4f, 4.8f); glVertex3f(-8.7f, 1.4f, 4.2f);
    glEnd();

    float enginePositions[2] = { 3.0f, -3.0f };
    for (int i = 0; i < 2; ++i) {
        float engineX = enginePositions[i];

        setMaterial(mat_white);
        glColor3f(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(engineX, -0.45f, 0.55f);
        glRotatef(78.0f, 1.0f, 0.0f, 0.0f);
        glScalef(0.28f, 0.70f, 1.15f);
        glutSolidCube(1.0f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(engineX, -0.8f, -0.2f);

        setMaterial(mat_red);
        glColor3f(0.85f, 0.10f, 0.15f);
        gluCylinder(quadric, 0.60f, 0.46f, 2.55f, 36, 1);

        setMaterial(mat_gold);
        glColor3f(0.85f, 0.65f, 0.15f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, -0.12f);
        gluCylinder(quadric, 0.57f, 0.60f, 0.12f, 32, 1);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0f, 0.0f, -0.01f);
        drawEngineFan(0.46f);
        glPopMatrix();

        setMaterial(mat_dark);
        glColor3f(0.15f, 0.15f, 0.15f);
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 2.55f);
        gluCylinder(quadric, 0.45f, 0.38f, 0.28f, 28, 1);
        glPopMatrix();

        // ── Afterburner cone ─────────────────────────────────────────────────
        if (afterburnerIntensity > 0.01f) {
            float t       = afterburnerIntensity;
            float flicker = 0.82f + 0.18f * std::sin(lightTimer * 42.0f + (float)i * 1.3f);
            float coneLen = (2.8f + 0.6f * std::sin(lightTimer * 28.0f)) * t;
            float baseR   = 0.36f * t;

            glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_LIGHTING);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE); // additive blend

            // Nozzle exit is at local z = 2.83 (2.55 + 0.28)
            float nozzleZ = 2.83f;
            int   segs    = 24;

            // --- Outer halo cone (orange/yellow) ---
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 0.55f, 0.05f, 0.0f);           // tip: transparent
            glVertex3f(0.0f, 0.0f, nozzleZ + coneLen);
            glColor4f(1.0f, 0.45f, 0.0f, 0.45f * t * flicker);
            for (int s = 0; s <= segs; ++s) {
                float ang = s * 2.0f * (float)M_PI / segs;
                glVertex3f(std::cos(ang) * baseR * 1.35f,
                           std::sin(ang) * baseR * 1.35f,
                           nozzleZ);
            }
            glEnd();

            // --- Inner core cone (blue-white) ---
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.85f, 0.95f, 1.0f, 0.0f);           // tip: transparent
            glVertex3f(0.0f, 0.0f, nozzleZ + coneLen * 0.72f);
            glColor4f(0.7f, 0.85f, 1.0f, 0.75f * t * flicker);
            for (int s = 0; s <= segs; ++s) {
                float ang = s * 2.0f * (float)M_PI / segs;
                glVertex3f(std::cos(ang) * baseR * 0.65f,
                           std::sin(ang) * baseR * 0.65f,
                           nozzleZ);
            }
            glEnd();

            // --- Bright nozzle glow disk ---
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(1.0f, 0.80f, 0.30f, 0.90f * t * flicker);
            glVertex3f(0.0f, 0.0f, nozzleZ + 0.02f);
            glColor4f(1.0f, 0.50f, 0.05f, 0.0f);
            for (int s = 0; s <= segs; ++s) {
                float ang = s * 2.0f * (float)M_PI / segs;
                glVertex3f(std::cos(ang) * baseR,
                           std::sin(ang) * baseR,
                           nozzleZ + 0.02f);
            }
            glEnd();

            glDepthMask(GL_TRUE);
            glPopAttrib();
        }
        // ── End afterburner ──────────────────────────────────────────────────

        glPopMatrix();
    }

    setMaterial(mat_white);
    glColor3f(0.95f, 0.95f, 0.95f);
    glBegin(GL_POLYGON);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.7f, 6.0f);
    glVertex3f(0.0f, 3.5f, 8.5f);
    glVertex3f(0.0f, 3.5f, 9.8f);
    glVertex3f(0.0f, 0.7f, 9.2f);
    glEnd();

    setMaterial(mat_gold);
    glColor3f(0.85f, 0.65f, 0.15f);
    glBegin(GL_QUADS);
    glVertex3f(0.01f, 1.5f, 7.1f); glVertex3f(0.01f, 2.2f, 7.5f);
    glVertex3f(0.01f, 2.2f, 9.5f); glVertex3f(0.01f, 1.5f, 9.0f);

    glVertex3f(-0.01f, 1.5f, 7.1f); glVertex3f(-0.01f, 1.5f, 9.0f);
    glVertex3f(-0.01f, 2.2f, 9.5f); glVertex3f(-0.01f, 2.2f, 7.5f);
    glEnd();

    setMaterial(mat_red);
    setMaterial(mat_red);
    glColor3f(0.85f, 0.10f, 0.15f);
    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.01f, 1.0f); // Face North
    glVertex3f(0.02f, 2.2f, 7.5f); glVertex3f(0.02f, 3.5f, 8.5f);
    glVertex3f(0.02f, 3.5f, 9.8f); glVertex3f(0.02f, 2.2f, 9.5f);
    glEnd();

    glBegin(GL_POLYGON);
    glNormal3f(0.0f, 0.01f, -1.0f); // Face South
    glVertex3f(-0.02f, 2.2f, 7.5f); glVertex3f(-0.02f, 2.2f, 9.5f);
    glVertex3f(-0.02f, 3.5f, 9.8f); glVertex3f(-0.02f, 3.5f, 8.5f);
    glEnd();

    // Enhanced vertical rudder fin - now much larger and more visible
    drawHingedSurface(mat_red, 1.0f, 0.15f, 0.20f,
                      0.0f, 2.0f, 7.2f,
                      0.0f, rudderAngle,
                      0.0f, 1.10f, 1.45f,
                      0.16f, 2.20f, 2.80f);

    // Bright accent stripe on rudder
    setMaterial(mat_gold);
    glColor3f(1.0f, 0.85f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 2.0f, 7.2f);
    glRotatef(rudderAngle, 0.0f, 1.0f, 0.0f);
    glTranslatef(0.0f, 1.10f + 0.55f, 1.45f);
    glScalef(0.05f, 0.35f, 0.90f);
    glutSolidCube(1.0f);
    glPopMatrix();

    setMaterial(mat_grey);
    glColor3f(0.70f, 0.70f, 0.75f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 1.0f, 0.2f);
    glVertex3f(0.3f, 0.3f, 7.5f); glVertex3f(3.5f, 0.3f, 9.2f); glVertex3f(0.3f, 0.3f, 9.5f);
    glNormal3f(0.0f, 1.0f, 0.2f);
    glVertex3f(-0.3f, 0.3f, 7.5f); glVertex3f(-0.3f, 0.3f, 9.5f); glVertex3f(-3.5f, 0.3f, 9.2f);
    glEnd();

    // Enhanced horizontal stabilizers (elevators) - larger and more prominent
    drawHingedSurface(mat_grey, 0.80f, 0.80f, 0.85f,
                      2.15f, -0.15f, 7.75f,
                      elevatorAngle, 0.0f,
                      0.0f, 0.0f, 0.85f,
                      3.20f, 0.12f, 1.70f);
    drawHingedSurface(mat_grey, 0.80f, 0.80f, 0.85f,
                      -2.15f, -0.15f, 7.75f,
                      elevatorAngle, 0.0f,
                      0.0f, 0.0f, 0.85f,
                      3.20f, 0.12f, 1.70f);

    // Bright accent stripes on elevators for pitch feedback
    setMaterial(mat_gold);
    glColor3f(0.95f, 0.80f, 0.0f);
    glPushMatrix();
    glTranslatef(2.15f, -0.15f, 7.75f);
    glRotatef(elevatorAngle, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, 0.85f);
    glScalef(0.15f, 0.06f, 0.60f);
    glutSolidCube(1.0f);
    glPopMatrix();
    
    glPushMatrix();
    glTranslatef(-2.15f, -0.15f, 7.75f);
    glRotatef(elevatorAngle, 1.0f, 0.0f, 0.0f);
    glTranslatef(0.0f, 0.0f, 0.85f);
    glScalef(0.15f, 0.06f, 0.60f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Wing-mounted vertical fins (winglets) with aileron response
    // Left wing aileron fin
    setMaterial(mat_red);
    glColor3f(0.90f, 0.15f, 0.20f);
    glPushMatrix();
    glTranslatef(-9.2f, 0.5f, 3.5f);
    if (aileronAngle != 0.0f) {
        glRotatef(aileronAngle * 0.5f, 0.0f, 0.0f, 1.0f);
    }
    glNormal3f(1.0f, 0.0f, 0.0f); // Facing inward
    glScalef(0.35f, 0.90f, 0.15f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Right wing aileron fin
    setMaterial(mat_red);
    glColor3f(0.90f, 0.15f, 0.20f);
    glPushMatrix();
    glTranslatef(9.2f, 0.5f, 3.5f);
    if (aileronAngle != 0.0f) {
        glRotatef(-aileronAngle * 0.5f, 0.0f, 0.0f, 1.0f);
    }
    glNormal3f(-1.0f, 0.0f, 0.0f); // Facing inward
    glScalef(0.35f, 0.90f, 0.15f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Bright accent on winglets for roll feedback
    setMaterial(mat_gold);
    glColor3f(1.0f, 0.90f, 0.0f);
    glPushMatrix();
    glTranslatef(-9.2f, 0.5f, 3.5f);
    if (aileronAngle != 0.0f) {
        glRotatef(aileronAngle * 0.5f, 0.0f, 0.0f, 1.0f);
    }
    glTranslatef(0.0f, 0.30f, 0.0f);
    glScalef(0.06f, 0.30f, 0.12f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(9.2f, 0.5f, 3.5f);
    if (aileronAngle != 0.0f) {
        glRotatef(-aileronAngle * 0.5f, 0.0f, 0.0f, 1.0f);
    }
    glTranslatef(0.0f, 0.30f, 0.0f);
    glScalef(0.06f, 0.30f, 0.12f);
    glutSolidCube(1.0f);
    glPopMatrix();

    drawLandingGear();

    // ── Navigation and strobe lights ────────────────────────────────────────
    // Faster, sharper blink for "blinking" effect
    float navBlink = (std::sin(lightTimer * 12.0f) > 0.0f) ? 1.0f : 0.05f; 
    float strobe   = (std::sin(lightTimer * 20.0f) > 0.8f) ? 1.0f : 0.0f;

    // Night light window: 6 PM (18.0) to 9 AM (9.0)
    bool  isNight  = (gameTime >= 18.0f || gameTime < 9.0f);
    float navBase  = isNight ? 1.0f : 0.45f;
    float glowSize = isNight ? 1.0f : 0.0f; // halos only at night

    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);   // lights always visible
    glDepthMask(GL_FALSE);

    // ── Left wing: RED nav light ─────────────────────────────────────────────
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glColor4f(0.95f * navBase, 0.05f, 0.05f, navBlink); // Blinking core
    glPushMatrix(); glTranslatef(-8.5f, 0.2f, 4.9f);
    glutSolidSphere(0.02f, 8, 8); // pinpoint core (reduced 70%)
    if (glowSize > 0.01f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 0.0f, 0.0f, 0.80f * navBlink);
        glutSolidSphere(0.05f, 8, 8); // pinpoint glow
        glColor4f(1.0f, 0.1f, 0.0f, 0.30f * navBlink);
        glutSolidSphere(0.12f, 6, 6);  // pinpoint halo
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glPopMatrix();

    // ── Right wing: GREEN nav light ──────────────────────────────────────────
    glColor4f(0.05f, 0.95f * navBase, 0.05f, navBlink);
    glPushMatrix(); glTranslatef(8.5f, 0.2f, 4.9f);
    glutSolidSphere(0.02f, 8, 8);
    if (glowSize > 0.01f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.0f, 1.0f, 0.0f, 0.80f * navBlink);
        glutSolidSphere(0.05f, 8, 8);
        glColor4f(0.0f, 1.0f, 0.1f, 0.30f * navBlink);
        glutSolidSphere(0.12f, 6, 6);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glPopMatrix();

    // ── Tail: WHITE glowing night light ──────────────────────────────────────
    glColor4f(navBase, navBase, navBase * 0.9f, navBlink);
    glPushMatrix(); glTranslatef(0.0f, 0.7f, -5.0f);
    glutSolidSphere(0.02f, 8, 8);
    if (glowSize > 0.01f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, 0.90f * navBlink);
        glutSolidSphere(0.06f, 8, 8);
        glColor4f(0.9f, 0.9f, 1.0f, 0.35f * navBlink);
        glutSolidSphere(0.15f, 6, 6);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glPopMatrix();

    // ── Wingtip STROBE lights (white, sharp triple pulse) ───────────────────
    // Left wingtip strobe
    glColor4f(strobe, strobe, strobe, strobe);
    glPushMatrix(); glTranslatef(-8.5f, 0.35f, 4.9f);
    glutSolidSphere(0.015f, 6, 6);
    if (strobe > 0.01f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, strobe);
        glutSolidSphere(0.08f, 6, 6);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glPopMatrix();

    // Right wingtip strobe
    glColor4f(strobe, strobe, strobe, strobe);
    glPushMatrix(); glTranslatef(8.5f, 0.35f, 4.9f);
    glutSolidSphere(0.015f, 6, 6);
    if (strobe > 0.01f) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(1.0f, 1.0f, 1.0f, strobe);
        glutSolidSphere(0.08f, 6, 6);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    glPopMatrix();

    // ── Nose: landing / taxi light (always blinking at night) ────────────────
    if (isNight) {
        glColor4f(0.98f, 0.98f, 1.0f, navBlink);
        glPushMatrix(); glTranslatef(0.0f, -0.2f, 9.2f);
        glutSolidSphere(0.025f, 8, 8);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glColor4f(0.85f, 0.90f, 1.0f, 0.70f * navBlink);
        glutSolidSphere(0.08f, 8, 8);
        glColor4f(0.80f, 0.85f, 1.0f, 0.25f * navBlink);
        glutSolidSphere(0.17f, 6, 6);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glPopMatrix();
    }


    glDepthMask(GL_TRUE);
    glPopAttrib();




    glPopMatrix();
}

// Load a plane model from the planes/ folder
bool loadSelectedPlaneModel(const std::string& planeFilename) {
    std::string fullPath = "planes/" + planeFilename;
    
    printf("Loading plane model: %s\n", fullPath.c_str());
    fflush(stdout);
    
    if (g_useLoadedModel) {
        printf("Unloading previous model...\n");
        fflush(stdout);
        unloadJetModel();  // Unload previous model
    }

    LoadedModel newModel;
    newModel.scene = nullptr;
    
    if (!loadModelFromFile(fullPath, newModel)) {
        printf("ERROR: Failed to load plane model: %s\n", fullPath.c_str());
        fflush(stdout);
        g_useLoadedModel = false;
        return false;
    }

    g_loadedJetModel = newModel;
    g_useLoadedModel = true;
    printf("Plane model loaded successfully!\n");
    fflush(stdout);
    return true;
}

// Draw the jet using either loaded model or default procedural jet

struct DebrisPart {
    float x, y, z;
    float vx, vy, vz;
    float rx, ry, rz;
    float rv;
    float size;
};
#define MAX_DEBRIS 250
static DebrisPart s_debris[MAX_DEBRIS];
static bool s_debrisInit = false;

void drawExplosionAndSparks() {
    if (isExploding) {
        if (!s_debrisInit) {
            for (int i = 0; i < MAX_DEBRIS; i++) {
                s_debris[i].x = planeX;
                s_debris[i].y = planeY;
                s_debris[i].z = planeZ;
                
                // Explode outward with high velocity
                float angle1 = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
                float angle2 = ((float)rand() / RAND_MAX) * M_PI - M_PI/2.0f;
                float speed = ((float)rand() / RAND_MAX) * 800.0f + 200.0f; // Violent burst
                
                s_debris[i].vx = cos(angle1) * cos(angle2) * speed;
                s_debris[i].vy = sin(angle2) * speed + 300.0f; // Upward bias
                s_debris[i].vz = sin(angle1) * cos(angle2) * speed;
                
                s_debris[i].rx = ((float)rand() / RAND_MAX) * 360.0f;
                s_debris[i].ry = ((float)rand() / RAND_MAX) * 360.0f;
                s_debris[i].rz = ((float)rand() / RAND_MAX) * 360.0f;
                s_debris[i].rv = ((float)rand() / RAND_MAX) * 500.0f - 250.0f;
                s_debris[i].size = ((float)rand() / RAND_MAX) * 6.0f + 1.0f;
            }
            s_debrisInit = true;
        }

        // Integrate Debris Physics
        for (int i = 0; i < MAX_DEBRIS; i++) {
            s_debris[i].y -= 9.81f * 40.0f * deltaTime; // Gravity
            // Ground collision
            float groundH = getVoxelHeight(s_debris[i].x, s_debris[i].z);
            if (s_debris[i].y < groundH) {
                s_debris[i].y = groundH;
                s_debris[i].vy = -s_debris[i].vy * 0.4f; // Bounce
                s_debris[i].vx *= 0.8f; // Friction
                s_debris[i].vz *= 0.8f;
                s_debris[i].rv *= 0.8f;
            } else {
                s_debris[i].x += s_debris[i].vx * deltaTime;
                s_debris[i].y += s_debris[i].vy * deltaTime;
                s_debris[i].z += s_debris[i].vz * deltaTime;
            }
            
            s_debris[i].rx += s_debris[i].rv * deltaTime;
            s_debris[i].ry += s_debris[i].rv * deltaTime;
            s_debris[i].rz += s_debris[i].rv * deltaTime;
            
            glPushMatrix();
            glTranslatef(s_debris[i].x, s_debris[i].y, s_debris[i].z);
            glRotatef(s_debris[i].rx, 1.0f, 0.0f, 0.0f);
            glRotatef(s_debris[i].ry, 0.0f, 1.0f, 0.0f);
            glRotatef(s_debris[i].rz, 0.0f, 0.0f, 1.0f);
            
            // Fire to smoke gradient
            float heat = 1.0f - (explosionTimer / 0.4f);
            if (heat > 0.5f) {
                glColor4f(1.0f, 0.5f, 0.0f, 1.0f); // Fire
            } else {
                glColor4f(0.2f, 0.2f, 0.2f, heat * 2.0f); // Smoke
            }
            
            glutSolidCube(s_debris[i].size);
            glPopMatrix();
        }
        
    } else {
        s_debrisInit = false; // Reset for next time
    }

    if (isBellyLanding && currentSpeed > 10.0f) {
        glPushMatrix();
        glTranslatef(planeX, planeY, planeZ);
        // Sparks for belly slide
        glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
        float clearanceOffset = 2.0f; // Fixed belly clearance in physics
        glTranslatef(0.0f, -clearanceOffset, 0.0f); // Under the belly
        for (int i = 0; i < 15; i++) {
            glPushMatrix();
            // Sparks fly backwards and randomly sideways/up
            float dx = ((float)rand() / RAND_MAX - 0.5f) * 4.0f;
            float dy = ((float)rand() / RAND_MAX) * 2.0f;
            float dz = ((float)rand() / RAND_MAX) * 8.0f + 2.0f; // Behind the plane
            glTranslatef(dx, dy, dz);
            glColor4f(1.0f, 0.8f + ((float)rand()/RAND_MAX)*0.2f, 0.0f, 1.0f); // Yellowish-orange
            glutSolidCube(0.5f);
            glPopMatrix();
        }
        glPopMatrix();
    }
}

void drawJet() {
    if (g_useLoadedModel && g_loadedJetModel.scene) {
        glPushMatrix();
        glTranslatef(planeX, planeY, planeZ);
        glRotatef(-yaw, 0.0f, 1.0f, 0.0f);
        glRotatef(pitch, 1.0f, 0.0f, 0.0f);
        glRotatef(-roll, 0.0f, 0.0f, 1.0f);
        
        // Apply user-defined adjustments
        glScalef(modelGlobalScale, modelGlobalScale, modelGlobalScale);
        glRotatef(modelGlobalRotX, 1.0f, 0.0f, 0.0f);
        glRotatef(modelGlobalRotY, 0.0f, 1.0f, 0.0f);
        glRotatef(modelGlobalRotZ, 0.0f, 0.0f, 1.0f);
        
        // Disable shader to use fixed-function pipeline for model rendering
        glUseProgram(0);
        drawLoadedModel(g_loadedJetModel);
        // Shader will be re-enabled by display() for next frame
        
        glPopMatrix();
    } else {
        // Fall back to default procedural jet
        drawDetailedJet();
    }
}

// Unload the model and free resources
void unloadJetModel() {
    if (g_loadedJetModel.scene) {
        unloadModel(g_loadedJetModel);
        g_useLoadedModel = false;
    }
}

