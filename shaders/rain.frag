#version 450

/**
 * @file rain.frag
 * @brief Fragment shader for the Rain particle system.
 *
 * This shader converts square point primitives into thin, translucent 
 * vertical streaks to simulate rain. It utilizes non-uniform coordinate 
 * scaling to shape the point sprites into "needles" with a soft radial falloff.
 */

// --- Inputs (Interpolated from rain.vert) ---
layout(location = 0) in vec4 fragColor;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // 1. COORDINATE RE-CENTERING
    // Shift gl_PointCoord from [0, 1] to [-0.5, 0.5] for centered calculations.
    vec2 coord = gl_PointCoord - vec2(0.5);
    
    // 2. STREAK MASK GENERATION
    // Compress the X-axis by 8.0x to transform the circle into a thin vertical needle.
    float streak = length(coord * vec2(8.0, 1.0)); 
    
    // Hard discard for corners outside the needle radius.
    if (streak > 0.5) discard;

    // 3. SOFT EDGE FALLOFF
    // Creates translucency at the edges of the drop for a smoother look.
    float softEdge = smoothstep(0.5, 0.1, streak);
    
    // 4. FINAL COMPOSITION
    // Output the tinted drop with a base alpha multiplier of 0.4.
    outColor = vec4(fragColor.rgb, fragColor.a * softEdge * 0.4);
}