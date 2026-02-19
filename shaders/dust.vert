#version 450

/**
 * @file dust.vert
 * @brief Particle shader updated for 128-byte Push Constant alignment.
 */

// --- Inputs (From Storage Buffer) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; 
layout(location = 2) in vec4 inColor;    

// --- Outputs ---
layout(location = 0) out vec4 fragColor;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte alignment
    vec4 color;    
};

// --- Uniform Data ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte alignment.
 * For particles, the 'model' matrix is identity because positions are already world-space.
 * We use the first 64 bytes for the quadrant's View-Projection.
 */
layout(push_constant) uniform Push {
    mat4 vp;    // View-Projection (Quadrant specific)
    mat4 unused;// Padding to maintain 128-byte Mesh alignment
} push;

void main() {
    // 1. POSITION TRANSFORMATION
    // Particles are already in world space from compute simulation.
    vec4 worldPos = vec4(inPosition.xyz, 1.0);
    gl_Position = push.vp * worldPos;

    // 2. PERSPECTIVE POINT ATTENUATION
    // Calculate distance relative to the active camera for 3D depth
    float dist = distance(inPosition.xyz, ubo.viewPos);
    dist = max(dist, 0.1); // Guard against division by zero

    /**
     * @brief Point Size Calculation
     * Multiplier (15.0) ensures dust looks like fine grit rather than large flakes.
     */
    gl_PointSize = inPosition.w * (15.0 / dist);

    // 3. COLOR PASSTHROUGH
    fragColor = inColor; 
}