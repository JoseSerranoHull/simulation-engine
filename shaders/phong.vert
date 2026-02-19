#version 450

/**
 * @file phong.vert
 * @brief Primary vertex shader updated for 128-byte Push Constants (MVP + Model).
 */

// --- Inputs ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragGouraudColor;
layout(location = 4) out vec4 fragPosLightSpace;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // Strict 16-byte alignment
    vec4 color;    
};

// --- Uniform Interfaces ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    
    SparkLight sparks[10]; // Synchronized with Common.h
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
// Fulfills Step 2: 128-byte block containing both matrices
layout(push_constant) uniform PushConstants { 
    mat4 mvp;   // View-Projection * Model (Quadrant specific)
    mat4 model; // Raw World Matrix (Lighting consistency)
} push;

void main() {
    // 1. CLIP-SPACE POSITIONING
    // Projects the vertex into the specific viewport quadrant
    gl_Position = push.mvp * vec4(inPosition, 1.0);

    // 2. WORLD-SPACE RECONSTRUCTION
    // FIX: Restore 3D depth by transforming position and normals into world space
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz; 
    
    // Transform normal to world space using the Normal Matrix for correct 3D shading
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    
    fragTexCoord = inTexCoord;
    
    // 3. SHADOW COORDINATE CALCULATION
    // Transforms the world position into the light's perspective for shadow sampling
    fragPosLightSpace = ubo.lightSpaceMatrix * worldPos;

    // 4. GOURAUD LIGHTING FALLBACK
    if (ubo.useGouraud == 1) {
        vec3 L = normalize(ubo.lightPos - fragPos);
        float diff = max(dot(normalize(fragNormal), L), 0.2);
        fragGouraudColor = ubo.lightColor * diff;
    }
}