#version 450

/**
 * @file flock.vert
 * @brief Vertex shader for GPU-driven boid point-sprite rendering.
 *
 * Reads boid position and velocity directly from the FlockGpuBackend SSBO
 * (bound as a vertex buffer). Outputs clip-space position with perspective-
 * attenuated point size, and forwards velocity magnitude to the fragment
 * stage for speed-based colour.
 */

// --- Inputs from BoidState SSBO (stride = 32 bytes) ---
layout(location = 0) in vec4 inPosGroup; // xyz = world position, w = groupId
layout(location = 1) in vec4 inVel;      // xyz = velocity,        w = 0

// --- Outputs ---
layout(location = 0) out float fragSpeed;

// --- Global Engine UBO (set 0, binding 0 — matches all other shaders) ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4  view;
    mat4  proj;
    mat4  lightSpaceMatrix;
    vec3  lightPos;
    vec3  viewPos;
    vec3  lightColor;
    int   useGouraud;
    float time;
    SparkLight sparks[4];
} ubo;

void main() {
    vec4 viewPos = ubo.view * vec4(inPosGroup.xyz, 1.0);
    gl_Position  = ubo.proj * viewPos;

    // Perspective-attenuated point size
    float dist   = length(viewPos.xyz);
    gl_PointSize = clamp(80.0 / max(dist, 0.1), 8.0, 48.0);

    fragSpeed = length(inVel.xyz);
}
