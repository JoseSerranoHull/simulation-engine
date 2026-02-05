#version 450

/**
 * @file water.vert
 * @brief Vertex shader for animated water surfaces.
 *
 * Implements procedural wave displacement using a sine wave function based 
 * on world-space coordinates and global time. It prepares world-space 
 * positions and normals for refraction and reflection in the fragment stage.
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
    // 1. WAVE ANIMATION LOGIC
    // Apply a simple vertical displacement (Sine wave) to simulate surface ripples.
    vec3 pos = inPosition;
    float wave = sin(pos.x * 10.0 + ubo.time * 2.0) * 0.02;
    pos.y += wave;

    // 2. GEOMETRY TRANSFORMATION
    // Transform the displaced vertex into world-space and clip-space.
    vec4 worldPos = push.model * vec4(pos, 1.0);
    gl_Position = ubo.proj * ubo.view * worldPos;

    // 3. DATA PASSTHROUGH
    // Prepare attributes for interpolation in the fragment stage.
    fragPos = vec3(worldPos);
    fragTexCoord = inTexCoord;

    // 4. NORMAL RECONSTRUCTION
    // Note: This calculates the normal based on the original mesh orientation.
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;

    // 5. FALLBACK INITIALIZATION
    fragGouraudColor = vec3(1.0);
}