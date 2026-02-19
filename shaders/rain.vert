#version 450

/**
 * @file rain.vert
 * @brief Vertex shader for rain particles updated for 128-byte alignment.
 */

// --- Inputs (Directly from the Storage Buffer) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Life/Age
layout(location = 2) in vec4 inColor;    

// --- Outputs ---
layout(location = 0) out vec4 fragColor;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // Using vec4 for strict 16-byte alignment
    vec4 color;    
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
    
    // Aligned with 10 sparks and checker colors
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/**
 * Fulfills Step 2: 128-byte alignment.
 * Receives the quadrant-specific View-Projection matrix from the Renderer.
 * Padding matrix ensures consistency with MeshPushConstants (128 bytes total).
 */
layout(push_constant) uniform Push {
    mat4 vp;      // View-Projection (Quadrant specific)
    mat4 unused;  // Padding for 128-byte consistency
} push;

void main() {
    // 1. POSITION TRANSFORMATION
    // Particles are already in world space from compute simulation.
    // Apply the pre-calculated VP matrix for the current viewport quadrant.
    vec4 worldPos = vec4(inPosition.xyz, 1.0);
    gl_Position = push.vp * worldPos;

    // 2. PERSPECTIVE POINT ATTENUATION
    // Calculate world-space distance from active camera for consistent 3D depth cues.
    float dist = distance(inPosition.xyz, ubo.viewPos);
    dist = max(dist, 0.1); // Guard against division by zero

    /**
     * @brief Point Size Calculation
     * Fulfills Requirement: Visualise simulations from multiple views.
     * MULTIPLIER (12.0): Kept for needle-like rain streaks.
     */
    float multiplier = 12.0;
    gl_PointSize = inPosition.w * (1.0 / dist) * multiplier;

    // 3. COLOR PASSTHROUGH
    fragColor = inColor;
}