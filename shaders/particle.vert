#version 450

/**
 * @file particle.vert
 * @brief Generic particle vertex shader updated for 128-byte alignment.
 */

// --- Inputs (Directly from Storage Buffer) ---
layout(location = 0) in vec4 inPosition; // xyz = world pos, w = size
layout(location = 1) in vec4 inVelocity; // Unused for drawing
layout(location = 2) in vec4 inColor;

// --- Outputs ---
layout(location = 0) out vec4 fragColor;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    // 16-byte aligned
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
    
    // Updated to match 10 sparks and checker colors in Common.h
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/**
 * Fulfills Step 2: 128-byte alignment.
 * Receives the quadrant-specific View-Projection matrix from the Renderer.
 * Padding matrix ensures consistency with MeshPushConstants.
 */
layout(push_constant) uniform Push {
    mat4 vp;      // View-Projection (Quadrant specific)
    mat4 unused;  // Padding for 128-byte consistency
} push;

void main() {
    // 1. POSITION TRANSFORMATION
    // Projects the world-space particle into the correct split-screen quadrant
    gl_Position = push.vp * vec4(inPosition.xyz, 1.0);

    // 2. POINT ATTENUATION
    // FIX: Calculate distance relative to active camera to restore 3D depth
    float dist = distance(inPosition.xyz, ubo.viewPos);
    dist = max(dist, 0.1); // Guard against division by zero

    // Adjust size based on distance and the 'w' size component from compute simulation
    gl_PointSize = inPosition.w * (10.0 / dist); 

    // 3. COLOR PASSTHROUGH
    fragColor = inColor;
}