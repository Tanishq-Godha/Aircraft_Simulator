#include <GL/glut.h>
#include <cstring>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "globals.h"
#include "terrain.h"
#include "jet.h"
#include "hud.h"
#include "physics.h"
#include "camera.h"
#include "input.h"
#include "init.h"

void reshape(int w, int h) {
    screenW = w;
    screenH = h;
    glViewport(0,0,w,h);
}

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

    // button border/background
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

void mouseClick(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        int mouseY = screenH - y; // Convert to ortho coordinates
        
        if (gameState == 0) { // Menu
            int buttonWidth = 250;
            int buttonHeight = 50;
            int startY = screenH / 2 + 20;
            
            // Start Game button
            if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                mouseY >= startY && mouseY <= startY + buttonHeight) {
                gameState = 1;
            }
            // Controls button
            else if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                     mouseY >= startY - 70 && mouseY <= startY - 20) {
                gameState = 2;
            }
            // Select Map button
            else if (x >= screenW/2 - buttonWidth/2 && x <= screenW/2 + buttonWidth/2 &&
                     mouseY >= startY - 140 && mouseY <= startY - 90) {
                gameState = 3;
            }
            // Exit button
            else if (x >= screenW - 130 && x <= screenW - 10 && mouseY >= 10 && mouseY <= 50) {
                exit(0);
            }
        } else if (gameState == 2) { // Controls
            if (x >= 50 && x <= 150 && mouseY >= 50 && mouseY <= 80) {
                gameState = 0; // Back button
            }
        } else if (gameState == 3) { // Map select
            // Default Terrain button
            if (x >= screenW/2 - 125 && x <= screenW/2 + 125 &&
                mouseY >= screenH/2 + 20 && mouseY <= screenH/2 + 70) {
                selectedMap = 0;
                gameState = 0;
            }
            // City Map button
            else if (x >= screenW/2 - 125 && x <= screenW/2 + 125 &&
                     mouseY >= screenH/2 - 40 && mouseY <= screenH/2 + 10) {
                selectedMap = 1;
                gameState = 0;
            }
            // Back button
            else if (x >= 50 && x <= 170 && mouseY >= 50 && mouseY <= 90) {
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
    int step = 30;
    drawText(60, rowStart, "WASD - Turn");
    drawText(60, rowStart - step, "+/- - Throttle");
    drawText(60, rowStart - 2*step, "G - Landing Gear");
    drawText(60, rowStart - 3*step, "[/] - Flaps");
    drawText(60, rowStart - 4*step, "V - Camera Mode");
    drawText(60, rowStart - 5*step, "P - Reset");
    drawText(60, rowStart - 6*step, "M - Return to Menu");
    drawText(60, rowStart - 7*step, "ESC - Exit");

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

void drawSky() {
    glDisable(GL_LIGHTING); // Sky objects don't need lighting

    if (gameTime >= 6.0f && gameTime <= 18.0f) {
        // Daytime: sun and clouds
        float t = (gameTime - 6.0f) / 12.0f;
        float sunElev = std::sin(t * M_PI);
        float sunAzim = t * M_PI;
        float sunX = 10000.0f * std::cos(sunElev) * std::sin(sunAzim);
        float sunY = 10000.0f * std::sin(sunElev);
        float sunZ = 10000.0f * std::cos(sunElev) * std::cos(sunAzim);

        // Draw sun
        glPushMatrix();
        glTranslatef(sunX, sunY, sunZ);
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow
        glutSolidSphere(200.0f, 20, 20);
        glPopMatrix();

        // Draw clouds (cluster of spheres for each cloud, higher density during day)
        glColor3f(1.0f, 1.0f, 1.0f);

        int cloudCount = 12;
        for (int c = 0; c < cloudCount; c++) {
            float baseX = (c - cloudCount/2) * 1800.0f + std::fmod(c * 330.0f, 800.0f);
            float baseY = 2800.0f + (c % 3) * 450.0f;
            float baseZ = std::fmod(c * 2200.0f, 10000.0f) - 5000.0f;
            int clusterSize = 5 + (c % 4);
            for (int j = 0; j < clusterSize; j++) {
                float ox = baseX + (j - clusterSize/2) * 220.0f + std::fmod(c * j * 19.0f, 160.0f);
                float oy = baseY + std::fmod(j * 60.0f, 100.0f);
                float oz = baseZ + std::fmod(j * 120.0f, 250.0f);
                glPushMatrix();
                glTranslatef(ox, oy, oz);
                glutSolidSphere(280.0f + std::fmod(j * 12.0f, 40.0f), 12, 12);
                glPopMatrix();
            }
        }
    } else {
        // Nighttime: moon and stars
        float t = (gameTime < 6.0f ? gameTime + 18.0f : gameTime - 6.0f) / 12.0f;
        float moonElev = sin(t * M_PI);
        float moonAzim = t * M_PI;
        float moonX = 5000.0f * cos(moonElev) * sin(moonAzim);
        float moonY = 5000.0f * sin(moonElev);
        float moonZ = 5000.0f * cos(moonElev) * cos(moonAzim);

        // Draw moon
        glPushMatrix();
        glTranslatef(moonX, moonY, moonZ);
        glColor3f(0.9f, 0.9f, 1.0f); // Light blue-white
        glutSolidSphere(150.0f, 20, 20);
        glPopMatrix();

        // Draw stars
        glPointSize(2.0f);
        glBegin(GL_POINTS);
        glColor3f(1.0f, 1.0f, 1.0f);
        for (int i = 0; i < 50; i++) {
            float x = std::fmod(i * 137.5f, 20000.0f) - 10000.0f;
            float y = 8000.0f + std::fmod(i * 73.0f, 2000.0f);
            float z = std::fmod(i * 211.0f, 20000.0f) - 10000.0f;
            glVertex3f(x, y, z);
        }
        glEnd();
    }

    glEnable(GL_LIGHTING);
}

void display() {
    if (gameState == 0) {
        drawMenu();
    } else if (gameState == 1) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        // 1️⃣ Apply camera transform
        setupCamera();

        // 2️⃣ Dynamic lighting based on day/night cycle
        float sunElev, sunAzim;
        if (gameTime >= 6.0f && gameTime <= 18.0f) {
            // Daytime: sun
            float t = (gameTime - 6.0f) / 12.0f; // 0 to 1
            sunElev = sin(t * M_PI);
            sunAzim = t * M_PI; // 0 to pi
            GLfloat light_pos[] = {static_cast<GLfloat>(10000.0f * std::cos(sunElev) * std::sin(sunAzim)), 
                                   static_cast<GLfloat>(10000.0f * std::sin(sunElev)), 
                                   static_cast<GLfloat>(10000.0f * std::cos(sunElev) * std::cos(sunAzim)), 1.0f};
            GLfloat light_diffuse[] = {1.0f, 0.98f, 0.95f, 1.0f};
            glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
            GLfloat ambient[] = {0.3f, 0.3f, 0.3f, 1.0f};
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        } else {
            // Nighttime: moon
            float t = (gameTime < 6.0f ? gameTime + 18.0f : gameTime - 6.0f) / 12.0f;
            sunElev = sin(t * M_PI);
            sunAzim = t * M_PI;
            GLfloat light_pos[] = {static_cast<GLfloat>(5000.0f * std::cos(sunElev) * std::sin(sunAzim)), 
                                   static_cast<GLfloat>(5000.0f * std::sin(sunElev)), 
                                   static_cast<GLfloat>(5000.0f * std::cos(sunElev) * std::cos(sunAzim)), 1.0f};
            GLfloat light_diffuse[] = {0.2f, 0.2f, 0.3f, 1.0f}; // Dim blue
            glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
            glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
            GLfloat ambient[] = {0.05f, 0.05f, 0.1f, 1.0f};
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        }

        glEnable(GL_LIGHT0);
        glEnable(GL_LIGHTING);

        // 3️⃣ Draw scene
        drawVoxelTerrain();
        drawDetailedJet();
        drawHUD();

        // 4️⃣ Draw sky objects
        drawSky();

        glutSwapBuffers();
    } else if (gameState == 2) {
        drawControls();
    } else if (gameState == 3) {
        drawMapSelect();
    }
}

int main(int argc, char** argv) {

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB |
                        GLUT_DEPTH | GLUT_MULTISAMPLE);

    glutInitWindowSize(screenW, screenH);
    glutCreateWindow("Voxel Flight Modular");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(updatePhysics);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutSpecialFunc(specialKeyDown);
    glutSpecialUpFunc(specialKeyUp);
    glutMouseFunc(mouseClick);

    glutMainLoop();
}
