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

float Hash(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float LeafNoise(vec3 p) {
    float n = Hash(floor(p * 0.25)); // Big clusters
    n += Hash(floor(p * 1.1)) * 0.5; // Fine grain
    n += Hash(floor(p * 4.0)) * 0.25; // Sharp detail
    return n / 1.75;
}

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
    vec3 baseColor = gl_Color.rgb;
    
    // Detect foliage (green-ish tint)
    bool isFoliage = (baseColor.g > baseColor.r * 1.1 && baseColor.g > baseColor.b * 1.1);
    
    if (isFoliage) {
        float noise = LeafNoise(vFragPos);
        // Apply color jitter (darker and lighter "leaves")
        baseColor *= (0.85 + noise * 0.35);
        // Add a slight "vein" or "stem" effect by darkening based on coordinates
        if (fract(vFragPos.x * 0.1) < 0.05 || fract(vFragPos.z * 0.1) < 0.05) {
            baseColor *= 0.9;
        }
    }
    
    // Detect Asphalt (Roads, Runways, Taxiways)
    // Detect Asphalt (Roads, Runways, Taxiways)
    bool isAsphalt = (baseColor.r < 0.35 && abs(baseColor.r - baseColor.g) < 0.15);
    
    if (isAsphalt) {
        // 1. High-Fidelity Asphalt Grain
        float grain = Hash(vFragPos * 10.0); 
        float grainBig = Hash(floor(vFragPos * 0.1));
        baseColor *= (0.65 + grain * 0.4 + grainBig * 0.15); 
        if (Hash(vFragPos * 0.05) > 0.985) baseColor *= 0.75; // Weathering

        // 2. Symmetric Alignment Logic
        float ax = abs(vFragPos.x);
        float az = abs(vFragPos.z);
        
        // Major Grid (3000 spacing, 450 footprint)
        float mX3 = mod(ax, 3000.0); float mZ3 = mod(az, 3000.0);
        float dM_X = min(mX3, 3000.0 - mX3);
        float dM_Z = min(mZ3, 3000.0 - mZ3);
        
        // Minor Grid (1200 spacing, 150 footprint)
        float mX1 = mod(ax, 1200.0); float mZ1 = mod(az, 1200.0);
        float dS_X = min(mX1, 1200.0 - mX1);
        float dS_Z = min(mZ1, 1200.0 - mZ1);

        bool roadNS_MAJ = (dM_X < 225.0); bool roadEW_MAJ = (dM_Z < 225.0);
        bool roadNS_MIN = (dS_X < 75.0);  bool roadEW_MIN = (dS_Z < 75.0);
        bool roadNS = roadNS_MAJ || roadNS_MIN;
        bool roadEW = roadEW_MAJ || roadEW_MIN;

        // --- 3. Intersection Suppression (Corners Logic) ---
        if (roadNS && roadEW) {
            // No markings in the middle of crossings
        } else {
            // --- 4. Dynamic Marking Placement ---
            float w = 0.0; float d = 0.0; bool isM = false;
            if (roadNS) {
                isM = roadNS_MAJ;
                w = isM ? 450.0 : 150.0;
                d = isM ? dM_X : dS_X;
            } else if (roadEW) {
                isM = roadEW_MAJ;
                w = isM ? 450.0 : 150.0;
                d = isM ? dM_Z : dS_Z;
            }

            if (w > 0.0) {
                // A. White Dashed Centerline (Centered on 0.0)
                if (d < 5.0) {
                    float dash = mod(vFragPos.x + vFragPos.z + 30000.0, 500.0);
                    if (dash < 280.0) baseColor = vec3(1.0, 1.0, 1.0);
                }
                
                // B. Solid Yellow Sides (Absolute Ends)
                // Only for "Thicker" Major Roads
                if (isM && abs(d - 225.0) < 4.5) {
                    baseColor = vec3(0.95, 0.75, 0.08);
                }
                
                // C. Tire Wear (Major Only)
                if (isM && abs(d - 80.0) < 45.0) baseColor *= 0.85;
            }
        }
    }

    float diff = max(dot(vNormal, uLightDir), 0.0);
    float shadow = ShadowCalculation(vFragPosLightSpace);
    
    // Ambient + Diffuse (with shadow)
    float ambient = 0.35;
    vec3 lighting = (ambient + (1.0 - shadow) * diff) * baseColor;
    
    // Manual Fog calculation (Linear)
    if (uFogEnabled) {
        float fogFactor = (uFogEnd - vFogDist) / (uFogEnd - uFogStart);
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        lighting = mix(uFogColor, lighting, fogFactor);
    }
    
    gl_FragColor = vec4(lighting, gl_Color.a);
}

