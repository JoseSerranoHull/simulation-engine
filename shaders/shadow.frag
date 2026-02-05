#version 450

/**
 * @file shadow.frag
 * @brief Fragment shader for shadow map generation with alpha testing.
 *
 * This shader is used during the shadow pass to handle semi-transparent 
 * or masked materials (like foliage or glass). It performs an alpha test 
 * to ensure that only opaque parts of a texture cast shadows.
 */

// --- Interpolated Inputs (from shadow.vert) ---
layout(location = 0) in vec2 fragTexCoord;

// --- Set 1: Material Textures ---
layout(set = 1, binding = 0) uniform sampler2D texSampler;

void main() {
    // 1. ALPHA SAMPLING
    // Sample the alpha channel of the material's primary albedo texture.
    float alpha = texture(texSampler, fragTexCoord).a;

    // 2. ALPHA TEST (Discard Logic)
    // If the pixel is transparent (below the 0.5 threshold), discard the fragment.
    // This prevents transparent areas from writing to the depth/shadow map.
    if (alpha < 0.5) {
        discard;
    }
}