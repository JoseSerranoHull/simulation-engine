#version 450

/**
 * @file dust.vert
 * @brief Vertex shader for the Dust Vortex particle system.
 * * Transforms simulated particle positions into clip space and calculates
 * perspective-accurate point sizes. This shader reads directly from the 
 * Storage Buffer populated by the compute stage.
 */

// --- Inputs (Directly from the Storage Buffer / Vertex Input) ---
// Note: Location 1 (inVelocity) is kept to maintain layout parity with the Particle struct
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Simulated Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Particle Age (Unused here)
layout(location = 2) in vec4 inColor;    // Color calculated in compute shader

// --- Outputs ---
layout(location = 0) out vec4 fragColor;

// --- Uniform Data (Global Engine State) ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    SparkLight sparks[4]; // Matches C++ EngineConstants::MAX_SPARKS
} ubo;

void main() {
    // 1. WORLD TO VIEW SPACE TRANSFORMATION
    vec4 viewPos = ubo.view * vec4(inPosition.xyz, 1.0);
    
    // 2. VIEW TO CLIP SPACE TRANSFORMATION
    gl_Position = ubo.proj * viewPos;

    // 3. PERSPECTIVE POINT ATTENUATION
    // Calculate distance from the camera in view-space
    float dist = length(viewPos.xyz);

    /**
     * @brief Point Size Calculation
     * Perspective Scaling: Size = BaseSize * (1.0 / distance)
     * HABOOB MULTIPLIER (10.0): Adjusted to maintain the "dusty" density within the globe.
     * Higher multipliers result in larger "fluffy" billboards, while lower values look like fine sand.
     */
    gl_PointSize = inPosition.w * (1.0 / dist) * 10.0;

    // Pass the compute-generated color (including alpha fade) to the fragment stage
    fragColor = inColor; 
}