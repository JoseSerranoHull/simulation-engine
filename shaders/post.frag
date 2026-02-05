#version 450

/**
 * @file post.frag
 * @brief Post-processing fragment shader for the final image output.
 *
 * This shader performs high-level image effects on the resolved HDR scene, 
 * including a soft-knee bloom with a 9-tap Gaussian-style blur, 
 * Reinhard tone mapping to convert HDR to SDR, and gamma correction.
 */

// --- Interpolated Inputs ---
layout(location = 0) in vec2 inUV;

// --- Outputs ---
layout(location = 0) out vec4 outColor;

/** * @brief Scene Sampler
 * Set 0, Binding 0: This is the 1x Resolved HDR Scene provided by the post-processor.
 */
layout(set = 0, binding = 0) uniform sampler2D sceneSampler;

// --- Push Constants ---
layout(push_constant) uniform PushConsts {
    int enableBloom;
} push;

/**
 * @brief Performs a robust 9-tap blur to prevent blocky bloom artifacts.
 * @param tex The input texture sampler.
 * @param uv The current fragment texture coordinates.
 * @return The blurred RGB result.
 */
vec3 blur(sampler2D tex, vec2 uv) {
    vec3 result = vec3(0.0);
    vec2 texelSize = 1.0 / textureSize(tex, 0);
    
    // Weighted Gaussian-style 9-tap kernel
    float kernel[9] = float[](
        1.0/16.0, 2.0/16.0, 1.0/16.0,
        2.0/16.0, 4.0/16.0, 2.0/16.0,
        1.0/16.0, 2.0/16.0, 1.0/16.0
    );

    vec2 offsets[9] = vec2[](
        vec2(-1, -1), vec2(0, -1), vec2(1, -1),
        vec2(-1,  0), vec2(0,  0), vec2(1,  0),
        vec2(-1,  1), vec2(0,  1), vec2(1,  1)
    );

    for(int i = 0; i < 9; i++) {
        result += texture(tex, uv + (offsets[i] * texelSize * 2.0)).rgb * kernel[i];
    }
    
    return result;
}

void main() {
    // 1. SCENE SAMPLING
    // Sample the resolved HDR scene.
    vec3 sceneColor = texture(sceneSampler, inUV).rgb;
    vec3 result = sceneColor;

    if (push.enableBloom == 1) {
        // 2. SOFT-KNEE BLOOM (Improved thresholding)
        // Check for highlights crossing the 0.7 threshold.
        float brightness = dot(sceneColor, vec3(0.2126, 0.7152, 0.0722));
        
        if(brightness > 0.7) {
            // Apply a wider 9-tap blur to spread the light.
            vec3 bloomColor = blur(sceneSampler, inUV);
            
            // STRENGTH BOOST: 1.5 multiplier used for noticeable luminosity.
            result += bloomColor * 1.5; 
        }
    }

    // 3. REINHARD TONE MAPPING
    // Maps HDR intensity values to the SDR [0, 1] range.
    vec3 mapped = result / (result + vec3(1.0));
    
    // 4. GAMMA CORRECTION
    // Converts linear color space to sRGB for display compatibility.
    mapped = pow(mapped, vec3(1.0 / 2.2));
    
    outColor = vec4(mapped, 1.0);
}