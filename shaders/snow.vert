#version 450

/**
 * @file snow.vert
 * @brief Vertex shader for the Snow particle system.
 *
 * Transforms simulated snow particle positions into clip space and calculates
 * perspective-accurate point sizes. The point size is attenuated based on 
 * distance from the camera, with a specifically tuned multiplier to ensure 
 * the flakes appear delicate rather than overly puffy.
 */

// --- Inputs (Directly from the Storage Buffer / Vertex Input) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Life/Age (Unused here)
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
    // 1. POSITION TRANSFORMATION
    // Transform the particle position into view-space and then to clip-space.
    vec4 viewPos = ubo.view * vec4(inPosition.xyz, 1.0);
    gl_Position = ubo.proj * viewPos;

    // 2. PERSPECTIVE POINT ATTENUATION
    // Calculate distance from the camera in view-space to scale point size correctly.
    float dist = length(viewPos.xyz);

    /**
     * @brief Point Size Calculation
     * Perspective Scaling: Size = BaseSize * (1.0 / distance)
     * MULTIPLIER (8.0): Tuned specifically for snow to avoid an overly 
     * "puffy" look, creating a finer, more realistic precipitation effect.
     */
    float multiplier = 8.0; 
    gl_PointSize = inPosition.w * (1.0 / dist) * multiplier;

    // 3. DATA PASSTHROUGH
    // Pass the blue-tinted color to the fragment stage.
    fragColor = inColor; 
}