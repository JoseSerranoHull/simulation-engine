#version 450

/**
 * @file gouraud.frag
 * @brief Fragment shader for Gouraud-shaded environmental assets.
 *
 * This shader performs a simple texture lookup and modulates the result 
 * with the interpolated vertex color calculated in the vertex stage. 
 */

// --- Interpolated Inputs (from gouraud.vert) ---
layout(location = 0) in vec3 vColor;
layout(location = 1) in vec2 vTexCoord;

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
    SparkLight sparks[4]; // Must match C++ exactly
} ubo;

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

// --- Set 1: Material Textures ---
layout(set = 1, binding = 0) uniform sampler2D texSampler;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. FINAL COMPOSITION
    // Texture sampling modulated by per-vertex color.
    outColor = vec4(vColor * texture(texSampler, vTexCoord).rgb, 1.0);
}