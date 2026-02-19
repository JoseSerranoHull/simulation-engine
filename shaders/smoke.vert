#version 450

/**
 * @file smoke.vert
 * @brief Smoke particle vertex shader updated for 128-byte alignment and Multiview.
 */

// --- Inputs (Directly from the Storage Buffer) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Particle Age
layout(location = 2) in vec4 inColor;    

// --- Outputs ---
layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragAge; 

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    // 16-byte aligned
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
    
    // Updated to match 10 sparks and checker colors in Common.h
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/**
 * Fulfills Requirement: 128-byte memory uniformity.
 * Receives the quadrant-specific View-Projection matrix from the Renderer.
 * The 'unused' matrix ensures the block size matches the Mesh shader layout.
 */
layout(push_constant) uniform Push {
    mat4 vp;      // View-Projection (Quadrant specific)
    mat4 unused;  // Padding for 128-byte consistency
} push;

void main() {
    // 1. POSITION TRANSFORMATION
    // Apply the pre-calculated VP matrix for the current quadrant
    vec4 worldPos = vec4(inPosition.xyz, 1.0);
    gl_Position = push.vp * worldPos;

    // 2. POINT SIZE ATTENUATION
    // Calculate world-space distance relative to the active camera
    float dist = distance(inPosition.xyz, ubo.viewPos);
    dist = max(dist, 0.1); // Guard against division by zero

    /**
     * @brief Point Size Calculation
     * Perspective Scaling: Size = BaseSize * (1.0 / distance) * multiplier
     * MULTIPLIER (30.0): Retained for voluminous, billowy smoke.
     */
    gl_PointSize = inPosition.w * (30.0 / dist); 

    // 3. DATA PASSTHROUGH
    fragColor = inColor;
    fragAge = inVelocity.w;
}