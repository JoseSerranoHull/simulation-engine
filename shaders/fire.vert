#version 450

/**
 * @file fire.vert
 * @brief Vertex shader for fire particles updated for 128-byte alignment.
 */

// --- Inputs (Directly from the Storage Buffer) ---
layout(location = 0) in vec4 inPosition; // xyz = world pos, w = size
layout(location = 1) in vec4 inVelocity; // xyz = velocity, w = life
layout(location = 2) in vec4 inColor;

// --- Outputs ---
layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragLife;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // Strict 16-byte alignment
    vec4 color;    // Matches C++ structure
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

    // Synchronized with Common.h (10 sparks + checker colors)
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte alignment.
 * We use the first 64 bytes for the quadrant-specific View-Projection.
 * The second 64 bytes are unused padding to match MeshPushConstants.
 */
layout(push_constant) uniform Push {
    mat4 vp;      // Projection * View (Quadrant specific)
    mat4 unused;  // Padding for 128-byte alignment
} push;

void main() {
    // 1. POSITION TRANSFORMATION
    // Transform world-space particle position using the quadrant's VP matrix
    vec4 worldPos = vec4(inPosition.xyz, 1.0);
    gl_Position = push.vp * worldPos;

    // 2. PERSPECTIVE POINT ATTENUATION
    // Calculate distance relative to the active camera to restore 3D depth
    float dist = distance(inPosition.xyz, ubo.viewPos);
    dist = max(dist, 0.1); 

    // Point Size Calculation (20.0 multiplier for voluminous embers)
    gl_PointSize = inPosition.w * (20.0 / dist); 

    // 3. DATA PASSTHROUGH
    fragColor = inColor;
    fragLife = inVelocity.w; 
}