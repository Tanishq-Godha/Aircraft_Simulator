#include "hud.h"
#include "globals.h"
#include "terrain.h"
#include "math_utils.h"
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

void drawHorizontalBar(float x, float y, float w, float h,
                       float normalized,
                       float r, float g, float b) {
    normalized = clampf(normalized, 0.0f, 1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y); glVertex2f(x+w, y);
    glVertex2f(x+w, y+h); glVertex2f(x, y+h);
    glEnd();

    glColor4f(r, g, b, 0.75f);
    glBegin(GL_QUADS);
    glVertex2f(x+1,           y+1);
    glVertex2f(x + w*normalized - 1, y+1);
    glVertex2f(x + w*normalized - 1, y+h-1);
    glVertex2f(x+1,           y+h-1);
    glEnd();
}

} // namespace

void drawHUD() {
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0, screenW, 0, screenH); glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();

    float cx = screenW / 2.0f; 
    float cy = screenH / 2.0f;

    // ── 1. Top Header Bar ──────────────────────────────────────────────────
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(0, screenH); glVertex2f(screenW, screenH);
    glVertex2f(screenW, screenH - 45); glVertex2f(0, screenH - 45);
    glEnd();
    
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    
    // View Select
    std::string viewText = "VIEW: ";
    switch(cameraMode) {
        case 0: viewText += "CHASE"; break;
        case 1: viewText += "COCKPIT"; break;
        case 2: viewText += "CINEMATIC"; break;
        case 3: viewText += "FLY-BY"; break;
    }
    renderText(25, screenH - 30, GLUT_BITMAP_HELVETICA_18, viewText);

    // State centered
    std::string flightState = "STATE: ";
    if (crashed || isExploding)    flightState += "CRASHED";
    else if (isBellyLanding) flightState += "BELLY SLIDE";
    else if (isGrounded) flightState += "ROLLING";
    else            flightState += "AIRBORNE";
    renderText(cx - 70, screenH - 30, GLUT_BITMAP_HELVETICA_18, flightState);

    // Time on right
    int hour = (int)gameTime;
    int min  = (int)((gameTime - hour) * 60.0f);
    int displayHour = hour % 12;
    if (displayHour == 0) displayHour = 12;
    std::string ampm = (hour < 12) ? "AM" : "PM";
    char timeBuf[64];
    if (timeScale > 1.1f)
        std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %s [%dX]", displayHour, min, ampm.c_str(), (int)timeScale);
    else
        std::snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d %s", displayHour, min, ampm.c_str());
    renderText(screenW - 160, screenH - 30, GLUT_BITMAP_HELVETICA_18, timeBuf);

    // ── 2. Gauges (Sides) ─────────────────────────────────────────────────────
    float speedGaugeX   = 35.0f;
    float altitudeGaugeX = screenW - 61.0f;
    float gaugeY = cy - 110.0f;
    float gaugeW = 26.0f;
    float gaugeH = 220.0f;

    drawVerticalGauge(speedGaugeX,    gaugeY, gaugeW, gaugeH, clampf(currentSpeed / 920.0f, 0.0f, 1.0f));
    drawVerticalGauge(altitudeGaugeX, gaugeY, gaugeW, gaugeH, clampf(planeY / 12000.0f, 0.0f, 1.0f));

    char speedBuf[16], altBuf[16];
    std::snprintf(speedBuf, sizeof(speedBuf), "%d", (int)std::round(currentSpeed));
    std::snprintf(altBuf,   sizeof(altBuf),   "%d", (int)std::round(planeY));

    // SPD Label
    glColor3f(1, 1, 1);
    renderText(speedGaugeX, gaugeY + gaugeH + 10, GLUT_BITMAP_HELVETICA_12, "SPD");
    renderText(speedGaugeX - 5, gaugeY - 25, GLUT_BITMAP_HELVETICA_18, speedBuf);

    // ALT Label
    renderText(altitudeGaugeX, gaugeY + gaugeH + 10, GLUT_BITMAP_HELVETICA_12, "ALT");
    renderText(altitudeGaugeX - 10, gaugeY - 25, GLUT_BITMAP_HELVETICA_18, altBuf);
    
    float agl = planeY - getSceneHeight(planeX, planeZ);
    char aglBuf[32];
    std::snprintf(aglBuf, sizeof(aglBuf), "AGL %d", (int)std::max(0.0f, std::round(agl)));
    renderText(altitudeGaugeX - 25, gaugeY - 45, GLUT_BITMAP_HELVETICA_12, aglBuf);

    // ── 3. System Status (Bottom Left) ─────────────────────────────────────────
    float sysX = 35.0f;
    float sysY = 50.0f;
    
    // Throttle
    renderText(sysX, sysY + 65, GLUT_BITMAP_HELVETICA_12, "THR");
    drawHorizontalBar(sysX + 40, sysY + 65, 120.0f, 12.0f, throttle, 1.0f, 1.0f, 1.0f);
    if (afterburnerIntensity > 0.05f) {
        float ab = afterburnerIntensity;
        glColor4f(1.0f, 0.6f * ab, 0.0f, ab);
        renderText(sysX + 165, sysY + 65, GLUT_BITMAP_HELVETICA_12, "AB");
        glColor4f(1,1,1,1);
    }

    // Gear
    std::string gearText = "GEAR: ";
    if (gearInTransition) {
        gearText += (gearDeployed ? "DEPLOYING" : "RETRACTING");
    } else {
        gearText += ((gearAnimation > 0.95f) ? "DOWN" : "UP");
    }
    renderText(sysX, sysY + 25, GLUT_BITMAP_HELVETICA_12, gearText);

    // Controls tip
    glColor4f(1, 1, 1, 0.6f);
    renderText(sysX, sysY, GLUT_BITMAP_HELVETICA_10, "SPEED CTRL: R/F OR UP/DOWN");
    glColor4f(1, 1, 1, 1);

    // ── 4. Indicators / Warnings (Center) ──────────────────────────────────────
    if (autopilotOn && !autoLandOn) {
        char apBuf[48];
        std::snprintf(apBuf, sizeof(apBuf), "AP ACTIVE: ALT %d", (int)std::round(autopilotAlt));
        glColor3f(0.2f, 1.0f, 0.4f);
        renderText(cx - 85, screenH - 75, GLUT_BITMAP_HELVETICA_18, apBuf);
    }

    if (autoLandOn) {
        glColor3f(1.0f, 0.3f, 0.3f);
        renderText(cx - 95, screenH - 75, GLUT_BITMAP_HELVETICA_18, "AUTO-LAND: GLIDING/BRAKING");
    } else if (autoLandFailTimer > 0.0f) {
        glColor3f(1.0f, 0.2f, 0.0f);
        std::string msg = "AUTO-LAND UNAVAILABLE: " + autoLandFailReason;
        renderText(cx - 150, screenH - 75, GLUT_BITMAP_HELVETICA_18, msg);
    }

    if (gearAnimation < 0.95f && agl < 500.0f && !isGrounded) {
        glColor3f(1, 0, 0);
        renderText(cx - 65, cy + 140, GLUT_BITMAP_HELVETICA_18, "CHECK GEAR!");
    }

    if (engineOut) {
        glColor3f(1.0f, 0.1f, 0.1f);
        renderText(cx - 70, cy + 170, GLUT_BITMAP_HELVETICA_18, "ENGINE OUT");
    }

    // Crosshair / Flight Director
    if (cameraMode == 1) {
        glColor4f(0.2f, 1.0f, 0.2f, 0.8f);
        glBegin(GL_LINES);
        glVertex2f(cx - 20, cy); glVertex2f(cx - 7, cy);
        glVertex2f(cx +  7, cy); glVertex2f(cx + 20, cy);
        glVertex2f(cx, cy - 20); glVertex2f(cx, cy - 7);
        glVertex2f(cx, cy +  7); glVertex2f(cx, cy + 20);
        glEnd();

        glPushMatrix();
        glTranslatef(cx, cy, 0.0f);
        glRotatef(-roll, 0.0f, 0.0f, 1.0f);
        glTranslatef(0.0f, -pitch * 6.0f, 0.0f);
        glBegin(GL_LINES);
        glVertex2f(-120, 0); glVertex2f(-40, 0);
        glVertex2f(  40, 0); glVertex2f(120, 0);
        for (int i = -90; i <= 90; i += 15) {
            if (i == 0) continue;
            float yOff = i * 6.0f;
            glVertex2f(-60, yOff); glVertex2f(-40, yOff);
            glVertex2f( 40, yOff); glVertex2f( 60, yOff);
        }
        glEnd();
        glPopMatrix();
    }

    if (isStalling) {
        glColor3f(1.0f, 0.0f, 0.0f);
        renderText(cx - 45, cy + 100, GLUT_BITMAP_TIMES_ROMAN_24, "STALL!");
    }

    if (crashed) {
        glColor4f(0, 0, 0, 0.6f);
        glBegin(GL_QUADS);
        glVertex2f(cx-110, cy+60); glVertex2f(cx+110, cy+60);
        glVertex2f(cx+110, cy+95); glVertex2f(cx-110, cy+95);
        glEnd();
        glColor3f(1.0f, 0.2f, 0.2f);
        renderText(cx - 160, cy + 70, GLUT_BITMAP_HELVETICA_18, "CRASHED - PRESS P (OR R3 / BACK BUTTON)");
    }

    // ── PAUSE overlay ────────────────────────────────────────────────────────────
    if (isPaused) {
        glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(screenW, 0);
        glVertex2f(screenW, screenH); glVertex2f(0, screenH);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        renderText(cx - 58, cy + 30, GLUT_BITMAP_TIMES_ROMAN_24, "PAUSED");
        renderText(cx - 95, cy - 10, GLUT_BITMAP_HELVETICA_18, "Press Y to Resume");
        renderText(cx - 110, cy - 35, GLUT_BITMAP_HELVETICA_12, "M = Menu   ESC = Menu   P = Reset");
    }

    // Screen fade (landing cinematic)
    if (screenFade > 0.01f) {
        glDisable(GL_TEXTURE_2D);
        glColor4f(0.0f, 0.0f, 0.0f, clampf(screenFade, 0.0f, 1.0f));
        glBegin(GL_QUADS);
        glVertex2f(0, 0); glVertex2f(screenW, 0);
        glVertex2f(screenW, screenH); glVertex2f(0, screenH);
        glEnd();
    }

    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
}