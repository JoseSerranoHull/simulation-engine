#version 450

/**
 * @file smoke.frag
 * @brief Fragment shader for the Smoke particle system.
 *
 * This shader converts square point primitives into soft, billowing soot clouds.
 * It utilizes a cubic radial falloff for "fluffy" density and a linear age-based 
 * fade to simulate smoke dissipating as it rises.
 */

// --- Inputs (Interpolated from smoke.vert) ---
layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragAge;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. CIRCULAR SHAPING
    // gl_PointCoord provides coordinates [0, 1] across the point primitive.
    // We shift it to [-0.5, 0.5] to calculate distance from the center.
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    // Hard clip outside the radius to maintain perfect circularity.
    if (dist > 0.5) discard;

    // 2. RADIAL FALLOFF (Soot Density)
    // Using a power of 3.0 creates a "heavy" falloff that is denser at the center.
    float alpha = pow(smoothstep(0.5, 0.0, dist), 3.0);
    
    // 3. DISSIPATION OVER TIME
    // Fades the particle based on its normalized age (fragAge).
    float lifeFade = 1.0 - fragAge;

    // 4. FINAL COMPOSITION
    // Output dark soot color (Blackened) with a global 0.6 density multiplier.
    outColor = vec4(fragColor.rgb, fragColor.a * alpha * lifeFade * 0.6);
}