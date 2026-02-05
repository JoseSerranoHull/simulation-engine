#version 450

/**
 * @file rain.vert
 * @brief Vertex shader for the Rain particle system.
 *
 * Transforms simulated rain particle positions into clip space and calculates
 * perspective-accurate point sizes. The point size is attenuated based on 
 * distance from the camera to ensure the streaks appear thin and needle-like.
 */

// --- Inputs (Directly from the Storage Buffer / Vertex Input) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Life/Age
layout(location = 2) in vec4 inColor;    // Color calculated in compute shader

// --- Outputs ---
layout(location = 0) out vec4 fragColor;

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
    SparkLight sparks[4]; // Matches C++ EngineConstants::MAX_SPARKS
} ubo;

void main() {
    // 1. WORLD TO VIEW SPACE TRANSFORMATION
    vec4 viewPos = ubo.view * vec4(inPosition.xyz, 1.0);

    // 2. VIEW TO CLIP SPACE TRANSFORMATION
    gl_Position = ubo.proj * viewPos;

    // 3. PERSPECTIVE POINT ATTENUATION
    // Calculate distance from the camera in view-space to scale point size correctly.
    float dist = length(viewPos.xyz);

    /**
     * @brief Point Size Calculation
     * Perspective Scaling: Size = BaseSize * (1.0 / distance)
     * MULTIPLIER (12.0): Specifically tuned for rain to keep streaks thin.
     * (Compared to 25.0 used for fluffy snow particles).
     */
    float multiplier = 12.0;
    gl_PointSize = inPosition.w * (1.0 / dist) * multiplier;

    // 4. DATA PASSTHROUGH
    fragColor = inColor;
}