#version 450

/**
 * @file checkerboard.frag
 * @brief Fragment shader for the procedural checkerboard material.
 *
 * Generates a procedural two-colour checkerboard pattern from the object-space
 * position, then applies Blinn-Phong lighting (identical to phong.frag) so the
 * pattern responds correctly to the scene light and casts shadows.
 *
 * colorA/colorB and the tiling scale are configurable at runtime via push
 * constants, allowing direct ImGui control from GenericScenario::OnGUI().
 *
 * Fulfills Simulation Lab 2 Q2:
 *  "The light and dark colours for the checkerboard can be controlled via the
 *   material menu control."
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in vec3 fragGouraudColor;
layout(location = 4) in vec4 fragPosLightSpace;
layout(location = 5) in vec3 fragModelPos;    // Object-space — drives checker pattern

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- Uniform Interfaces ---
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

layout(set = 0, binding = 1) uniform sampler2D shadowMap;

// --- Push Constants ---
layout(push_constant) uniform PushConstants {
    mat4  model;    // offset 0  — used by vert (frag reads colour fields below)
    vec4  colorA;   // offset 64 — Light square colour
    vec4  colorB;   // offset 80 — Dark square colour
    float scale;    // offset 96 — Tiling density (squares per unit)
} push;

// --- Output ---
layout(location = 0) out vec4 outColor;

// --- Shadow Helper ---
float calculateShadow(vec4 posLightSpace) {
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    projCoords.xy   = projCoords.xy * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.002;
    return (currentDepth - bias > closestDepth) ? 0.4 : 1.0;
}

// --- Procedural Checkerboard ---
// Tiles in model space so the pattern stays stable as the object rotates.
// mod(floor(x) + floor(y) + floor(z), 2) alternates 0/1 between squares.
vec3 checkerColor() {
    float cx = floor(fragModelPos.x * push.scale);
    float cy = floor(fragModelPos.y * push.scale);
    float cz = floor(fragModelPos.z * push.scale);
    float check = mod(cx + cy + cz, 2.0);
    return mix(push.colorB.rgb, push.colorA.rgb, check);
}

void main() {
    // 1. Procedural albedo from checker pattern
    vec3  albedo = checkerColor();
    float shadow = calculateShadow(fragPosLightSpace);

    // 2. Ambient
    vec3 ambientResult = albedo * (ubo.lightColor * 0.05);

    if (ubo.useGouraud == 1) {
        // Gouraud fallback
        outColor = vec4(ambientResult + albedo * fragGouraudColor * shadow, 1.0);
    } else {
        // Blinn-Phong
        vec3  N = normalize(fragNormal);
        vec3  L = normalize(ubo.lightPos - fragPos);
        vec3  V = normalize(ubo.viewPos  - fragPos);
        vec3  H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);
        float spec = pow(max(dot(N, H), 0.0), 64.0) * 0.3;

        vec3 diffuseResult  = albedo * diff * ubo.lightColor * shadow;
        vec3 specularResult = vec3(1.0) * spec * ubo.lightColor * shadow;

        outColor = vec4(ambientResult + diffuseResult + specularResult, 1.0);
    }
}
