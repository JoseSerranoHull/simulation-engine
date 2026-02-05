#version 450

/**
 * @file dust.frag
 * @brief Fragment shader for dust vortex particles.
 * * This shader converts square point primitives into soft-edged circular clouds.
 * It implements "Soft Particle" depth-testing, which fades particles as they 
 * approach solid geometry to prevent sharp intersections.
 */

// --- Inputs (Interpolated from dust.vert) ---
layout(location = 0) in vec4 fragColor;

// --- Uniform Interfaces ---
layout(location = 0) out vec4 outColor;

/** * @brief Depth reference.
 * Note: Reuses the shadow map binding as a depth reference for soft-blending logic.
 */
layout(set = 0, binding = 1) uniform sampler2D depthMap;

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
    SparkLight sparks[4];
} ubo;

void main() {
    // 1. CIRCULAR SHAPING
    // gl_PointCoord provides coordinates [0, 1] across the point primitive.
    // We shift it to [-0.5, 0.5] to calculate distance from the center.
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    // Hard clip outside the radius to maintain circularity
    if (dist > 0.5) discard;

    // 2. RADIAL GRADIENT (Cloud Density)
    // Using a power of 2.0 creates a "fluffy" falloff that is denser at the center.
    float softAlpha = pow(smoothstep(0.5, 0.0, dist), 2.0);
    
    // 3. SOFT PARTICLE BLENDING
    // This prevents particles from "cutting" through the sand terrain.
    // Calculate the screen-space UV to sample the current scene depth.
    vec2 screenUV = gl_FragCoord.xy / textureSize(depthMap, 0);
    float sceneDepth = texture(depthMap, screenUV).r;
    float particleDepth = gl_FragCoord.z;

    // Fade Distance: Larger values (0.01) create a smoother transition near geometry.
    float fadeDistance = 0.01; 
    float diff = sceneDepth - particleDepth;
    
    // If the particle is behind geometry (diff < 0), it is hidden.
    // If it is near geometry, it fades out using smoothstep.
    float softness = smoothstep(0.0, fadeDistance, diff);

    // 4. FINAL COMPOSITION
    // Base Color (from vertex) * Radial Shape * Depth Softness * Global Density Multiplier.
    // The 0.25 multiplier ensures that overlapping dust clouds don't become oversaturated.
    outColor = vec4(fragColor.rgb, fragColor.a * softAlpha * softness * 0.25);
    
    // Alpha Discard: Prevents invisible fragments from writing to the frame buffer.
    if (outColor.a < 0.001) discard;
}