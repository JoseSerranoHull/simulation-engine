#version 450

/**
 * @file water.frag
 * @brief Refractive water shader with Horizon-Clamped lighting logic.
 *
 * This shader simulates animated water with screen-space refraction. It uses 
 * a virtualized light position to ensure that reflections and water tints 
 * remain aesthetically pleasing even when the global light source orbits 
 * below the horizon.
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragGouraudColor;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- SET 0: GLOBAL ENGINE DATA ---
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

/** * @brief Resolved Scene Color
 * Set 0 Binding 2: Matches global layout where the resolved opaque scene 
 * is provided for refractive sampling.
 */
layout(set = 0, binding = 2) uniform sampler2D sceneSampler; 

// --- SET 1: MATERIAL SPECIFIC DATA ---
layout(set = 1, binding = 1) uniform sampler2D normalSampler;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. HORIZON CLAMP (The Experiment)
    // Create a 'virtual' height to prevent reflections from breaking at night.
    // By clamping at -0.1, we maintain a "Dusk" state throughout the night phase.
    vec3 virtualLightPos = ubo.lightPos;
    virtualLightPos.y = max(virtualLightPos.y, -0.1);

    // 2. RIPPLE ANIMATION & NORMAL MAPPING
    vec2 movingUV = fragTexCoord + vec2(ubo.time * 0.02, ubo.time * 0.02);
    vec3 normalMap = texture(normalSampler, movingUV).rgb * 2.0 - 1.0;
    vec3 N = normalize(fragNormal + normalMap * 0.6); 

    // 3. REFRACTION LOGIC
    vec2 screenUV = gl_FragCoord.xy / textureSize(sceneSampler, 0).xy;
    float waterDistortion = 0.02; 
    
    vec2 finalUV = clamp(screenUV + (normalMap.xy * waterDistortion), 0.001, 0.999);
    vec3 sceneColor = texture(sceneSampler, finalUV).rgb;

    // 4. DYNAMIC FRESNEL & ATMOSPHERIC REFLECTION
    // Use the virtualized height for lighting calculations.
    float height = virtualLightPos.y / 4.0; 
    float reflectionStrength = clamp(height + 0.5, 0.1, 0.6);

    vec3 V = normalize(ubo.viewPos - fragPos);
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 4.0);
    fresnel = clamp(fresnel, 0.0, reflectionStrength); 

    // 5. COLOR COMPOSITION
    // The tints now use the virtual height-based calculations to stay stable.
    vec3 skyReflection = ubo.lightColor * 0.5; 
    vec3 waterTint = vec3(0.02, 0.08, 0.1) * (ubo.lightColor + 0.2); 
    
    // 6. FINAL BLENDING
    // Background is a mix of refracted scene and the deep-water tint.
    vec3 background = mix(sceneColor, waterTint, 0.5);
    vec3 finalColor = mix(background, skyReflection, fresnel);

    // Alpha remains high (0.85) to ensure the floor is visible but distorted.
    outColor = vec4(finalColor, 0.85);
}