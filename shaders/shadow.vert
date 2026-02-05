#version 450

/**
 * @file shadow.vert
 * @brief Vertex shader for generating shadow depth maps.
 *
 * This shader transforms vertices directly into light-space using the 
 * lightSpaceMatrix. It also passes texture coordinates to the fragment 
 * stage to support alpha-tested shadows for masked materials.
 */

// --- Inputs (Vertex Attributes) ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;    
layout(location = 2) in vec2 inTexCoord; 
layout(location = 3) in vec3 inNormal;   

// --- Outputs ---
layout(location = 0) out vec2 fragTexCoord; // Pass to fragment stage for alpha testing

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
    SparkLight sparks[4]; 
} ubo;

// --- Push Constants ---
layout(push_constant) uniform PushConstants { 
    mat4 model; 
} push;

void main() {
    // 1. DATA PASSTHROUGH
    // Pass UV coordinates for alpha-mask sampling in the fragment stage.
    fragTexCoord = inTexCoord;

    // 2. LIGHT-SPACE TRANSFORMATION
    // Transform the vertex directly into light-perspective clip space.
    // gl_Position = LightProjection * LightView * Model * Position
    gl_Position = ubo.lightSpaceMatrix * push.model * vec4(inPosition, 1.0);
}