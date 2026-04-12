#version 120
varying vec4 vFragPosLightSpace;
varying vec3 vNormal;
varying vec3 vFragPos;
varying float vFogDist; // Distance for Fog

uniform mat4 uLightSpaceMatrix;
uniform mat4 uInvViewMatrix;

void main() {
    // Transform vertex to Eye Space (Standard OpenGL)
    vec4 eyePos = gl_ModelViewMatrix * gl_Vertex;
    
    // Pass the absolute eye-space distance (z is negative in view space)
    vFogDist = length(eyePos.xyz);
    
    // Convert back to World Space for the Light Space Matrix
    vec4 worldPos = uInvViewMatrix * eyePos;
    
    vFragPos = worldPos.xyz;
    vNormal = normalize(gl_NormalMatrix * gl_Normal);
    
    // Project into Light Space for Shadow Sampling
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    
    gl_Position = gl_ProjectionMatrix * eyePos;
    gl_FrontColor = gl_Color;
}

