#version 450

/**
 * @file shader.frag
 * @brief Basic fragment shader for textured environmental assets.
 *
 * This shader performs a simple texture lookup and outputs the result with 
 * a forced opaque alpha. It maintains the uniform interface required for 
 * consistency with the engine's global descriptor layouts.
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragGouraudColor; // Added to match phong.vert

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

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. TEXTURE SAMPLING
    // Fetch base albedo from the diffuse map.
    vec4 texColor = texture(texSampler, fragTexCoord);

    // 2. FINAL COMPOSITION
    // Output the sampled color with alpha forced to 1.0 for the opaque pass.
    outColor = vec4(texColor.rgb, 1.0);
}