#version 450

// --- Interpolated Inputs ---
layout(location = 0) in vec3 fragPos;
layout(location = 1) in vec3 fragColor;
layout(location = 2) in vec3 fragNormal;

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
    int useGouraud;
    float time;
    SparkLight sparks[4];
} ubo;

// NOTE: No Set 1 binding — flat color uses vertex color as albedo directly.

// --- Outputs ---
layout(location = 0) out vec4 outColor;

void main() {
    vec3 albedo  = fragColor;
    vec3 N       = normalize(fragNormal);
    vec3 L       = normalize(ubo.lightPos - fragPos);
    vec3 V       = normalize(ubo.viewPos  - fragPos);
    vec3 H       = normalize(L + V);

    float ambient  = 0.15;
    float diffuse  = max(dot(N, L), 0.0);
    float specular = pow(max(dot(N, H), 0.0), 32.0) * 0.3;

    vec3 result = albedo * (ambient + diffuse * ubo.lightColor) + specular * ubo.lightColor;
    outColor = vec4(result, 1.0);
}
