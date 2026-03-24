#include "hud.h"
#include "globals.h"
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

    std::string viewText = "";
    if (cameraMode == 0) viewText = "VIEW: CHASE";
    if (cameraMode == 1) viewText = "VIEW: COCKPIT";
    if (cameraMode == 2) viewText = "VIEW: CINEMATIC";
    if (cameraMode == 3) viewText = "VIEW: FLY-BY";
    renderText(cx - 50, screenH - 30, GLUT_BITMAP_HELVETICA_18, viewText);

    renderText(50, cy, GLUT_BITMAP_HELVETICA_18, "SPD: " + std::to_string((int)currentSpeed));
    renderText(screenW - 120, cy, GLUT_BITMAP_HELVETICA_18, "ALT: " + std::to_string((int)planeY));
    
    renderText(50, cy - 30, GLUT_BITMAP_HELVETICA_12, "THRUST: " + std::to_string((int)(throttle * 100)) + "%");
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

    if (isStalling) { glColor3f(1.0f, 0.0f, 0.0f); renderText(cx - 40, cy + 100, GLUT_BITMAP_TIMES_ROMAN_24, "STALL!"); }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}