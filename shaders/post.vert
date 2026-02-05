#version 450

/**
 * @file post.vert
 * @brief Vertex shader for full-screen post-processing effects.
 *
 * This shader utilizes a procedural triangle generation technique to cover 
 * the entire viewport without requiring a vertex buffer. It calculates 
 * Normalized Device Coordinates (NDC) and corresponding texture UVs.
 */

// --- Outputs ---
layout(location = 0) out vec2 outUV;

void main() {
    // 1. PROCEDURAL FULL-SCREEN TRIANGLE GENERATION
    // Generates a single large triangle that covers the entire screen.
    // Vertices: (0,0), (2,0), (0,2) in UV space.
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    
    // 2. COORDINATE TRANSFORMATION
    // Maps UV coordinates [0, 2] to NDC space [-1, 1].
    // Z is set to 0.0 as post-processing is a 2D overlay operation.
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}