#version 450

/**
 * @file fire.vert
 * @brief Vertex shader for the Bonfire particle system.
 *
 * Transforms simulated particle positions into clip space and calculates
 * perspective-accurate point sizes for the flame embers.
 */

// --- Inputs (Directly from the Storage Buffer / Vertex Input) ---
layout(location = 0) in vec4 inPosition; // xyz = pos, w = size
layout(location = 1) in vec4 inVelocity; // xyz = vel, w = life
layout(location = 2) in vec4 inColor;

// --- Outputs ---
layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragLife;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- Uniform Data (Global Engine State) ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;    
    vec3 lightColor; 
    int  useGouraud; 
    float time;
    SparkLight sparks[4]; // Matches Experience.h refactor
} ubo;

void main() {
    // 1. WORLD TO VIEW SPACE TRANSFORMATION
    vec4 viewPos = ubo.view * vec4(inPosition.xyz, 1.0);
    
    // 2. VIEW TO CLIP SPACE TRANSFORMATION
    gl_Position = ubo.proj * viewPos;

    // 3. PERSPECTIVE POINT ATTENUATION
    float dist = length(viewPos.xyz);
    
    // REDUCED MULTIPLIER: Use 15.0 or 20.0 instead of 50.0
    // This keeps them bulky but prevents the "white wall" effect
    gl_PointSize = inPosition.w * (1.0 / dist) * 20.0; 

    // 4. DATA PASSTHROUGH
    fragColor = inColor;
}