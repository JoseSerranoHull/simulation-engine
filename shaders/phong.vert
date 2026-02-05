#version 450

/**
 * @file phong.vert
 * @brief Primary vertex shader for scene objects.
 *
 * Transforms vertex positions into clip space and world space. It calculates 
 * surface normals and prepares light-space coordinates for shadow mapping. 
 * Additionally, it provides a per-vertex lighting fallback for Gouraud shading.
 */

// --- Inputs (Vertex Attributes) ---
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
    vec3 position;
    vec3 color;
};

// --- Uniform Interfaces ---
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
    // Prepare world-space attributes for the fragment stage.
    fragPos = vec3(worldPos);
    fragTexCoord = inTexCoord;
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    
    // 3. SHADOW COORDINATE CALCULATION
    // Transform position into light-perspective for shadow sampling.
    fragPosLightSpace = ubo.lightSpaceMatrix * worldPos;

    // 4. GOURAUD LIGHTING FALLBACK
    // Calculate per-vertex diffuse lighting if Gouraud mode is enabled.
    if (ubo.useGouraud == 1) {
        vec3 L = normalize(ubo.lightPos - fragPos);
        float diff = max(dot(normalize(fragNormal), L), 0.2);
        fragGouraudColor = ubo.lightColor * diff;
    }
}