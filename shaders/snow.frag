#version 450

/**
 * @file snow.frag
 * @brief Fragment shader for the Snow particle system.
 *
 * This shader converts square point primitives into soft, fluffy snowflakes. 
 * It utilizes a wide smoothstep for feathered edges and a high-power central 
 * highlight to simulate the crystalline reflective glimmer characteristic 
 * of falling snow.
 */

// --- Inputs (Interpolated from snow.vert) ---
layout(location = 0) in vec4 fragColor;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. COORDINATE RE-CENTERING
    // Shift gl_PointCoord from [0, 1] to [-0.5, 0.5] for radial calculations.
    vec2 coord = gl_PointCoord - vec2(0.5);
    float dist = length(coord);

    // Hard clip outside the circle boundary.
    if (dist > 0.5) discard;

    // 2. FEATHERED ALPHA FALLOFF
    // Starts falling off early (0.05) to create a "fuzzy" or "fluffy" appearance.
    float softAlpha = smoothstep(0.5, 0.05, dist);
    
    // 3. CRYSTALLINE GLIMMER
    // A sharp, localized highlight in the center of the flake to simulate reflection.
    float glimmer = pow(1.0 - dist * 2.0, 6.0) * 0.3;

    // 4. FINAL COMPOSITION
    // Combine the base blue-tinted color with the glimmer highlight.
    outColor = vec4(fragColor.rgb + glimmer, fragColor.a * softAlpha);
}