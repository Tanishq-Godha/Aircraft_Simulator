#ifndef SHADOW_SYSTEM_H
#define SHADOW_SYSTEM_H

#include "shader_loader.h"

struct ShadowSystem {
    GLuint shadowFBO;
    GLuint shadowMap;
    GLuint depthShader;
    GLuint mainShader;
    
    int shadowSize;
    float lightSpaceMatrix[16];
    
    bool init(int size);
    void setupLightSpace(float sunX, float sunY, float sunZ, float px, float py, float pz);
    void bindShadowPass();
    void bindMainPass(float sunX, float sunY, float sunZ, float* cameraMatrix);
    void unbind();
};

extern ShadowSystem gShadows;

#endif
