#version 120
varying vec4 vFragPosLightSpace;
varying vec3 vNormal;
varying vec3 vFragPos;
varying float vFogDist;

uniform sampler2D uShadowMap;
uniform vec3 uLightDir;

// Fog Uniforms
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;
uniform bool uFogEnabled;

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    if(projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;
    if(projCoords.z > 1.0) return 0.0;
    
    float currentDepth = projCoords.z;
    float bias = max(0.0025 * (1.0 - dot(vNormal, uLightDir)), 0.0005);
    
    // PCF (Percentage-Closer Filtering) 3x3
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(4096.0, 4096.0); // Match init.cpp res
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture2D(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    
    return shadow;
}

void main() {
    float diff = max(dot(vNormal, uLightDir), 0.0);
    float shadow = ShadowCalculation(vFragPosLightSpace);
    
    // Ambient + Diffuse (with shadow)
    float ambient = 0.35;
    vec3 lighting = (ambient + (1.0 - shadow) * diff) * gl_Color.rgb;
    
    // Manual Fog calculation (Linear)
    if (uFogEnabled) {
        float fogFactor = (uFogEnd - vFogDist) / (uFogEnd - uFogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        lighting = mix(uFogColor, lighting, fogFactor);
    }
    
    gl_FragColor = vec4(lighting, gl_Color.a);
}

