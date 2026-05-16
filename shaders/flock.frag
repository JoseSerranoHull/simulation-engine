#version 450

/**
 * @file flock.frag
 * @brief Fragment shader for GPU-driven boid point-sprite rendering.
 *
 * Discards fragments outside a circle boundary to produce round sprites.
 * Applies a smooth alpha falloff at the edge and colours the boid based on
 * speed: slow boids are cool blue; fast boids are warm orange-red.
 */

// --- Inputs ---
layout(location = 0) in float fragSpeed;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    // Re-centre gl_PointCoord from [0,1] to [-0.5, 0.5]
    vec2  coord = gl_PointCoord - vec2(0.5);
    float dist  = length(coord);

    // Discard fragments outside the circle
    if (dist > 0.5) { discard; }

    // Soft alpha edge falloff
    float alpha = smoothstep(0.5, 0.2, dist);

    // Speed-based colour ramp: slow = blue, fast = orange-red
    float t    = clamp(fragSpeed / 6.0, 0.0, 1.0);
    vec3  col  = mix(vec3(0.2, 0.55, 1.0), vec3(1.0, 0.35, 0.05), t);

    // Bright core highlight
    float core = pow(1.0 - dist * 2.0, 4.0) * 0.4;
    col += vec3(core);

    outColor = vec4(col, alpha);
}
