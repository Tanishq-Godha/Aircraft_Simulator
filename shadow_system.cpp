#include "shadow_system.h"
#include <GL/glu.h>
#include <iostream>
#include <cmath>

ShadowSystem gShadows;

namespace {
    // Helper to invert a 4x4 matrix (Column-Major)
    // Used to go from Eye Space back to World Space
    bool invertMatrix(const float m[16], float invOut[16]) {
        float inv[16], det;
        int i;

        inv[0] = m[5]  * m[10] * m[15] - 
                 m[5]  * m[11] * m[14] - 
                 m[9]  * m[6]  * m[15] + 
                 m[9]  * m[7]  * m[14] +
                 m[13] * m[6]  * m[11] - 
                 m[13] * m[7]  * m[10];

        inv[4] = -m[4]  * m[10] * m[15] + 
                  m[4]  * m[11] * m[14] + 
                  m[8]  * m[6]  * m[15] - 
                  m[8]  * m[7]  * m[14] - 
                  m[12] * m[6]  * m[11] + 
                  m[12] * m[7]  * m[10];

        inv[8] = m[4]  * m[9] * m[15] - 
                 m[4]  * m[11] * m[13] - 
                 m[8]  * m[5]  * m[15] + 
                 m[8]  * m[7]  * m[13] + 
                 m[12] * m[5]  * m[11] - 
                 m[12] * m[7]  * m[9];

        inv[12] = -m[4]  * m[9] * m[14] + 
                   m[4]  * m[10] * m[13] +
                   m[8]  * m[5]  * m[14] - 
                   m[8]  * m[6]  * m[13] - 
                   m[12] * m[5]  * m[10] + 
                   m[12] * m[6]  * m[9];

        inv[1] = -m[1]  * m[10] * m[15] + 
                  m[1]  * m[11] * m[14] + 
                  m[9]  * m[2]  * m[15] - 
                  m[9]  * m[3]  * m[14] - 
                  m[13] * m[2]  * m[11] + 
                  m[13] * m[3]  * m[10];

        inv[5] = m[0]  * m[10] * m[15] - 
                 m[0]  * m[11] * m[14] - 
                 m[8]  * m[2]  * m[15] + 
                 m[8]  * m[3]  * m[14] + 
                 m[12] * m[2]  * m[11] - 
                 m[12] * m[3]  * m[10];

        inv[9] = -m[0]  * m[9] * m[15] + 
                  m[0]  * m[11] * m[13] + 
                  m[8]  * m[1]  * m[15] - 
                  m[8]  * m[3]  * m[13] - 
                  m[12] * m[1]  * m[11] + 
                  m[12] * m[3]  * m[9];

        inv[13] = m[0]  * m[9] * m[14] - 
                  m[0]  * m[10] * m[13] - 
                  m[8]  * m[1]  * m[14] + 
                  m[8]  * m[2]  * m[13] + 
                  m[12] * m[1]  * m[10] - 
                  m[12] * m[2]  * m[9];

        inv[2] = m[1]  * m[6] * m[15] - 
                 m[1]  * m[7] * m[14] - 
                 m[5]  * m[2]  * m[15] + 
                 m[5]  * m[3]  * m[14] + 
                 m[13] * m[2]  * m[7] - 
                 m[13] * m[3]  * m[6];

        inv[6] = -m[0]  * m[6] * m[15] + 
                  m[0]  * m[7] * m[14] + 
                  m[4]  * m[2]  * m[15] - 
                  m[4]  * m[3]  * m[14] - 
                  m[12] * m[2]  * m[7] + 
                  m[12] * m[3]  * m[6];

        inv[10] = m[0]  * m[5] * m[15] - 
                  m[0]  * m[7] * m[13] - 
                  m[4]  * m[1]  * m[15] + 
                  m[4]  * m[3]  * m[13] + 
                  m[12] * m[1]  * m[7] - 
                  m[12] * m[3]  * m[5];

        inv[14] = -m[0]  * m[5] * m[14] + 
                   m[0]  * m[6] * m[13] + 
                   m[4]  * m[1]  * m[14] - 
                   m[4]  * m[2]  * m[13] - 
                   m[12] * m[1]  * m[6] + 
                   m[12] * m[2]  * m[5];

        inv[3] = -m[1] * m[6] * m[11] + 
                  m[1] * m[7] * m[10] + 
                  m[5] * m[2] * m[11] - 
                  m[5] * m[3] * m[10] - 
                  m[9] * m[2] * m[7] + 
                  m[9] * m[3] * m[6];

        inv[7] = m[0] * m[6] * m[11] - 
                 m[0] * m[7] * m[10] - 
                 m[4] * m[2] * m[11] + 
                 m[4] * m[3] * m[10] + 
                 m[8] * m[2] * m[7] - 
                 m[8] * m[3] * m[6];

        inv[11] = -m[0] * m[5] * m[11] + 
                   m[0] * m[7] * m[9] + 
                   m[4] * m[1] * m[11] - 
                   m[4] * m[3] * m[9] - 
                   m[8] * m[1] * m[7] + 
                   m[8] * m[3] * m[5];

        inv[15] = m[0] * m[5] * m[10] - 
                  m[0] * m[6] * m[9] - 
                  m[4] * m[1] * m[10] + 
                  m[4] * m[2] * m[9] + 
                  m[8] * m[1] * m[6] - 
                  m[8] * m[2] * m[5];

        det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

        if (det == 0) return false;

        det = 1.0f / det;
        for (i = 0; i < 16; i++) invOut[i] = inv[i] * det;
        return true;
    }
}

bool ShadowSystem::init(int size) {
    shadowSize = size;
    if (!initShaderExtensions()) return false;
    
    mainShader = loadShaders("shaders/main_shaded.vert", "shaders/main_shaded.frag");
    
    const char* emptyFrag = "#version 120\nvoid main() {}";
    depthShader = loadShadersFromSource("#version 120\nvoid main() { gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; }", emptyFrag);

    if (!depthShader || !mainShader) return false;

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowSize, shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void ShadowSystem::setupLightSpace(float sunX, float sunY, float sunZ, float px, float py, float pz) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix(); glLoadIdentity();
    
    float orthoSize = 4000.0f; // Greatly expanded to capture distant shadows ahead/behind
    glOrtho(-orthoSize, orthoSize, -orthoSize, orthoSize, 100.0f, 15000.0f);
    float proj[16]; glGetFloatv(GL_PROJECTION_MATRIX, proj);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix(); glLoadIdentity();
    float dist = 8000.0f;
    gluLookAt(px + sunX * dist, py + sunY * dist, pz + sunZ * dist, px, py, pz, 0, 1, 0);
    float view[16]; glGetFloatv(GL_MODELVIEW_MATRIX, view);
    
    glLoadIdentity();
    glMultMatrixf(proj);
    glMultMatrixf(view);
    glGetFloatv(GL_MODELVIEW_MATRIX, lightSpaceMatrix);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void ShadowSystem::bindShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glViewport(0, 0, shadowSize, shadowSize);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(depthShader);
}

void ShadowSystem::bindMainPass(float sunX, float sunY, float sunZ, float* cameraMatrix,
                               const WeatherProfile& weather, float r, float g, float b) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(mainShader);
    
    // Inverse View Matrix to get World Space from Eye Space
    float invView[16];
    if (invertMatrix(cameraMatrix, invView)) {
        glUniformMatrix4fv(glGetUniformLocation(mainShader, "uInvViewMatrix"), 1, GL_FALSE, invView);
    }

    glUniformMatrix4fv(glGetUniformLocation(mainShader, "uLightSpaceMatrix"), 1, GL_FALSE, lightSpaceMatrix);
    glUniform3f(glGetUniformLocation(mainShader, "uLightDir"), sunX, sunY, sunZ);
    
    // Pass Fog Uniforms
    glUniform3f(glGetUniformLocation(mainShader, "uFogColor"), r, g, b);
    glUniform1f(glGetUniformLocation(mainShader, "uFogStart"), weather.fogStart);
    glUniform1f(glGetUniformLocation(mainShader, "uFogEnd"),   weather.fogEnd);
    glUniform1i(glGetUniformLocation(mainShader, "uFogEnabled"), 1); // Fog always available in Game State 1

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowMap);
    glUniform1i(glGetUniformLocation(mainShader, "uShadowMap"), 0);
}


void ShadowSystem::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}
