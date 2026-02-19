#version 450

/**
 * @file checker.vert
 * @brief Checkerboard shader updated for 128-byte Push Constants (MVP + Model).
 */

// --- Uniform Interfaces ---
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    
    // Aligned with Common.h
    vec4 sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
// Updated to match the 128-byte layout used in Pipeline.h and Mesh::draw
layout(push_constant) uniform Push {
    mat4 mvp;   // View-Projection * Model (Viewport quadrant specific)
    mat4 model; // Raw World Matrix for 3D reconstruction
} push;

// --- Inputs ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;

void main() {
    // 1. POSITION TRANSFORMATION
    // Projects the vertex into the specific quadrant for Multiview
    gl_Position = push.mvp * vec4(inPosition, 1.0);

    // 2. DATA PASSING
    fragTexCoord = inTexCoord;
    
    // 3. 3D DEPTH RECONSTRUCTION
    // FIX: Transform to world-space so lighting isn't "flat"
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz; 
    
    // Transform normal to world space to react to rotation
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
}