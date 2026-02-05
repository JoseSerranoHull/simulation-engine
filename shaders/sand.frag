#version 450

/**
 * @file sand.frag
 * @brief Primary fragment shader for the lunar/desert sand terrain.
 *
 * Implements Blinn-Phong lighting with shadow mapping, normal mapping, 
 * and multi-point spark lighting for dynamic fire effects. It utilizes 
 * global light color to drive dynamic ambient levels for day/night transitions.
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
layout(set = 1, binding = 0) uniform sampler2D texSampler;    // Base Color
layout(set = 1, binding = 1) uniform sampler2D normalSampler; // Normal Map
layout(set = 1, binding = 2) uniform sampler2D aoSampler;     // AO Map

// --- Outputs ---
layout(location = 0) out vec4 outColor;

/**
 * @brief Performs shadow map lookup and depth comparison.
 * @param posLightSpace Fragment position transformed by the light's matrix.
 * @return Light multiplier (0.4 for shadow, 1.0 for illuminated).
 */
float calculateShadow(vec4 posLightSpace) {
    // 1. Perspective divide
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // 2. Transform XY from [-1,1] to [0,1] for UV sampling
    // Z remains [0,1] due to Vulkan's depth convention
    projCoords.xy = projCoords.xy * 0.5 + 0.5; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    // 3. Shadow Acne Prevention
    float bias = 0.002;
    return (currentDepth - bias > closestDepth) ? 0.4 : 1.0;
}

/**
 * @brief Calculates contributions from dynamic point lights (cactus fire sparks).
 * @param N Calculated surface normal.
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
        
        // LIGHT WRAPPING: Allows light to bend around surfaces to prevent sharp black lines
        float wrap = 0.4; 
        float diff = max(0.0, (dot(N, L) + wrap) / (1.0 + wrap));
        
        totalSparkLight += (albedo * ubo.sparks[i].color * diff * attenuation);
    }
    
    // Fire Ambient: Subtle warmth for fragments near the fire origin
    vec3 fireAmbient = albedo * vec3(1.0, 0.4, 0.1) * 0.015;
    
    return totalSparkLight + fireAmbient;
}

void main() {
    // 1. TEXTURE SAMPLING
    // Sample base albedo and ambient occlusion maps.
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    float ao = texture(aoSampler, fragTexCoord).r;
    
    // 2. NORMAL RECONSTRUCTION
    // Combine interpolated normal with detail map normals.
    vec3 mapNormal = texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 N = normalize(fragNormal + mapNormal * 2.2); 
    
    // 3. LIGHTING VECTOR MATH
    vec3 L = normalize(ubo.lightPos - fragPos);
    vec3 V = normalize(ubo.viewPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    
    // 4. SPECULAR CALCULATION
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 16.0) * 0.3;
    
    // 5. SHADOW EVALUATION
    float shadow = calculateShadow(fragPosLightSpace);

    // 6. DYNAMIC AMBIENT (Day/Night Sync)
    // Uses lightColor to ensure the world dims as the sun sets.
    vec3 ambientResult = albedo * (ubo.lightColor * 0.05 * ao); 

    // 7. ILLUMINATION COMPOSITION
    vec3 diffuseResult = (albedo * diff * ubo.lightColor) * shadow;
    vec3 specularResult = (spec * ubo.lightColor) * shadow;

    // 8. FIRE LIGHT INTEGRATION
    vec3 sparkContribution = calculateSparkLighting(N, fragPos, albedo);

    // Final color output (Forced to 1.0 alpha for opaque pass)
    outColor = vec4(ambientResult + diffuseResult + specularResult + sparkContribution, 1.0);
}