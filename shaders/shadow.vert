#version 450

/**
 * @file shadow.vert
 * @brief Shadow mapping vertex shader updated for 128-byte push constant alignment.
 */

// --- Inputs ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormal;   

// --- Outputs ---
layout(location = 0) out vec2 fragTexCoord;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    // 16-byte aligned
};

// --- Set 0: Global Data ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    
    // Aligned with Common.h (10 sparks + checker colors)
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte alignment.
 * Shadow pass uses the first 64 bytes for the raw model matrix.
 * The second 64 bytes are unused padding to maintain MeshPushConstants consistency.
 */
layout(push_constant) uniform PushConstants { 
    mat4 model;   // First 64 bytes: World transformation
    mat4 unused;  // Second 64 bytes: Padding for 128-byte layout
} push;

void main() {
    // 1. DATA PASSTHROUGH
    fragTexCoord = inTexCoord;

    // 2. LIGHT-SPACE TRANSFORMATION
    // Transforms the vertex into world space via push.model, 
    // then into light-space using the matrix from the aligned UBO.
    gl_Position = ubo.lightSpaceMatrix * push.model * vec4(inPosition, 1.0);
}