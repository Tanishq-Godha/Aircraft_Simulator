#include "jet.h"
#include "globals.h"
#include <GL/glut.h>
#include <GL/glu.h>
#include <math.h>

extern GLUquadric* quadric;

namespace {

float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

void setMaterial(const GLfloat* mat) {
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat);
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
    glRotatef(yaw, 0.0f, 1.0f, 0.0f);
    glRotatef(pitch, 1.0f, 0.0f, 0.0f);
    glRotatef(roll, 0.0f, 0.0f, 1.0f);

    GLfloat mat_white[] = { 0.95f, 0.95f, 0.95f, 1.0f };
    GLfloat mat_red[] = { 0.85f, 0.10f, 0.15f, 1.0f };
    GLfloat mat_gold[] = { 0.85f, 0.65f, 0.15f, 1.0f };
    GLfloat mat_grey[] = { 0.70f, 0.70f, 0.75f, 1.0f };
    GLfloat mat_dark[] = { 0.15f, 0.15f, 0.15f, 1.0f };

    setMaterial(mat_white);
    glColor3f(0.95f, 0.95f, 0.95f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -3.0f);
    gluCylinder(quadric, 0.8f, 0.8f, 9.0f, 48, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -3.0f);
    glScalef(1.0f, 1.0f, 2.5f);
    double eqn[4] = { 0.0, 0.0, -1.0, 0.0 };
    glClipPlane(GL_CLIP_PLANE0, eqn);
    glEnable(GL_CLIP_PLANE0);
    gluSphere(quadric, 0.8f, 48, 48);
    glDisable(GL_CLIP_PLANE0);
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

    float flapAngle = flaps * 28.0f;
    setMaterial(mat_white);
    glColor3f(0.95f, 0.95f, 0.95f);
    glPushMatrix();
    glTranslatef(0.0f, -0.16f, 2.05f);
    glRotatef(-flapAngle, 1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 0.3f, 1.0f);
    glVertex3f(0.85f, 0.0f, 0.0f); glVertex3f(5.50f, 0.12f, 0.75f);
    glVertex3f(5.50f, -0.10f, 1.55f); glVertex3f(0.85f, -0.10f, 0.95f);

    glVertex3f(-0.85f, 0.0f, 0.0f); glVertex3f(-0.85f, -0.10f, 0.95f);
    glVertex3f(-5.50f, -0.10f, 1.55f); glVertex3f(-5.50f, 0.12f, 0.75f);
    glEnd();
    glPopMatrix();

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
    glColor3f(0.85f, 0.10f, 0.15f);
    glBegin(GL_POLYGON);
    glVertex3f(0.02f, 2.2f, 7.5f); glVertex3f(0.02f, 3.5f, 8.5f);
    glVertex3f(0.02f, 3.5f, 9.8f); glVertex3f(0.02f, 2.2f, 9.5f);
    glEnd();

    glBegin(GL_POLYGON);
    glVertex3f(-0.02f, 2.2f, 7.5f); glVertex3f(-0.02f, 2.2f, 9.5f);
    glVertex3f(-0.02f, 3.5f, 9.8f); glVertex3f(-0.02f, 3.5f, 8.5f);
    glEnd();

    setMaterial(mat_grey);
    glColor3f(0.70f, 0.70f, 0.75f);
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 1.0f, 0.2f);
    glVertex3f(0.3f, 0.3f, 7.5f); glVertex3f(3.5f, 0.3f, 9.2f); glVertex3f(0.3f, 0.3f, 9.5f);
    glNormal3f(0.0f, 1.0f, 0.2f);
    glVertex3f(-0.3f, 0.3f, 7.5f); glVertex3f(-0.3f, 0.3f, 9.5f); glVertex3f(-3.5f, 0.3f, 9.2f);
    glEnd();

    drawLandingGear();

    glPopMatrix();
}
