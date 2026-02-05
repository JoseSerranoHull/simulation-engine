#version 450

/**
 * @file base.frag
 * @brief Primary fragment shader for opaque environmental assets.
 * Implements Blinn-Phong lighting with shadow mapping, normal mapping, 
 * and multi-point spark lighting for the desert fire effects.
 */

// --- Interpolated Inputs (from phong.vert) ---
layout(location = 0) in vec3 fragPos;              // World-space fragment position
layout(location = 1) in vec2 fragTexCoord;        // Texture coordinates
layout(location = 2) in vec3 fragNormal;          // Interpolated surface normal
layout(location = 3) in vec3 fragGouraudColor;    // Fallback Gouraud lighting (if enabled)
layout(location = 4) in vec4 fragPosLightSpace;   // Position relative to light source

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
    int  useGouraud;
    float time;
    SparkLight sparks[4]; // Array of dynamic point lights for fire effects
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

layout(set = 1, binding = 0) uniform sampler2D texSampler;    // Albedo/Diffuse
layout(set = 1, binding = 1) uniform sampler2D normalSampler; // Tangent/Object space normals
layout(set = 1, binding = 2) uniform sampler2D aoSampler;     // Ambient Occlusion

// --- Outputs ---
layout(location = 0) out vec4 outColor;

/**
 * @brief Performs shadow map lookup and depth comparison.
 * @param posLightSpace Fragment position transformed by the light's MVP matrix.
 * @return 0.4 if in shadow, 1.0 if illuminated.
 */
float calculateShadow(vec4 posLightSpace) {
    // 1. Perspective divide to get Normalized Device Coordinates [-1, 1]
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // 2. Transform XY from [-1,1] to [0,1] for UV sampling
    // Z remains [0,1] due to Vulkan's depth convention (GLM_FORCE_DEPTH_ZERO_TO_ONE)
    projCoords.xy = projCoords.xy * 0.5 + 0.5; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    // 3. Shadow Acne Prevention: Bias is calibrated to prevent moiré patterns on flat desert surfaces.
    float bias = 0.002; 
    return (currentDepth - bias > closestDepth) ? 0.4 : 1.0;
}

/**
 * @brief Calculates point-light contributions from the four dynamic cactus fire sparks.
 * Implements Linear+Quadratic falloff and Half-Lambert wrapping for soft illumination.
 */
vec3 calculateSparkLighting(vec3 N, vec3 fragPos, vec3 albedo) {
    vec3 totalSparkLight = vec3(0.0);
    
    for(int i = 0; i < 4; i++) {
        vec3 L_vec = ubo.sparks[i].position - fragPos;
        float dist = length(L_vec);
        vec3 L     = normalize(L_vec);
        
        // Short-range attenuation (Linear + Quadratic falloff)
        float attenuation = 1.0 / (1.0 + 2.0 * dist + 25.0 * (dist * dist));
        if (attenuation < 0.01) continue;
        
        // Wrap Lighting: Allows light to bend around curved surfaces (like cactus ribs)
        float wrap = 0.4; 
        float diff = max(0.0, (dot(N, L) + wrap) / (1.0 + wrap));
        
        totalSparkLight += (albedo * ubo.sparks[i].color * diff * attenuation);
    }
    
    // Ambient Warmth: Subtly tints the scene near the fire origin.
    vec3 fireAmbient = albedo * vec3(1.0, 0.4, 0.1) * 0.015;
    
    return totalSparkLight + fireAmbient;
}

void main() {
    // 1. Texture Sampling
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    float ao    = texture(aoSampler, fragTexCoord).r;
    
    // 2. Normal Reconstruction
    // Unpacks [0, 1] texture data to [-1, 1] vector space.
    vec3 mapNormal = texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0;
    // Blend interpolated normal with map normal (weighted by 2.2 for increased detail depth)
    vec3 N = normalize(fragNormal + mapNormal * 2.2); 
    
    // 3. Lighting Vector Math
    vec3 L = normalize(ubo.lightPos - fragPos);
    vec3 V = normalize(ubo.viewPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    
    // 4. Specular (Hardness: 16.0)
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), 16.0) * 0.3;
    
    // 5. Shadow Evaluation
    float shadow = calculateShadow(fragPosLightSpace);

    // 6. Dynamic Ambient (Day/Night Synchronization)
    // Ambient intensity is tied to ubo.lightColor, ensuring the world darkens as the sun sets.
    vec3 ambientResult = albedo * (ubo.lightColor * 0.05 * ao); 

    // 7. Illumination Composition
    vec3 diffuseResult  = (albedo * diff * ubo.lightColor) * shadow;
    vec3 specularResult = (spec * ubo.lightColor) * shadow;

    // 8. Fire/Spark Light Integration
    vec3 sparkContribution = calculateSparkLighting(N, fragPos, albedo);

    // Final Color Output (Alpha forced to 1.0 for opaque pass)
    outColor = vec4(ambientResult + diffuseResult + specularResult + sparkContribution, 1.0);
}