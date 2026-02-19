#version 450

/**
 * @file skybox.vert
 * @brief Skybox vertex shader updated for 128-byte Push Constants and Multiview.
 */

// --- Inputs ---
layout(location = 0) in vec3 inPosition;

// --- Outputs ---
layout(location = 0) out vec3 outTexCoord;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    
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
    
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte alignment.
 * In the Opaque pass, the Renderer provides the quadrant VP matrix here.
 * For Skybox, we only use the VP matrix (mvp slot) and treat it as the ViewNoTrans * Proj.
 */
layout(push_constant) uniform PushConstants {
    mat4 mvp;   // View-Projection (Quadrant specific)
    mat4 model; // Unused for skybox (Skybox has no world transform)
} push;

void main() {
    // 1. TEXTURE COORDINATE PASSTHROUGH
    outTexCoord = inPosition;
    
    // 2. PROJECTIVE TRANSFORMATION
    /**
     * Note: In Renderer::recordOpaquePass, the skybox is only drawn in quadrant 0 (Main).
     * We use the push.mvp which already contains the correct View * Proj.
     * To keep it centered, ensure the 'view' used to build 'mvp' has translation removed.
     */
    vec4 pos = push.mvp * vec4(inPosition, 1.0);
    
    // 3. DEPTH OPTIMIZATION (XYWW Trick)
    // Ensures the skybox is always at the maximum depth (z=1.0).
    gl_Position = pos.xyww;
}