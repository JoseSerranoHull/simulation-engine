#version 450

/**
 * @file gouraud.vert
 * @brief Vertex shader implementing the Gouraud lighting model.
 *
 * This shader performs per-vertex lighting calculations (Ambient, Diffuse, Specular)
 * and passes the resulting color to the fragment stage. This reduces fragment 
 * overhead by interpolating light intensity across the face.
 */

// --- Inputs (Vertex Attributes) ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
// Output COLOR directly to fragment shader
layout(location = 0) out vec3 vColor;
layout(location = 1) out vec2 vTexCoord;

// --- Data Structures ---
struct SparkLight {
    vec3 position;
    vec3 color;
};

// --- Set 0: Global Data ---
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

// --- Push Constants ---
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    // 1. GEOMETRY CALCULATIONS
    // Transform position and normal into world-space.
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    vec3 fragPos = vec3(worldPos);
    vec3 norm = normalize(mat3(transpose(inverse(push.model))) * inNormal);

    // 2. LIGHTING CALCULATIONS (Per-Vertex)
    
    // Ambient component
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * ubo.lightColor;

    // Diffuse component
    vec3 lightDir = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * ubo.lightColor;

    // Specular component
    float specularStrength = 0.5;
    vec3 viewDir = normalize(ubo.viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * ubo.lightColor;

    // 3. FINAL OUTPUT DATA
    // Combine lighting components and pass attributes.
    vColor = ambient + diffuse + specular;
    vTexCoord = inTexCoord;

    // 4. CLIP SPACE TRANSFORMATION
    gl_Position = ubo.proj * ubo.view * worldPos;
}