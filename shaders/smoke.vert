#version 450

/**
 * @file smoke.vert
 * @brief Vertex shader for the Smoke particle system.
 *
 * Transforms simulated smoke particle positions into clip space and calculates
 * perspective-accurate point sizes. It passes the normalized particle age to 
 * the fragment stage to facilitate dissipation effects.
 */

// --- Inputs (Directly from the Storage Buffer / Vertex Input) ---
layout(location = 0) in vec4 inPosition; // xyz = World Position, w = Base Size
layout(location = 1) in vec4 inVelocity; // xyz = Velocity, w = Particle Age
layout(location = 2) in vec4 inColor;    // Color calculated in compute shader

// --- Outputs ---
layout(location = 0) out vec4 fragColor;
layout(location = 1) out float fragAge; // Passes life/age to the fragment stage

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
    int  useGouraud;
    float time;
    SparkLight sparks[4]; // Matches C++ EngineConstants::MAX_SPARKS
} ubo;

void main() {
    // 1. POSITION TRANSFORMATION
    // Transform the particle into view-space and then to clip-space.
    vec4 viewPos = ubo.view * vec4(inPosition.xyz, 1.0);
    gl_Position = ubo.proj * viewPos;

    // 2. POINT SIZE ATTENUATION
    // Calculate distance from the camera to scale the point size accurately.
    float dist = length(viewPos.xyz);
    
    /**
     * @brief Point Size Calculation
     * Perspective Scaling: Size = BaseSize * (1.0 / distance)
     * MULTIPLIER (30.0): Adjusted to make smoke look voluminous and billowy 
     * without reaching the massive scale of the Haboob dust clouds.
     */
    float multiplier = 30.0; 
    gl_PointSize = inPosition.w * (1.0 / dist) * multiplier; 

    // 3. DATA PASSTHROUGH
    // Sending color and normalized age (0.0 to 1.0) for fragment fade-out logic.
    fragColor = inColor;
    fragAge = inVelocity.w;
}