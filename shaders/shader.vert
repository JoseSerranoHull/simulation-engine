#version 450

/**
 * @file shader.vert
 * @brief Basic vertex shader updated for 128-byte Push Constants (MVP + Model).
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

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
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
    int useGouraud;
    float time;
    
    // Updated to match Common.h exactly
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte block containing both matrices.
 * This layout is now agnostic and used by all mesh shaders.
 */
layout(push_constant) uniform PushConstants { 
    mat4 mvp;   // View-Projection * Model (Quadrant specific)
    mat4 model; // Raw World Matrix for 3D depth fix
} push;

void main() {
    // 1. CLIP-SPACE POSITIONING
    // Projects the vertex into the specific viewport quadrant
    gl_Position = push.mvp * vec4(inPosition, 1.0);
    
    // 2. WORLD-SPACE RECONSTRUCTION
    // FIX: Restore 3D volume by transforming to world coordinates
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    fragPos = worldPos.xyz;
    
    // Transform normal to world space for dynamic lighting
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    
    fragTexCoord = inTexCoord;

    // 3. FALLBACK INITIALIZATION
    fragGouraudColor = vec3(1.0); 
}