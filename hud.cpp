#include "hud.h"
#include "globals.h"
#include "terrain.h"
#include <GL/glut.h>
#include <cstdio>
#include <string>
#include <cmath>

// --- HUD Display ---
void renderText(float x, float y, void* font, const std::string& text) {
    glRasterPos2f(x, y);
    for (char c : text) glutBitmapCharacter(font, c);
}

namespace {

float clampf(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void drawVerticalGauge(float x, float y, float width, float height, float normalized) {
    normalized = clampf(normalized, 0.0f, 1.0f);

    glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    for (int tick = 1; tick < 5; ++tick) {
        float tickY = y + (height * tick) / 5.0f;
        glBegin(GL_LINES);
        glVertex2f(x, tickY);
        glVertex2f(x + width * 0.35f, tickY);
        glVertex2f(x + width * 0.65f, tickY);
        glVertex2f(x + width, tickY);
        glEnd();
    }

    float fillHeight = height * normalized;
    glColor4f(1.0f, 1.0f, 1.0f, 0.22f);
    glBegin(GL_QUADS);
    glVertex2f(x + 2.0f, y + 2.0f);
    glVertex2f(x + width - 2.0f, y + 2.0f);
    glVertex2f(x + width - 2.0f, y + fillHeight - 2.0f);
    glVertex2f(x + 2.0f, y + fillHeight - 2.0f);
    glEnd();

    float markerY = y + fillHeight;
    glColor4f(1.0f, 1.0f, 1.0f, 0.95f);
    glBegin(GL_LINES);
    glVertex2f(x - 6.0f, markerY);
    glVertex2f(x + width + 6.0f, markerY);
    glEnd();
}

} // namespace

void drawHUD() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f); 
    float cx = screenW / 2.0f; float cy = screenH / 2.0f;
    float agl = planeY - getVoxelHeight(planeX, planeZ);
    if (agl < 0.0f) agl = 0.0f;
    float speedGaugeX = 18.0f;
    float altitudeGaugeX = screenW - 44.0f;
    float gaugeY = cy - 150.0f;
    float gaugeW = 26.0f;
    float gaugeH = 220.0f;
    float speedNorm = clampf(currentSpeed / 920.0f, 0.0f, 1.0f);
    float altitudeNorm = clampf(planeY / 12000.0f, 0.0f, 1.0f);

    char speedBuf[32];
    char altBuf[32];
    char aglBuf[32];
    std::snprintf(speedBuf, sizeof(speedBuf), "%03d", (int)std::round(currentSpeed));
    std::snprintf(altBuf, sizeof(altBuf), "%05d", (int)std::round(planeY));
    std::snprintf(aglBuf, sizeof(aglBuf), "AGL %04d", (int)std::round(agl));

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
    renderText(speedGaugeX - 2.0f, gaugeY + gaugeH + 14.0f, GLUT_BITMAP_HELVETICA_12, "SPD");
    renderText(speedGaugeX - 6.0f, gaugeY - 18.0f, GLUT_BITMAP_HELVETICA_18, speedBuf);
    renderText(altitudeGaugeX - 2.0f, gaugeY + gaugeH + 14.0f, GLUT_BITMAP_HELVETICA_12, "ALT");
    renderText(altitudeGaugeX - 40.0f, gaugeY - 18.0f, GLUT_BITMAP_HELVETICA_18, altBuf);
    renderText(altitudeGaugeX - 48.0f, gaugeY - 34.0f, GLUT_BITMAP_HELVETICA_12, aglBuf);

    // Game time display with AM/PM
    int hour = (int)gameTime;
    int min = (int)((gameTime - hour) * 60.0f);
    int displayHour = hour % 12;
    if (displayHour == 0) displayHour = 12;
    std::string ampm = (hour < 12) ? "AM" : "PM";
    char timeBuf[32];
    std::snprintf(timeBuf, sizeof(timeBuf), "TIME: %02d:%02d %s", displayHour, min, ampm.c_str());
    renderText(screenW - 240, screenH - 30, GLUT_BITMAP_HELVETICA_18, timeBuf);
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

    drawVerticalGauge(speedGaugeX, gaugeY, gaugeW, gaugeH, speedNorm);
    drawVerticalGauge(altitudeGaugeX, gaugeY, gaugeW, gaugeH, altitudeNorm);

    glBegin(GL_LINE_LOOP); glVertex2f(50, cy - 45); glVertex2f(150, cy - 45); glVertex2f(150, cy - 55); glVertex2f(50, cy - 55); glEnd();
    glBegin(GL_QUADS); glVertex2f(50, cy - 45); glVertex2f(50 + (throttle * 100.0f), cy - 45); glVertex2f(50 + (throttle * 100.0f), cy - 55); glVertex2f(50, cy - 55); glEnd();

    if (cameraMode == 1) {
        glBegin(GL_LINES);
        glVertex2f(cx - 15, cy); glVertex2f(cx - 5, cy); glVertex2f(cx + 5, cy);  glVertex2f(cx + 15, cy);
        glVertex2f(cx, cy - 15); glVertex2f(cx, cy - 5);
        glEnd();

        glPushMatrix();
        glTranslatef(cx, cy, 0.0f); glRotatef(-roll, 0.0f, 0.0f, 1.0f); glTranslatef(0.0f, -pitch * 5.0f, 0.0f); 
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
    }

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
