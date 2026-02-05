#version 450

/**
 * @file fire.frag
 * @brief Fragment shader for the Bonfire particle system.
 *
 * This shader converts square point primitives into soft-edged, high-intensity 
 * fire embers. It implements a dual-layer radial falloff and an exponential 
 * HDR boost to simulate intense luminosity.
 */

// --- Inputs (Interpolated from fire.vert) ---
layout(location = 0) in vec4 fragColor;
layout(location = 1) in float fragLife;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. CIRCULAR SHAPING
    // gl_PointCoord goes from 0.0 to 1.0 across the point quad.
    // We shift it to [-0.5, 0.5] to calculate distance from the center.
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    // CRITICAL: Hard radial mask to remove square corners.
    if (dist > 0.5) discard;

    // 2. DUAL-LAYER FALLOFF
    // Improved Dual-Layer Falloff for fire density.
    float core      = exp(-dist * dist * 30.0); // Tighter core
    float wisps     = exp(-dist * dist * 8.0) * 0.4;
    float softAlpha = (core + wisps);

    // 3. EXPONENTIAL HDR BOOST
    // Restores the high-luminosity appearance (Screenshot 2026-01-02 look).
    vec3 fireColor = fragColor.rgb;
    
    // Boost intensity based on proximity to center (core)
    fireColor *= (1.0 + core * 5.0); 

    // 4. FINAL COMPOSITION
    outColor = vec4(fireColor, fragColor.a * softAlpha * 1.5);
}