#version 450

/**
 * @file skybox.vert
 * @brief Vertex shader for the environmental skybox.
 *
 * This shader transforms the skybox geometry so that it remains centered 
 * on the camera, simulating infinite distance. It utilizes the "XYWW" trick 
 * to ensure the skybox is always rendered at the maximum depth (z=1.0).
 */

// --- Inputs (Vertex Attributes) ---
layout(location = 0) in vec3 inPosition;

// --- Outputs ---
layout(location = 0) out vec3 outTexCoord;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
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
    SparkLight sparks[4]; // Must match C++ exactly
} ubo;

void main() {
    // 1. TEXTURE COORDINATE PASSTHROUGH
    // The cube's local vertex position acts as the 3D sampling vector.
    outTexCoord = inPosition;
    
    // 2. VIEW MATRIX MODIFICATION
    // Remove translation by casting to mat3 and back to mat4.
    // This makes the skybox appear "infinitely far away" as it follows the camera.
    mat4 viewNoTrans = mat4(mat3(ubo.view)); 
    
    // 3. PROJECTIVE TRANSFORMATION
    vec4 pos = ubo.proj * viewNoTrans * vec4(inPosition, 1.0);
    
    // 4. DEPTH OPTIMIZATION (XYWW Trick)
    // Sets z = w, ensuring that after the perspective divide, z = 1.0.
    // This forces the skybox to the back of the depth buffer.
    gl_Position = pos.xyww;
}