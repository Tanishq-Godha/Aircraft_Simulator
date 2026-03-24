#include "hud.h"
#include "globals.h"
#include "terrain.h"
#include <GL/glut.h>
#include <string>
#include <cmath>

// --- HUD Display ---
void renderText(float x, float y, void* font, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

void drawHUD() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); 
    float cx = screenW / 2.0f; float cy = screenH / 2.0f;
    float agl = planeY - getVoxelHeight(planeX, planeZ);
    if (agl < 0.0f) agl = 0.0f;

    std::string gearText = "GEAR: ";
    if (gearInTransition) gearText += (gearDeployed ? "DEPLOYING" : "RETRACTING");
    else gearText += ((gearAnimation > 0.95f) ? "DOWN" : "UP");

    std::string flightState = "AIRBORNE";
    if (crashed) flightState = "CRASHED";
    else if (isGrounded) flightState = "ROLLING";

    std::string viewText = "";
    if (cameraMode == 0) viewText = "VIEW: CHASE";
    if (cameraMode == 1) viewText = "VIEW: COCKPIT";
    if (cameraMode == 2) viewText = "VIEW: CINEMATIC";
    if (cameraMode == 3) viewText = "VIEW: FLY-BY";
    renderText(cx - 50, screenH - 30, GLUT_BITMAP_HELVETICA_18, viewText);
    renderText(cx - 55, screenH - 50, GLUT_BITMAP_HELVETICA_12, "STATE: " + flightState);

    // Game time display
    int hour = (int)gameTime;
    int min = (int)((gameTime - hour) * 60.0f);
    std::string timeStr = "TIME: " + std::to_string(hour) + ":" + (min < 10 ? "0" : "") + std::to_string(min);
    renderText(screenW - 200, screenH - 30, GLUT_BITMAP_HELVETICA_18, timeStr);

    renderText(50, cy, GLUT_BITMAP_HELVETICA_18, "SPD: " + std::to_string((int)currentSpeed));
    renderText(screenW - 140, cy, GLUT_BITMAP_HELVETICA_18, "AGL: " + std::to_string((int)agl));
    
    renderText(50, cy - 30, GLUT_BITMAP_HELVETICA_12, "THRUST: " + std::to_string((int)(throttle * 100)) + "%");
    renderText(50, cy - 60, GLUT_BITMAP_HELVETICA_12,
           "FLAPS: " + std::to_string((int)(flaps * 100)) + "%");

    renderText(50, cy - 80, GLUT_BITMAP_HELVETICA_12,
            gearText);
    renderText(50, cy - 100, GLUT_BITMAP_HELVETICA_12,
            "SPEED CTRL: R/F OR UP/DOWN");

    if (gearAnimation < 0.95f && agl < 1200.0f && !isGrounded) {
        glColor3f(1,0,0);
        renderText(cx - 80, cy + 140, GLUT_BITMAP_HELVETICA_18, "CHECK GEAR");
    } else if (flaps < 0.25f && agl < 1000.0f && currentSpeed < 260.0f && !isGrounded) {
        glColor3f(1,0.6f,0);
        renderText(cx - 85, cy + 140, GLUT_BITMAP_HELVETICA_18, "ADD FLAPS");
    }
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glBegin(GL_LINE_LOOP); glVertex2f(50, cy - 45); glVertex2f(150, cy - 45); glVertex2f(150, cy - 55); glVertex2f(50, cy - 55); glEnd();
    glBegin(GL_QUADS); glVertex2f(50, cy - 45); glVertex2f(50 + (throttle * 100.0f), cy - 45); glVertex2f(50 + (throttle * 100.0f), cy - 55); glVertex2f(50, cy - 55); glEnd();

    glBegin(GL_LINES);
    glVertex2f(cx - 15, cy); glVertex2f(cx - 5, cy); glVertex2f(cx + 5, cy);  glVertex2f(cx + 15, cy);
    glVertex2f(cx, cy - 15); glVertex2f(cx, cy - 5);
    glEnd();

    glPushMatrix();
    glTranslatef(cx, cy, 0.0f); glRotatef(roll, 0.0f, 0.0f, 1.0f); glTranslatef(0.0f, -pitch * 5.0f, 0.0f); 
    glBegin(GL_LINES);
    glVertex2f(-100, 0); glVertex2f(-30, 0); glVertex2f(30, 0); glVertex2f(100, 0);
    for(int i = -90; i <= 90; i += 15) {
        if(i == 0) continue;
        float yOff = i * 5.0f;
        glVertex2f(-50, yOff); glVertex2f(-30, yOff); glVertex2f(30, yOff); glVertex2f(50, yOff);  
        if(i > 0) { glVertex2f(-50, yOff); glVertex2f(-50, yOff - 5); glVertex2f(50, yOff); glVertex2f(50, yOff - 5); } 
        else { glVertex2f(-50, yOff); glVertex2f(-50, yOff + 5); glVertex2f(50, yOff); glVertex2f(50, yOff + 5); }
    }
    glEnd(); glPopMatrix();

    if (isStalling) {
        glColor3f(1.0f, 0.0f, 0.0f);
        renderText(cx - 40, cy + 100, GLUT_BITMAP_TIMES_ROMAN_24, "STALL!");
    }

    if (crashed) {
        glColor3f(1.0f, 0.0f, 0.0f);
        renderText(cx - 90, cy + 70, GLUT_BITMAP_HELVETICA_18, "CRASHED - PRESS P");
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}
