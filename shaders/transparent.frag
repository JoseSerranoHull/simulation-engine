#version 450

/**
 * @file transparent.frag
 * @brief Fragment shader for semi-transparent environmental assets (e.g., grass).
 *
 * Implements alpha-masked transparency with shadow mapping and dynamic 
 * day/night lighting. Includes a constant emissive "magic glow" term to ensure 
 * visibility during night cycles and integrates multi-point spark lighting.
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragGouraudColor;
layout(location = 4) in vec4 fragPosLightSpace;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- Uniform Interfaces ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int useGouraud;
    float time;
    SparkLight sparks[4]; // Must match C++ exactly
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

// --- Set 1: Material Textures ---
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 2) uniform sampler2D alphaMask; 

// --- Outputs ---
layout(location = 0) out vec4 outColor;

/**
 * @brief Calculates contributions from dynamic point lights (cactus fire sparks).
 * @param N Surface normal.
 * @param fragPos World-space fragment position.
 * @param albedo Base diffuse color.
 * @return Total accumulated spark illumination.
 */
vec3 calculateSparkLighting(vec3 N, vec3 fragPos, vec3 albedo) {
    vec3 totalSparkLight = vec3(0.0);
    
    for(int i = 0; i < 4; i++) {
        vec3 L_vec = ubo.sparks[i].position - fragPos;
        float dist = length(L_vec);
        vec3 L = normalize(L_vec);
        
        // Short-range attenuation (Linear + Quadratic falloff)
        float attenuation = 1.0 / (1.0 + 2.0 * dist + 25.0 * (dist * dist));
        if (attenuation < 0.01) continue;
        
        // LIGHT WRAPPING: Prevents sharp black lines on curved surfaces
        float wrap = 0.4; 
        float diff = max(0.0, (dot(N, L) + wrap) / (1.0 + wrap));
        
        totalSparkLight += (albedo * ubo.sparks[i].color * diff * attenuation);
    }
    
    // Fire Ambient: Subtle warmth for fragments near the fire origin
    vec3 fireAmbient = albedo * vec3(1.0, 0.4, 0.1) * 0.015;
    
    return totalSparkLight + fireAmbient;
}

void main() {
    // 1. INITIAL SAMPLING
    vec4 baseColor = texture(texSampler, fragTexCoord);
    float mask = texture(alphaMask, fragTexCoord).r;

    // 2. ALPHA CLIPPING
    // Combines albedo alpha with the mask for standard grass transparency.
    float alpha = baseColor.a * mask;
    if (alpha < 0.05) discard;

    // 3. SHADOW CALCULATION
    // Perspective divide and UV coordinate transformation for shadow sampling.
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.002;
    float shadow = (currentDepth - bias > closestDepth) ? 0.4 : 1.0;

    // 4. DYNAMIC DAY/NIGHT LIGHTING
    // Dims based on lightColor (purple/black at night) provided by ClimateManager.
    vec3 ambientResult = baseColor.rgb * (ubo.lightColor * 0.04); 

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuseResult = (baseColor.rgb * diff * ubo.lightColor) * shadow;

    // 5. MAGIC GLOW (Emissive Term)
    // Provides a faint (0.12) permanent green light for visibility during night cycles.
    vec3 magicGlow = baseColor.rgb * 0.12;

    // 6. FIRE LIGHT INTEGRATION
    vec3 sparkContribution = calculateSparkLighting(N, fragPos, baseColor.rgb);
    
    // 7. FINAL COMPOSITION
    // Combine environmental lighting, magic glow, and spark lights.
    vec3 finalRGB = ambientResult + diffuseResult + magicGlow + sparkContribution;

    outColor = vec4(finalRGB, alpha);
}