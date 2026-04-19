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
    float tileSize = 64.0f; // Size of each dirt/stone block
    int cols = (int)(screenW / tileSize) + 1;
    int rows = (int)(screenH / tileSize) + 1;

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            // Pseudo-random hash for stone block variations
            unsigned int hash = (x * 73856093U ^ y * 19349663U);
            int variant = hash % 4;

            float gray = 0.35f;
            if (variant == 1) gray = 0.38f;
            else if (variant == 2) gray = 0.32f;
            else if (variant == 3) gray = 0.40f;

            glColor3f(gray, gray, gray);

            float bx = x * tileSize;
            float by = y * tileSize;

            glBegin(GL_QUADS);
            glVertex2f(bx, by);
            glVertex2f(bx + tileSize, by);
            glVertex2f(bx + tileSize, by + tileSize);
            glVertex2f(bx, by + tileSize);
            glEnd();

            // Subtle block outline
            glColor3f(gray - 0.05f, gray - 0.05f, gray - 0.05f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(bx, by);
            glVertex2f(bx + tileSize, by);
            glVertex2f(bx + tileSize, by + tileSize);
            glVertex2f(bx, by + tileSize);
            glEnd();
        }
    }

    // Moody vignette overlay
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBegin(GL_QUADS);
    glColor4f(0.05f, 0.05f, 0.05f, 0.7f); // Dark at bottom
    glVertex2f(0.0f, 0.0f);
    glVertex2f(screenW, 0.0f);
    glColor4f(0.0f, 0.0f, 0.0f, 0.2f);   // Lighter at top
    glVertex2f(screenW, screenH);
    glVertex2f(0.0f, screenH);
    glEnd();
    glDisable(GL_BLEND);
}

void drawButton(int x1, int y1, int width, int height, const char* label, bool hover = false) {
    int border = 2;
    int bevel = 3;

    // 1. Black outer border
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex2f(x1 - border, y1 - border);
    glVertex2f(x1 + width + border, y1 - border);
    glVertex2f(x1 + width + border, y1 + height + border);
    glVertex2f(x1 - border, y1 + height + border);
    glEnd();

    // Hover check
    float cBase[3] = {0.45f, 0.45f, 0.45f};
    float cHigh[3] = {0.65f, 0.65f, 0.65f};
    float cDark[3] = {0.25f, 0.25f, 0.25f};

    if (hover) {
        cBase[0] = 0.5f; cBase[1] = 0.5f; cBase[2] = 0.85f;
        cHigh[0] = 0.7f; cHigh[1] = 0.7f; cHigh[2] = 1.05f;
        cDark[0] = 0.3f; cDark[1] = 0.3f; cDark[2] = 0.5f;
    }

    // Base fill
    glColor3f(cBase[0], cBase[1], cBase[2]);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1);
    glVertex2f(x1 + width, y1);
    glVertex2f(x1 + width, y1 + height);
    glVertex2f(x1, y1 + height);
    glEnd();

    // Top Highlight (simulating 3D bevel)
    glColor3f(cHigh[0], cHigh[1], cHigh[2]);
    glBegin(GL_QUADS);
    glVertex2f(x1, y1 + height - bevel);
    glVertex2f(x1 + width, y1 + height - bevel);
    glVertex2f(x1 + width, y1 + height);
    glVertex2f(x1, y1 + height);
    // Left Highlight
    glVertex2f(x1, y1);
    glVertex2f(x1 + bevel, y1);
    glVertex2f(x1 + bevel, y1 + height);
    glVertex2f(x1, y1 + height);
    glEnd();

    // Shadow (Bottom/Right)
    glColor3f(cDark[0], cDark[1], cDark[2]);
    glBegin(GL_QUADS);
    // Bottom
    glVertex2f(x1, y1);
    glVertex2f(x1 + width, y1);
    glVertex2f(x1 + width, y1 + bevel);
    glVertex2f(x1, y1 + bevel);
    // Right
    glVertex2f(x1 + width - bevel, y1);
    glVertex2f(x1 + width, y1);
    glVertex2f(x1 + width, y1 + height);
    glVertex2f(x1 + width - bevel, y1 + height);
    glEnd();

    float textWidth = float(std::strlen(label) * 9);
    float textX = x1 + (width - textWidth) * 0.5f;
    float textY = y1 + (height - 18) * 0.5f + 4;

    // Drop shadow
    glColor3f(0.15f, 0.15f, 0.15f);
    drawText(textX + 2, textY - 2, label, GLUT_BITMAP_HELVETICA_18);
    
    if (hover) {
        glColor3f(1.0f, 1.0f, 0.6f); // Yellowish
    } else {
        glColor3f(0.85f, 0.85f, 0.85f);
    }
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

    // Calculate Exact Text Width for Auto-Centering
    float titleScale = 0.4f;
    float textWidth = 0.0f;
    const char* titleString = "Aircraft Simulator";
    for (const char* c = titleString; *c; c++) {
        textWidth += glutStrokeWidth(GLUT_STROKE_ROMAN, *c) * titleScale;
    }
    float titleX = centerX - (textWidth / 2.0f);

    // Draw Logo Drop Shadow
    glColor3f(0.15f, 0.15f, 0.15f);
    drawStrokeText(titleX + 5, startY + 75, titleString, titleScale, true);

    // Draw Main Title
    glColor3f(1.0f, 1.0f, 1.0f);
    drawStrokeText(titleX, startY + 80, titleString, titleScale, true);

    drawButton(centerX - buttonWidth / 2, startY, buttonWidth, buttonHeight, "Start Game", selectedMenuItem == 0);
    drawButton(centerX - buttonWidth / 2, startY - 70, buttonWidth, buttonHeight, "Controls", selectedMenuItem == 1);
    drawButton(centerX - buttonWidth / 2, startY - 140, buttonWidth, buttonHeight, "Select Map", selectedMenuItem == 2);
    drawButton(centerX - buttonWidth / 2, startY - 210, buttonWidth, buttonHeight, "Select Plane", selectedMenuItem == 3);
    drawButton(centerX - buttonWidth / 2, startY - 280, buttonWidth, buttonHeight, "Model Settings", selectedMenuItem == 4);

    drawButton(screenW - 130, 10, 120, 40, "Exit", selectedMenuItem == 5);

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

    drawButton(screenW/2 - 125, screenH/2 + 20, 250, 50, "Default Terrain", selectedMenuItem == 0);
    drawButton(screenW/2 - 125, screenH/2 - 40, 250, 50, "City Map", selectedMenuItem == 1);

    drawButton(50, 50, 120, 40, "Back", selectedMenuItem == 2);

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
        
        drawButton(centerX - 150, buttonY, 300, 40, g_availablePlanes[i].c_str(), selectedMenuItem == i);
    }

    drawButton(50, 50, 120, 40, "Back", selectedMenuItem == (int)g_availablePlanes.size());

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
void handleMenuNav(int d) {
    int maxItems = 0;
    if (gameState == 0) maxItems = 6;
    else if (gameState == 3) maxItems = 3;
    else if (gameState == 4) maxItems = g_availablePlanes.size() + 1;
    else return;
    
    if (maxItems == 0) return;
    selectedMenuItem += d;
    if (selectedMenuItem < 0) selectedMenuItem = maxItems - 1;
    else if (selectedMenuItem >= maxItems) selectedMenuItem = 0;
}

void handleMenuSelect() {
    if (gameState == 0) {
        if (selectedMenuItem == 0) gameState = 1;
        else if (selectedMenuItem == 1) gameState = 2;
        else if (selectedMenuItem == 2) gameState = 3;
        else if (selectedMenuItem == 3) gameState = 4;
        else if (selectedMenuItem == 4) gameState = 5;
        else if (selectedMenuItem == 5) exit(0);
        selectedMenuItem = 0;
    } else if (gameState == 2 || gameState == 5 || gameState == 1) {
        gameState = 0;
    } else if (gameState == 3) {
        if (selectedMenuItem == 0) { selectedMap = 0; }
        else if (selectedMenuItem == 1) { selectedMap = 1; }
        gameState = 0;
        selectedMenuItem = 0;
    } else if (gameState == 4) {
        if (selectedMenuItem < (int)g_availablePlanes.size()) {
            selectedPlane = selectedMenuItem + 1;
            loadSelectedPlaneModel(g_availablePlanes[selectedMenuItem]);
        }
        gameState = 0;
        selectedMenuItem = 0;
    }
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
    drawStrokeText((screenW - 200) / 2, screenH - 60, "Controls", 0.4f, true);

    int rowStart = screenH - 120;
    int step = 26;
    
    // Column 1 - Keyboard
    drawText(60, rowStart, "KEYBOARD", GLUT_BITMAP_HELVETICA_18);
    drawText(60, rowStart - step, "WASD     - Pitch / Roll");
    drawText(60, rowStart - 2*step, "Q / E    - Yaw (Rudder)");
    drawText(60, rowStart - 3*step, "+ / -    - Throttle Up/Down");
    drawText(60, rowStart - 4*step, "SPACE    - Wheel Brakes");
    drawText(60, rowStart - 5*step, "G        - Landing Gear");
    drawText(60, rowStart - 6*step, "V        - Change Camera");
    drawText(60, rowStart - 7*step, "B / M    - Back to Menu");
    drawText(60, rowStart - 8*step, "T / O    - Fast Forward Time");
    drawText(60, rowStart - 9*step, "Y        - Pause / Resume");
    drawText(60, rowStart - 10*step,"H        - Autopilot Altitude Hold");
    drawText(60, rowStart - 11*step,"L        - Auto-Land");
    drawText(60, rowStart - 12*step,"P        - Full Reset / Respawn");
    drawText(60, rowStart - 13*step,"K        - Cycle Weather Mode");
    drawText(60, rowStart - 14*step,"9        - Spawn Twin Towers");
    drawText(60, rowStart - 15*step,"ESC      - Main Menu");

    // Column 2 - Gamepad
    int col2 = screenW / 2 + 50;
    drawText(col2, rowStart, "GAMEPAD (XBOX)", GLUT_BITMAP_HELVETICA_18);
    drawText(col2, rowStart - step, "L / R Triggers - Throttle Down/Up");
    drawText(col2, rowStart - 2*step, "Left Stick X   - Roll (Left/Right)");
    drawText(col2, rowStart - 3*step, "Right Stick Y  - Pitch (Up/Down)");
    drawText(col2, rowStart - 4*step, "LB / RB        - Yaw (Left/Right)");
    drawText(col2, rowStart - 5*step, "D-Pad Down/L3  - Landing Gear Toggle");
    drawText(col2, rowStart - 6*step, "D-Pad Up       - Autopilot Toggle (Alt Hold)");
    drawText(col2, rowStart - 7*step, "A Button       - Change Camera");
    drawText(col2, rowStart - 8*step, "B Button       - Back to Menu");
    drawText(col2, rowStart - 9*step, "X Button       - Auto-Land");
    drawText(col2, rowStart - 10*step,"Y Button       - Spawn Twin Towers");
    drawText(col2, rowStart - 11*step,"Start          - Pause / Resume");
    drawText(col2, rowStart - 12*step,"Back / R3      - Full Reset / Respawn");

    drawButton(50, 50, 100, 30, "Back", true);

    glutSwapBuffers();
}
