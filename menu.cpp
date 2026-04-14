#include "menu.h"
#include "globals.h"
#include "math_utils.h"
#include "model_loader.h"
#include "jet.h"
#include "shader_loader.h"
#include <GL/glut.h>
#include <cstring>
#include <cstdlib>
#include <vector>

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

}  // namespace

// Global available planes list
static std::vector<std::string> g_availablePlanes;
static bool g_planesInitialized = false;

namespace {

} // namespace

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int mouseY = screenH - y;
        
        if (gameState == 0) { // Main menu
            int buttonWidth = 250;
            int buttonHeight = 50;
            int centerX = screenW / 2;
            int startY = screenH / 2 + 40;
            
            if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY, buttonWidth, buttonHeight)) {
                gameState = 1;  // Start game
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 70, buttonWidth, buttonHeight)) {
                gameState = 2;  // Controls
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 140, buttonWidth, buttonHeight)) {
                gameState = 3;  // Map select
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 210, buttonWidth, buttonHeight)) {
                // Initialize planes list on first access
                if (!g_planesInitialized) {
                    getAvailablePlanes(g_availablePlanes);
                    g_planesInitialized = true;
                }
                gameState = 4;  // Plane select
            }
            else if (pointInRect(x, mouseY, centerX - buttonWidth/2, startY - 280, buttonWidth, buttonHeight)) {
                if (!g_planesInitialized) {
                    getAvailablePlanes(g_availablePlanes);
                    g_planesInitialized = true;
                }
                // Ensure a plane is loaded so we can view it
                if (selectedPlane > 0 && selectedPlane < g_availablePlanes.size()) {
                    loadSelectedPlaneModel(g_availablePlanes[selectedPlane]);
                }
                gameState = 5;  // Model Viewer/Settings
            }
            else if (pointInRect(x, mouseY, screenW - 130, 10, 120, 40)) {
                exit(0);  // Exit
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
        } else if (gameState == 4) { // Plane select
            int centerX = screenW / 2;
            int startY = screenH / 2 + 60;
            int itemHeight = 45;
            
            // Check each plane option
            for (size_t i = 0; i < g_availablePlanes.size() && i < 10; ++i) {
                int buttonY = startY - (int)(i * itemHeight);
                if (pointInRect(x, mouseY, centerX - 150, buttonY, 300, 40)) {
                    selectedPlane = i;
                    
                    // Load the selected plane if not DEFAULT
                    if (i > 0 && i < g_availablePlanes.size()) {
                        loadSelectedPlaneModel(g_availablePlanes[i]);
                    } else {
                        // Default plane
                        unloadJetModel();
                    }
                    
                    gameState = 0;
                    return;
                }
            }
            
            // Back button
            if (pointInRect(x, mouseY, 50, 50, 120, 40)) {
                gameState = 0;
            }
        } else if (gameState == 5) { // Model Settings
            int btnW = 50;
            int btnH = 30;
            int bx1 = screenW / 2 + 100, bx2 = screenW / 2 + 160;
            int rowY = screenH - 120;
            
            // Scale
            if (pointInRect(x, mouseY, bx1, rowY, btnW, btnH)) modelGlobalScale -= 0.1f;
            if (pointInRect(x, mouseY, bx2, rowY, btnW, btnH)) modelGlobalScale += 0.1f;
            
            // Rot X
            rowY -= 50;
            if (pointInRect(x, mouseY, bx1, rowY, btnW, btnH)) modelGlobalRotX -= 5.0f;
            if (pointInRect(x, mouseY, bx2, rowY, btnW, btnH)) modelGlobalRotX += 5.0f;

            // Rot Y
            rowY -= 50;
            if (pointInRect(x, mouseY, bx1, rowY, btnW, btnH)) modelGlobalRotY -= 5.0f;
            if (pointInRect(x, mouseY, bx2, rowY, btnW, btnH)) modelGlobalRotY += 5.0f;

            // Rot Z
            rowY -= 50;
            if (pointInRect(x, mouseY, bx1, rowY, btnW, btnH)) modelGlobalRotZ -= 5.0f;
            if (pointInRect(x, mouseY, bx2, rowY, btnW, btnH)) modelGlobalRotZ += 5.0f;
            
            if (modelGlobalScale < 0.1f) modelGlobalScale = 0.1f;
            
            if (pointInRect(x, mouseY, 50, 50, 120, 40)) {
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
    int startY = screenH / 2 + 40;

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(centerX - 250, startY + 80, "Aircraft Simulator", 0.4f, true);

    drawButton(centerX - buttonWidth / 2, startY, buttonWidth, buttonHeight, "Start Game");
    drawButton(centerX - buttonWidth / 2, startY - 70, buttonWidth, buttonHeight, "Controls");
    drawButton(centerX - buttonWidth / 2, startY - 140, buttonWidth, buttonHeight, "Select Map");
    drawButton(centerX - buttonWidth / 2, startY - 210, buttonWidth, buttonHeight, "Select Plane");
    drawButton(centerX - buttonWidth / 2, startY - 280, buttonWidth, buttonHeight, "Model Settings");

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

void drawPlaneSelect() {
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
    drawStrokeText((screenW - 300) / 2, screenH - 60, "Select Aircraft", 0.35f, true);

    int centerX = screenW / 2;
    int startY = screenH / 2 + 60;
    int itemHeight = 45;

    // Draw available planes
    for (size_t i = 0; i < g_availablePlanes.size() && i < 10; ++i) {
        int buttonY = startY - (int)(i * itemHeight);
        
        // Highlight selected plane
        bool isSelected = (selectedPlane == (int)i);
        if (isSelected) {
            glColor3f(0.3f, 0.5f, 0.8f);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBegin(GL_QUADS);
            glVertex2f(centerX - 150, buttonY);
            glVertex2f(centerX + 150, buttonY);
            glVertex2f(centerX + 150, buttonY + 40);
            glVertex2f(centerX - 150, buttonY + 40);
            glEnd();
            glDisable(GL_BLEND);
        }
        
        drawButton(centerX - 150, buttonY, 300, 40, g_availablePlanes[i].c_str());
    }

    drawButton(50, 50, 120, 40, "Back");
    
    glutSwapBuffers();
}

void drawModelSettings() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // --- 3D Model Rendering (FIRST) ---
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    
    GLfloat lightPos[] = { 10.0f, 20.0f, 30.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (double)screenW / (double)screenH, 1.0, 1000.0);
    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.0, 10.0, 40.0,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);
              
    // Center the model in the lower part of the screen
    glTranslatef(0.0f, -5.0f, 0.0f);
    
    // Apply user settings
    glScalef(modelGlobalScale, modelGlobalScale, modelGlobalScale);
    glRotatef(modelGlobalRotX, 1.0f, 0.0f, 0.0f);
    glRotatef(modelGlobalRotY, 0.0f, 1.0f, 0.0f);
    glRotatef(modelGlobalRotZ, 0.0f, 0.0f, 1.0f);
    
    // Rotate slowly over time
    float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    glRotatef(time * 30.0f, 0.0f, 1.0f, 0.0f);

    if (g_useLoadedModel && g_loadedJetModel.scene) {
        glUseProgram(0);
        drawLoadedModel(g_loadedJetModel);
    } else {
        drawDetailedJet();
    }
    
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    
    // --- 2D UI Background & Buttons (SECOND, ON TOP) ---
    // Clear the depth buffer so 2D UI isn't blocked by the 3D model
    glClear(GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG); // Ensure fog doesn't hide the UI
    
    // We will NOT draw background gradient so we can see the 3D model underneath!
    // Just draw text overlays and buttons.
    
    // Draw a subtle translucent box for the UI text for readability
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    int panelW = 400;
    int panelH = 260;
    int px = screenW / 2 - 220;
    int py = screenH - 280;
    glVertex2f(px, py);
    glVertex2f(px + panelW, py);
    glVertex2f(px + panelW, py + panelH);
    glVertex2f(px, py + panelH);
    glEnd();
    glDisable(GL_BLEND);

    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(screenW / 2 - 150, screenH - 60, "Model Settings", 0.35f, true);

    int btnW = 50;
    int btnH = 30;
    int bx1 = screenW / 2 + 100, bx2 = screenW / 2 + 160;
    
    int rowY = screenH - 120;
    // Scale row
    drawText(screenW / 2 - 200, rowY + 5, "Scale factor:");
    char buffer[64];
    sprintf(buffer, "%.2f", modelGlobalScale);
    drawText(screenW / 2 - 50, rowY + 5, buffer);
    drawButton(bx1, rowY, btnW, btnH, "-");
    drawButton(bx2, rowY, btnW, btnH, "+");

    rowY -= 50;
    drawText(screenW / 2 - 200, rowY + 5, "Rot X (Pitch):");
    sprintf(buffer, "%.0f", modelGlobalRotX);
    drawText(screenW / 2 - 50, rowY + 5, buffer);
    drawButton(bx1, rowY, btnW, btnH, "-");
    drawButton(bx2, rowY, btnW, btnH, "+");

    rowY -= 50;
    drawText(screenW / 2 - 200, rowY + 5, "Rot Y (Yaw):");
    sprintf(buffer, "%.0f", modelGlobalRotY);
    drawText(screenW / 2 - 50, rowY + 5, buffer);
    drawButton(bx1, rowY, btnW, btnH, "-");
    drawButton(bx2, rowY, btnW, btnH, "+");

    rowY -= 50;
    drawText(screenW / 2 - 200, rowY + 5, "Rot Z (Roll):");
    sprintf(buffer, "%.0f", modelGlobalRotZ);
    drawText(screenW / 2 - 50, rowY + 5, buffer);
    drawButton(bx1, rowY, btnW, btnH, "-");
    drawButton(bx2, rowY, btnW, btnH, "+");

    drawButton(50, 50, 120, 40, "Back");

    glutSwapBuffers();
}
