#version 450

/**
 * @file skybox.frag
 * @brief Fragment shader for the environmental skybox.
 *
 * Samples a cubemap to render the surrounding environment. It implements 
 * atmospheric synchronization by tinting the sky based on global light state 
 * and applies height-based dimming to simulate day/night transitions.
 */

// --- Inputs (Interpolated from skybox.vert) ---
layout(location = 0) in vec3 fragTexCoord;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

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

/** * @brief Environment Cubemap
 * Set 1 Binding 0: Passed specifically to the skybox pipeline.
 */
layout(set = 1, binding = 0) uniform samplerCube skyboxSampler;

void main() {
    // 1. CUBEMAP SAMPLING
    // Sample the environment map using the 3D direction vector.
    vec3 envColor = texture(skyboxSampler, fragTexCoord).rgb;
    
    // 2. ATMOSPHERIC SYNCHRONIZATION
    // Apply tints from the ClimateManager (Day/Sunset/Night/Weather).
    vec3 atmosphericColor = envColor * ubo.lightColor;
    
    // 3. SKY DIMMING (Day/Night Intensity)
    // Uses light height to drive a "Dark Night" effect with generic normalization.
    float height = clamp(ubo.lightPos.y / 5.0, -1.0, 1.0);
    float skyDimmer = clamp(height + 0.4, 0.05, 1.2);
    
    // 4. FINAL COMPOSITION
    // Output the tinted and dimmed sky color as an opaque fragment.
    outColor = vec4(atmosphericColor * skyDimmer, 1.0);
}