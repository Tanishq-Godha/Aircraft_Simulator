#include "menu.h"
#include "globals.h"
#include "math_utils.h"
#include <GL/glut.h>
#include <cstring>
#include <cstdlib>

namespace {

void drawText(float x, float y, const char* text, void* font = GLUT_BITMAP_HELVETICA_18) {
    glRasterPos2f(x, y);
    for (const char* c = text; *c; c++) {
        glutBitmapCharacter(font, *c);
    }
}

void drawStrokeText(float x, float y, const char* text, float scale = 0.2f, bool bold = false) {
    glPushMatrix();
    glTranslatef(x, y, 0);
    glScalef(scale, scale, 1.0f);
    if (bold) {
        glLineWidth(2.0f);
        for (const char* c = text; *c; c++) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
        glLineWidth(1.0f);
    } else {
        for (const char* c = text; *c; c++) {
            glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
        }
    }
    glPopMatrix();
}

void drawBackgroundGradient() {
    glBegin(GL_QUADS);
    glColor3f(0.10f, 0.14f, 0.28f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(screenW, 0.0f);
    glColor3f(0.04f, 0.18f, 0.35f);
    glVertex2f(screenW, screenH);
    glVertex2f(0.0f, screenH);
    glEnd();
}

void drawButton(int x1, int y1, int width, int height, const char* label, bool hover = false) {
    float r = hover ? 0.9f : 1.0f;
    float g = hover ? 0.9f : 1.0f;
    float b = hover ? 0.9f : 1.0f;

    glColor4f(0.85f, 0.95f, 1.0f, 0.15f);
    glBegin(GL_QUADS);
    glVertex2f(x1 - 3, y1 - 3);
    glVertex2f(x1 + width + 3, y1 - 3);
    glVertex2f(x1 + width + 3, y1 + height + 3);
    glVertex2f(x1 - 3, y1 + height + 3);
    glEnd();

    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x1 + width, y1);
    glVertex2f(x1 + width, y1 + height);
    glVertex2f(x1, y1 + height);
    glEnd();

    float textWidth = float(std::strlen(label) * 9);
    float textX = x1 + (width - textWidth) * 0.5f;
    float textY = y1 + (height - 18) * 0.5f + 4;

    glColor3f(0.0f, 0.0f, 0.0f);
    drawText(textX, textY, label, GLUT_BITMAP_HELVETICA_18);
}

bool pointInRect(int px, int py, int x1, int y1, int width, int height) {
    return px >= x1 && px <= x1 + width && py >= y1 && py <= y1 + height;
}

} // namespace

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int mouseY = screenH - y;
        
        if (gameState == 0) { // Menu
            int buttonWidth = 250;
            int buttonHeight = 50;
            int centerX = screenW / 2;
            int startY = screenH / 2 + 20;
            
            if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY, buttonWidth, buttonHeight)) {
                gameState = 1;
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 70, buttonWidth, buttonHeight)) {
                gameState = 2;
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 140, buttonWidth, buttonHeight)) {
                gameState = 3;
            }
            else if (pointInRect(x, mouseY, screenW - 130, 10, 120, 40)) {
                exit(0);
            }
        } else if (gameState == 2) { // Controls
            if (pointInRect(x, mouseY, 50, 50, 100, 30)) {
                gameState = 0;
            }
        } else if (gameState == 3) { // Map select
            int centerX = screenW / 2;
            int centerY = screenH / 2;
            if (pointInRect(x, mouseY, centerX - 125, centerY + 20, 250, 50)) {
                selectedMap = 0;
                gameState = 0;
            }
            else if (pointInRect(x, mouseY, centerX - 125, centerY - 40, 250, 50)) {
                selectedMap = 1;
                gameState = 0;
            }
            else if (pointInRect(x, mouseY, 50, 50, 120, 40)) {
                gameState = 0;
            }
        }
    }
}

void drawMenu() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    drawBackgroundGradient();

    int buttonWidth = 250;
    int buttonHeight = 50;
    int centerX = screenW / 2;
    int startY = screenH / 2 + 20;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(centerX - 250, startY + 80, "Aircraft Simulator", 0.4f, true);

    drawButton(centerX - buttonWidth / 2, startY, buttonWidth, buttonHeight, "Start Game");
    drawButton(centerX - buttonWidth / 2, startY - 70, buttonWidth, buttonHeight, "Controls");
    drawButton(centerX - buttonWidth / 2, startY - 140, buttonWidth, buttonHeight, "Select Map");

    drawButton(screenW - 130, 10, 120, 40, "Exit");

    glutSwapBuffers();
}

void drawControls() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    drawBackgroundGradient();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(60, screenH - 60, "Game Controls", 0.25f, true);

    int rowStart = screenH - 110;
    int step = 26;
    // Column 1
    drawText(60,  rowStart,          "WASD     - Pitch / Roll");
    drawText(60,  rowStart - step,   "R / F    - Throttle Up/Down");
    drawText(60,  rowStart - 2*step, "+/-      - Throttle nudge");
    drawText(60,  rowStart - 3*step, "G        - Landing Gear");
    drawText(60,  rowStart - 4*step, "[/]      - Flaps");
    drawText(60,  rowStart - 5*step, "V        - Camera Mode");
    drawText(60,  rowStart - 6*step, "T        - Time Speed");
    drawText(60,  rowStart - 7*step, "P        - Full Reset");
    drawText(60,  rowStart - 8*step, "M        - Return to Menu");
    drawText(60,  rowStart - 9*step, "Y        - Pause / Resume");
    drawText(60,  rowStart - 10*step,"H        - Autopilot (Alt Hold)");
    drawText(60,  rowStart - 11*step,"L        - Auto-Land");
    drawText(60,  rowStart - 12*step,"ESC      - Main Menu");

    drawButton(50, 50, 100, 30, "Back");

    glutSwapBuffers();
}

void drawMapSelect() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    
    drawBackgroundGradient();

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText((screenW - 400) / 2, screenH/2 + 100, "Select Map", 0.35f, true);

    drawButton(screenW/2 - 125, screenH/2 + 20, 250, 50, "Default Terrain");
    drawButton(screenW/2 - 125, screenH/2 - 40, 250, 50, "City Map");

    drawButton(50, 50, 120, 40, "Back");
    
    glutSwapBuffers();
}
