#version 450

/**
 * @file gouraud.vert
 * @brief Vertex shader implementing Gouraud lighting with 128-byte Push Constants.
 */

// --- Inputs ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vTexCoord;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    // 16-byte aligned
};

// --- Set 0: Global Data ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int  useGouraud;
    float time;
    
    // Aligned with updated Common.h
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
// Fulfills Step 2: 128-byte block containing both matrices
layout(push_constant) uniform PushConstants {
    mat4 mvp;   // View-Projection * Model (Quadrant specific)
    mat4 model; // Raw World Matrix for 3D depth fix
} push;

void main() {
    // 1. CLIP SPACE TRANSFORMATION
    // Projects vertex into the specific split-screen quadrant
    gl_Position = push.mvp * vec4(inPosition, 1.0);

    // 2. WORLD SPACE RECONSTRUCTION
    // FIX: Restore 3D depth by calculating lighting in world space
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    vec3 worldNormal = normalize(mat3(transpose(inverse(push.model))) * inNormal);

    // 3. GOURAUD LIGHTING CALCULATIONS (Per-Vertex)
    // Ambient component
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;

    // Diffuse component
    vec3 lightDir = normalize(ubo.lightPos - worldPos.xyz);
    float diff = max(dot(worldNormal, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    // Specular component
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ubo.viewPos - worldPos.xyz);
    vec3 reflectDir = reflect(-lightDir, worldNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * ubo.lightColor;

    // 4. FINAL OUTPUT DATA
    vColor = ambient + diffuse + specular;
    vTexCoord = inTexCoord;
}