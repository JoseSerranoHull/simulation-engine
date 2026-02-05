#version 450

/**
 * @file phong.frag
 * @brief Multi-mode fragment shader for primary scene objects.
 *
 * Implements Blinn-Phong lighting with shadow mapping and dynamic point lighting 
 * (sparks). Supports a fallback Gouraud mode and a standard Phong/PBR-lite mode 
 * utilizing metallic and roughness textures.
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
    SparkLight sparks[4]; 
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

// --- Set 1: Material Textures ---
layout(set = 1, binding = 0) uniform sampler2D texSampler;
layout(set = 1, binding = 3) uniform sampler2D metallicSampler;
layout(set = 1, binding = 4) uniform sampler2D roughnessSampler;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

/**
 * @brief Performs shadow map lookup and depth comparison.
 * @param posLightSpace Fragment position in light-space NDC.
 * @return Light multiplier (0.4 for shadow, 1.0 for lit).
 */
float calculateShadow(vec4 posLightSpace) {
    // 1. Perspective divide
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // 2. Transform ONLY X and Y from [-1,1] to [0,1] for UV sampling
    projCoords.xy = projCoords.xy * 0.5 + 0.5; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    // 3. Simple bias to prevent shadow acne
    float bias = 0.002;
    return (currentDepth - bias > closestDepth) ? 0.4 : 1.0;
}

/**
 * @brief Calculates contributions from dynamic spark point lights.
 * @param N Surface normal.
 * @param fragPos World-space fragment position.
 * @param albedo Surface diffuse color.
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
    
    // Fire Ambient: Ensures pitch-black areas near fire retain slight warmth
    vec3 fireAmbient = albedo * vec3(1.0, 0.4, 0.1) * 0.015;
    
    return totalSparkLight + fireAmbient;
}

void main() {
    // 1. INITIAL SETUP
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    float shadow = calculateShadow(fragPosLightSpace);

    // 2. DYNAMIC AMBIENT CALCULATION
    vec3 ambientResult = albedo * (ubo.lightColor * 0.05); 

    if (ubo.useGouraud == 1) {
        // 3. GOURAUD FALLBACK MODE
        vec3 sparkContribution = calculateSparkLighting(normalize(fragNormal), fragPos, albedo);
        outColor = vec4(ambientResult + (albedo * fragGouraudColor * shadow) + sparkContribution, 1.0);
    } else {
        // 4. PHONG / PBR-LITE MODE
        float metallic = texture(metallicSampler, fragTexCoord).r;
        float roughness = texture(roughnessSampler, fragTexCoord).r;
        
        vec3 N = normalize(fragNormal);
        vec3 L = normalize(ubo.lightPos - fragPos);
        vec3 V = normalize(ubo.viewPos - fragPos);
        vec3 H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);
        
        // Specular logic based on material roughness
        float specPower = mix(128.0, 2.0, roughness); 
        float spec = pow(max(dot(N, H), 0.0), specPower) * 0.5;
        
        // Metal workflow: Tint reflections and darken diffuse
        vec3 specularTint = mix(vec3(1.0), albedo, metallic);
        vec3 diffuseColor = albedo * (1.0 - metallic);

        // 5. LIGHTING COMPOSITION
        vec3 diffuseResult = (diffuseColor * diff * ubo.lightColor) * shadow;
        vec3 specularResult = (specularTint * spec * ubo.lightColor) * shadow;

        // Spark Light contribution
        vec3 sparkContribution = calculateSparkLighting(N, fragPos, albedo);

        // Final Fragment Output
        outColor = vec4(ambientResult + diffuseResult + specularResult + sparkContribution, 1.0);
    }
}