#include "jet.h"
#include "globals.h"
#include <GL/glut.h>

#include "jet.h"
#include "globals.h"
#include <GL/glut.h>

#include "jet.h"
#include "globals.h"
#include <GL/glut.h>

#include "jet.h"
#include "globals.h"
#include <GL/glut.h>
#include <math.h>

void drawSideWindowsAndLogo() {
    // --- Window Color (Dark Glass) ---
    GLfloat mat_window[] = { 0.05f, 0.05f, 0.1f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_window);
    glColor3f(0.05f, 0.05f, 0.1f);

    // Draw row of windows on both sides
    for (float z = -2.5f; z < 5.5f; z += 0.4f) {
        // Skip windows where the wing connects (approx z = 0 to 2) 
        // if you want a more realistic look, otherwise leave it
        
        glPushMatrix();
        // Right side windows
        glBegin(GL_QUADS);
            glNormal3f(1.0f, 0.0f, 0.0f);
            glVertex3f(0.81f, 0.15f, z);       glVertex3f(0.81f, 0.25f, z);
            glVertex3f(0.81f, 0.25f, z + 0.2f); glVertex3f(0.81f, 0.15f, z + 0.2f);
        glEnd();

        // Left side windows
        glBegin(GL_QUADS);
            glNormal3f(-1.0f, 0.0f, 0.0f);
            glVertex3f(-0.81f, 0.15f, z);       glVertex3f(-0.81f, 0.25f, z);
            glVertex3f(-0.81f, 0.25f, z + 0.2f); glVertex3f(-0.81f, 0.15f, z + 0.2f);
        glEnd();
        glPopMatrix();
    }

    // // --- AIR INDIA Side Text (Simplified as a red stripe/block) ---
    // GLfloat mat_red[] = { 0.85f, 0.10f, 0.15f, 1.0f };
    // glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_red);
    // glColor3f(0.85f, 0.10f, 0.15f);

    // ======================================
// LARGE "AIR INDIA" ON RIGHT SIDE
// ======================================

    GLfloat mat_red[] = { 0.85f, 0.10f, 0.15f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_red);
    glColor3f(0.85f, 0.10f, 0.15f);

    // Slightly outside fuselage radius (~0.8)
    float x = 0.83f;

   glPushMatrix();

    // ==========================
    // NEW POSITION (CENTER BODY)
    // ==========================

    // Right side of fuselage (radius ≈ 0.8)
    // glTranslatef(0.83f, 0.55f, 0.5f);
    glTranslatef(0.83f, 0.0f, 3.0f);

    // Face outward correctly
    glRotatef(90, 0, 1, 0);

    // MUCH LARGER airline text
    glScalef(0.0075f, 0.0075f, 0.0075f);

    const char* text = "AIR INDIA";
    for (const char* p = text; *p; p++) {
        glLineWidth(1.35f);
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
        glLineWidth(1.0f);
    }

    glPopMatrix();

    // Left Side Logo Block
    glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);
        glVertex3f(-0.82f, 0.35f, -2.5f); glVertex3f(-0.82f, 0.55f, -2.5f);
        glVertex3f(-0.82f, 0.55f, 0.5f);  glVertex3f(-0.82f, 0.35f, 0.5f);
    glEnd();
}


void drawCockpitView() {
    // 1. Black Mask (The Raccoon Paint)
    GLfloat mat_black[] = { 0.02f, 0.02f, 0.02f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_black);
    glColor3f(0.02f, 0.02f, 0.02f);

    glPushMatrix();
    // Move it to the upper front of the nose
    glTranslatef(0.0f, 0.38f, -4.45f); 
    glRotatef(18.0f, 1.0f, 0.0f, 0.0f);
    
    // Draw the dark mask slightly larger than windows
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 1.0f, -0.5f);
        glVertex3f(-0.55f, 0.12f, 0.0f); glVertex3f(0.55f, 0.12f, 0.0f);
        glVertex3f(0.5f, -0.12f, 0.15f); glVertex3f(-0.5f, -0.12f, 0.15f);
    glEnd();

    // 2. Window Glass (Lighter blue-grey to stand out against the black mask)
    GLfloat mat_glass[] = { 0.3f, 0.4f, 0.5f, 1.0f };
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_glass);
    glColor3f(0.3f, 0.4f, 0.5f);

    // Individual Panels (Offset Z slightly so they don't flicker against the mask)
    glTranslatef(0.0f, 0.0f, -0.01f);
    glBegin(GL_QUADS);
        // Left Windows
        glVertex3f(-0.52f, 0.08f, 0.02f); glVertex3f(-0.28f, 0.1f, 0.01f);
        glVertex3f(-0.28f, -0.08f, 0.12f); glVertex3f(-0.52f, -0.08f, 0.12f);
        // Center Windows
        glVertex3f(-0.25f, 0.1f, 0.01f); glVertex3f(0.25f, 0.1f, 0.01f);
        glVertex3f(0.25f, -0.08f, 0.12f); glVertex3f(-0.25f, -0.08f, 0.12f);
        // Right Windows
        glVertex3f(0.28f, 0.1f, 0.01f); glVertex3f(0.52f, 0.08f, 0.02f);
        glVertex3f(0.52f, -0.08f, 0.12f); glVertex3f(0.28f, -0.08f, 0.12f);
    glEnd();
    glPopMatrix();
}

void drawDetailedJet() {
    if (cameraMode == 1) return; 

    glPushMatrix();
    glTranslatef(planeX, planeY, planeZ);
    glRotatef(yaw, 0, 1, 0); 
    glRotatef(pitch, 1, 0, 0); 
    glRotatef(roll, 0, 0, 1);

    // --- AIR INDIA COLOR PALETTE ---
    GLfloat mat_white[]  = { 0.95f, 0.95f, 0.95f, 1.0f };
    GLfloat mat_red[]    = { 0.85f, 0.10f, 0.15f, 1.0f }; 
    GLfloat mat_gold[]   = { 0.85f, 0.65f, 0.15f, 1.0f }; 
    GLfloat mat_grey[]   = { 0.70f, 0.70f, 0.75f, 1.0f }; 
    GLfloat mat_dark[]   = { 0.15f, 0.15f, 0.15f, 1.0f }; 

    // ==========================================
    // 1. FUSELAGE (Smooth Aerodynamic Profile)
    // ==========================================
    glColor3f(0.95f, 0.95f, 0.95f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_white);
    
    // Main Cabin Cylinder (z = -3.0 to 6.0)
    glPushMatrix(); 
    glTranslatef(0.0f, 0.0f, -3.0f); 
    gluCylinder(quadric, 0.8f, 0.8f, 9.0f, 48, 1); 
    glPopMatrix();

    // PROPER NOSE CONE (Uses a stretched sphere instead of a sharp cylinder)
    // This perfectly matches the z=-3.0 edge of the cylinder and curves smoothly to z=-5.0
    glPushMatrix(); 
    glTranslatef(0.0f, 0.0f, -3.0f); 
    glScalef(1.0f, 1.0f, 2.5f); // Stretch the Z-axis by 2.5x
    // We only want the front half of the sphere, so we clip it
    double eqn[4] = {0.0, 0.0, -1.0, 0.0}; 
    glClipPlane(GL_CLIP_PLANE0, eqn);
    glEnable(GL_CLIP_PLANE0);
    gluSphere(quadric, 0.8f, 48, 48); 
    glDisable(GL_CLIP_PLANE0);
    glPopMatrix();

    // Tail Cone (z = 6.0 to 9.5)
    glPushMatrix(); 
    glTranslatef(0.0f, 0.0f, 6.0f); 
    gluCylinder(quadric, 0.8f, 0.2f, 3.5f, 48, 1); 
    glPopMatrix();
    
    // Tail Cap
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 9.5f);
    gluSphere(quadric, 0.2f, 16, 16);
    glPopMatrix();

    // --- LIVERY: Red Underbelly ---
    glColor3f(0.85f, 0.10f, 0.15f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_red);
    glPushMatrix();
    glTranslatef(0.0f, -0.2f, -2.8f);
    glScalef(0.98f, 0.78f, 1.0f); 
    gluCylinder(quadric, 0.8f, 0.8f, 8.5f, 48, 1);
    glPopMatrix();

    // Add Cockpit Windows flush against the new smooth nose
    // drawCockpitWindows();
    drawSideWindowsAndLogo();
    drawCockpitView();
    // ==========================================
    // 2. WINGS
    // ==========================================
    glColor3f(0.70f, 0.70f, 0.75f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_grey);
    
    // Right Wing
    glBegin(GL_TRIANGLES);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.7f, -0.2f, -0.5f);  
    glVertex3f(8.5f, -0.1f, 4.0f);   
    glVertex3f(0.7f, -0.2f, 3.0f);   
    
    // Left Wing
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.7f, -0.2f, -0.5f); 
    glVertex3f(-0.7f, -0.2f, 3.0f);  
    glVertex3f(-8.5f, -0.1f, 4.0f);  
    glEnd();

    // A350 Style Curved Gold Winglets
    glColor3f(0.85f, 0.65f, 0.15f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_gold);
    glBegin(GL_QUADS);
    glVertex3f(8.5f, -0.1f, 3.5f); glVertex3f(8.7f, 1.4f, 4.2f);
    glVertex3f(8.7f, 1.4f, 4.8f); glVertex3f(8.5f, -0.1f, 4.0f);
    
    glVertex3f(-8.5f, -0.1f, 3.5f); glVertex3f(-8.5f, -0.1f, 4.0f);
    glVertex3f(-8.7f, 1.4f, 4.8f); glVertex3f(-8.7f, 1.4f, 4.2f);
    glEnd();

    // ==========================================
    // 3. ENGINES (Air India Red with Gold Trim)
    // ==========================================
    float enginePositions[2] = { 3.0f, -3.0f }; // Right and Left

    for (int i = 0; i < 2; i++) {
        glPushMatrix(); 
        glTranslatef(enginePositions[i], -0.8f, -0.2f); 
        
        // Red Engine Cowling
        glColor3f(0.85f, 0.10f, 0.15f);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_red);
        gluCylinder(quadric, 0.60f, 0.45f, 2.5f, 32, 1); 
        
        // Gold Intake Trim
        glColor3f(0.85f, 0.65f, 0.15f);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_gold);
        glTranslatef(0.0f, 0.0f, -0.2f);
        gluCylinder(quadric, 0.58f, 0.60f, 0.2f, 32, 1); 
        
        // Dark Engine Fan/Spinner
        glColor3f(0.15f, 0.15f, 0.15f);
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_dark);
        gluDisk(quadric, 0.0f, 0.58f, 32, 1);            
        glPopMatrix();
    }

    // ==========================================
    // 4. AIR INDIA TAIL LIVERY (Geometric Design)
    // ==========================================
    // Base White Tail Fin Structure
    glColor3f(0.95f, 0.95f, 0.95f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_white);
    glBegin(GL_POLYGON);
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.7f, 6.0f); 
    glVertex3f(0.0f, 3.5f, 8.5f);
    glVertex3f(0.0f, 3.5f, 9.8f);
    glVertex3f(0.0f, 0.7f, 9.2f);
    glEnd();

    // Gold Geometric Stripe
    glColor3f(0.85f, 0.65f, 0.15f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_gold);
    glBegin(GL_QUADS);
    // Draw slightly offset on X-axis (0.01f) to prevent z-fighting with the white base
    glVertex3f(0.01f, 1.5f, 7.1f); glVertex3f(0.01f, 2.2f, 7.5f);
    glVertex3f(0.01f, 2.2f, 9.5f); glVertex3f(0.01f, 1.5f, 9.0f);
    
    // Draw on the other side (-0.01f)
    glVertex3f(-0.01f, 1.5f, 7.1f); glVertex3f(-0.01f, 1.5f, 9.0f);
    glVertex3f(-0.01f, 2.2f, 9.5f); glVertex3f(-0.01f, 2.2f, 7.5f);
    glEnd();

    // Red Geometric Top
    glColor3f(0.85f, 0.10f, 0.15f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_red);
    glBegin(GL_POLYGON);
    glVertex3f(0.02f, 2.2f, 7.5f); 
    glVertex3f(0.02f, 3.5f, 8.5f);
    glVertex3f(0.02f, 3.5f, 9.8f);
    glVertex3f(0.02f, 2.2f, 9.5f);
    glEnd();
    
    glBegin(GL_POLYGON);
    glVertex3f(-0.02f, 2.2f, 7.5f); 
    glVertex3f(-0.02f, 2.2f, 9.5f);
    glVertex3f(-0.02f, 3.5f, 9.8f);
    glVertex3f(-0.02f, 3.5f, 8.5f);
    glEnd();

    // Rear Horizontal Stabilizers (Tail wings)
    glColor3f(0.70f, 0.70f, 0.75f);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_grey);
    glBegin(GL_TRIANGLES);
    // Right
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.3f, 0.3f, 7.5f); glVertex3f(3.5f, 0.3f, 9.2f); glVertex3f(0.3f, 0.3f, 9.5f);
    // Left
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.3f, 0.3f, 7.5f); glVertex3f(-0.3f, 0.3f, 9.5f); glVertex3f(-3.5f, 0.3f, 9.2f);
    glEnd();

    glPopMatrix();
}