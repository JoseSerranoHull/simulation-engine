#version 450

// Input from the SSBO (Same structure as C++)
layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVelocity; // Unused for drawing, but needed for layout
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

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
    int useGouraud;
    float time;
    SparkLight sparks[4]; // Must match C++ exactly
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * vec4(inPosition.xyz, 1.0);
    gl_PointSize = inPosition.w; // Use the 'w' component as size
    fragColor = inColor;
}