#version 450

/**
 * @file checkerboard.vert
 * @brief Vertex shader for the procedural checkerboard material.
 *
 * Identical to phong.vert in its transformation pipeline, but additionally
 * passes the object-space (local) position to the fragment shader so the
 * checkerboard pattern tiles in model space and remains stable under rotation.
 */

// --- Inputs (Vertex Attributes) ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec3 fragPos;         // World-space position
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragGouraudColor;
layout(location = 4) out vec4 fragPosLightSpace;
layout(location = 5) out vec3 fragModelPos;    // Object-space position for checker pattern

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
    int  useGouraud;
    float time;
    SparkLight sparks[4];
} ubo;

// --- Push Constants ---
// NOTE: The checkerboard frag shader extends this block with colorA/colorB/scale.
// The vert shader only reads the model matrix (first 64 bytes).
layout(push_constant) uniform PushConstants {
    mat4  model;    // offset 0  — used by vert
    vec4  colorA;   // offset 64 — used by frag
    vec4  colorB;   // offset 80 — used by frag
    float scale;    // offset 96 — used by frag
} push;

void main() {
    // 1. GEOMETRY TRANSFORMATION
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    // 2. DATA PASSTHROUGH
    fragPos          = vec3(worldPos);
    fragTexCoord     = inTexCoord;
    fragNormal       = mat3(transpose(inverse(push.model))) * inNormal;
    fragPosLightSpace = ubo.lightSpaceMatrix * worldPos;

    // 3. Object-space position for stable checkerboard tiling
    fragModelPos = inPosition;

    // 4. GOURAUD LIGHTING FALLBACK (parity with phong.vert)
    fragGouraudColor = vec3(0.0);
    if (ubo.useGouraud == 1) {
        vec3 L = normalize(ubo.lightPos - fragPos);
        float diff = max(dot(normalize(fragNormal), L), 0.2);
        fragGouraudColor = ubo.lightColor * diff;
    }
}
