#version 450

/**
 * @file shader.vert
 * @brief Basic vertex shader for environmental assets.
 *
 * This shader transforms vertex positions into clip space and world space.
 * It also prepares texture coordinates and world-space normals for use 
 * in the fragment stage.
 */

// --- Inputs (Vertex Attributes) ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec3 fragPos;          // World Position
layout(location = 1) out vec2 fragTexCoord;     // UV coordinates
layout(location = 2) out vec3 fragNormal;       // World Normals
layout(location = 3) out vec3 fragGouraudColor; // Fallback color pass

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
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
    SparkLight sparks[4]; // Must match C++ exactly
} ubo;

// --- Push Constants ---
layout(push_constant) uniform PushConstants { 
    mat4 model; 
} push;

void main() {
    // 1. GEOMETRY TRANSFORMATION
    // Transform position to world-space and clip-space.
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;
    
    // 2. DATA PASSTHROUGH
    // Prepare attributes for interpolation in the fragment stage.
    fragPos = vec3(worldPos);
    fragTexCoord = inTexCoord;

    // 3. NORMAL RECONSTRUCTION
    // Compute world-space normal using the normal matrix (transpose inverse model).
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;

    // 4. FALLBACK INITIALIZATION
    // Assign dummy value for Gouraud compatibility.
    fragGouraudColor = vec3(1.0); 
}