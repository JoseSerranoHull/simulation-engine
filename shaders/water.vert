#version 450

/**
 * @file water.vert
 * @brief Vertex shader for animated water with 128-byte Push Constants (MVP + Model).
 */

// --- Inputs ---
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

// --- Outputs ---
layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec3 fragGouraudColor;

// --- Data Structures ---
struct SparkLight {
    vec4 position; // 16-byte aligned
    vec4 color;    
};

// --- Uniform Data (Global Engine State) ---
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int useGouraud;
    float time;
    
    SparkLight sparks[10]; 
    vec4 checkColorA;
    vec4 checkColorB;
} ubo;

// --- Push Constants ---
/** * Fulfills Step 2: 128-byte block containing both matrices.
 * mvp: Projection * View * Model (Quadrant specific)
 * model: Raw World Matrix for lighting and wave consistency
 */
layout(push_constant) uniform PushConstants { 
    mat4 mvp;   
    mat4 model; 
} push;

void main() {
    // 1. WAVE ANIMATION LOGIC
    // Displacement stays in model space for local ripple consistency
    vec3 pos = inPosition;
    float wave = sin(pos.x * 10.0 + ubo.time * 2.0) * 0.02;
    pos.y += wave;

    // 2. CLIP-SPACE POSITIONING
    // Projects the displaced vertex into the specific viewport quadrant
    gl_Position = push.mvp * vec4(pos, 1.0);

    // 3. WORLD-SPACE RECONSTRUCTION
    // FIX: Restore 3D depth by transforming position and normals into world space
    vec4 worldPos = push.model * vec4(pos, 1.0);
    fragPos = worldPos.xyz;
    
    // Normal Matrix transformation for accurate reflections
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal;
    
    fragTexCoord = inTexCoord;

    // 4. FALLBACK INITIALIZATION
    fragGouraudColor = vec3(1.0);
}