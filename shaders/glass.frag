#version 450

/**
 * @file glass.frag
 * @brief Refractive glass shader with Horizon-Clamped lighting logic.
 *
 * Implements screen-space refraction and dynamic Fresnel reflections. 
 * Includes an experimental clamp that prevents lighting artifacts when the 
 * orbiting light source dips below the scene's horizon.
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- Set 0: Global Data ---
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

/** * @brief Scene Sampler
 * Set 0 Binding 2: Matches global layout where the resolved opaque scene 
 * is provided for refractive sampling.
 */
layout(set = 0, binding = 2) uniform sampler2D sceneSampler; 

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. HORIZON CLAMP (The Experiment)
    // We create a 'virtual' light position that never dips below dawn/dusk height.
    // This prevents fresnel and height-based math from breaking at night.
    vec3 virtualLightPos = ubo.lightPos;
    virtualLightPos.y = max(virtualLightPos.y, -0.1); // Clamp slightly below 0.0

    // 2. SCREEN-SPACE COORDINATE CALCULATION
    vec2 screenUV = gl_FragCoord.xy / textureSize(sceneSampler, 0).xy;
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(ubo.viewPos - fragPos);

    // 3. REFRACTION DISTORTION
    float distortionStrength = 0.03; 
    vec2 offset = N.xy * distortionStrength;
    
    // 4. SCENE SAMPLING
    vec2 finalUV = clamp(screenUV + offset, 0.001, 0.999);
    vec3 sceneColor = texture(sceneSampler, finalUV).rgb;

    // 5. LIGHTING & FRESNEL EFFECTS
    // Use the virtual light position to calculate height-based intensity.
    vec3 reflectionColor = ubo.lightColor * 0.4; 

    // Use virtualLightPos.y instead of ubo.lightPos.y
    float height = virtualLightPos.y / 4.0;
    float fresnelPower = 5.0;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), fresnelPower);
    
    // We add a floor of 0.005 so there is always a tiny glint on the glass
    float fresnelAlpha = clamp(height + 0.6, 0.005, 0.3);

    // Mix refracted scene with reflection.
    vec3 finalColor = mix(sceneColor, reflectionColor, fresnel * fresnelAlpha);

    // 6. FINAL ALPHA BLENDING
    // Use the height of the virtual light to keep the glass from becoming 
    // too opaque or fully invisible during the 'Night' phase.
    float alpha = clamp(0.5 + (height * 0.2), 0.4, 0.7);
    
    outColor = vec4(finalColor, alpha); 
}