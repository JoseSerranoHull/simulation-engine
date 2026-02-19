#version 450

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 lightPos;
    vec3 viewPos;
    vec3 lightColor;
    int useGouraud;
    float time;
    // Padding to match spark light array if necessary
    vec4 sparks[10]; 
    
    // --- NEW: Procedural Checkerboard Colors ---
    vec4 checkColorA; 
    vec4 checkColorB;
} ubo;

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

void main() {
    // 1. Calculate Checkerboard Pattern
    // Tiling factor of 10.0 matches your previous .ini settings
    float tiling = 10.0;
    vec2 pos = floor(fragTexCoord * tiling);
    float pattern = mod(pos.x + pos.y, 2.0);

    // 2. Select Colors from UBO
    vec3 baseColor = (pattern > 0.5) ? ubo.checkColorA.rgb : ubo.checkColorB.rgb;

    // 3. Basic Lighting (Lambertian)
    vec3 N = normalize(fragNormal);
    vec3 L = normalize(ubo.lightPos - fragPos);
    float diff = max(dot(N, L), 0.0);
    
    // Add ambient component so the dark side isn't pitch black
    vec3 ambient = 0.2 * baseColor;
    vec3 diffuse = diff * baseColor * ubo.lightColor;

    outColor = vec4(ambient + diffuse, 1.0);
}