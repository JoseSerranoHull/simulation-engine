# Final Lab Book: Real-Time Graphics (700106)

---

## MSc Computer Science for Games Programming

---

**Course**: 700106 - Real-Time Graphics

**Name**: JOSE JAVIER SERRANO SOLIS

**Instructors**: Dr. Qingde Li, Dr. Xihui Ma

---

## 1. Introduction: The Vulkan Sandy-Snow Globe

The "Sandy-Snow Globe" is a high-integrity graphics simulation built from the ground up using the Vulkan 1.3 API. This project focuses on the explicit management of the GPU lifecycle, manual memory orchestration, and custom shader-based physics. The core of the engine is designed around a multi-pass renderer capable of handling opaque environmental assets, complex GPU-driven particle simulations, and refractive translucent materials.
![Project Running](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img1.png> "Program running")

---

## 2. Milestone Registry

This project encompasses multiple distinct development milestones, each targeting a specific aspect of the graphics pipeline and simulation logic.

| Title | Core Requirement / Feature |
| --- | --- |
| **Vulkan Framework and Hardware Orchestration** | Foundation & SyncManager |
| **Geometry Representation (The Globe and Sand Plug)** | Procedural Generation |
| **Dual Shading Models (Gouraud vs. Phong)** | Toggleable Lighting Models |
| **Custom Model Loader (OBJ Parser)** | Wavefront Mesh Processing |
| **Environmental Simulation (Climate, Cacti, and Sun/Moon)** | Orbital Math & Growth |
| **GPU Compute Particle Systems (Rain and Dust)** | Compute Pipeline Kernels |
| **Shadow Mapping** | Multi-pass Depth Rendering |
| **Interaction (Cameras, Controls, and Config)** | Navigation & Data Loading |
| **Advanced Shadow Mapping (Alpha-Testing & Bias)** | Visual Polish & Artifact Fixes |
| **Translucent Globe & Refraction Snapshot** | Refractive Pass Orchestration |
| **HDR Pipeline & Reinhard Tone Mapping** | High-Precision Color Management |
| **GPU-Driven Turbulence & Soft Particles** | Physics Noise & Depth Blending |
| **Illuminating Sparks** | Compute-to-Graphics Light Bridge |
| **UI, Diagnostics, and Advanced Config** | IMGUI & Performance Telemetry |
| **Novel Features** | Custom VRAM Allocator, Multi-Material OBJ Groups, etc. |

---

## 3. Technical Implementation & Critique

### Part I: Engine Architecture & Data Orchestration

The foundation of the simulation involves a robust link between the CPU and GPU. In **Vulkan Framework and Hardware Orchestration**, I implemented a `SyncManager` to handle double-buffering. By using `VkFence` and `VkSemaphore`, the CPU can record commands for the next frame while the GPU renders the current one, preventing data races.

```cpp
// Logic from SyncManager.cpp showing initialization of sync primitives
void SyncManager::init(const VulkanContext* const ctx, uint32_t maxFrames, uint32_t imageCount) {
    commandBuffers.resize(maxFrames);
    imageAvailableSemaphores.resize(maxFrames);
    inFlightFences.resize(maxFrames);
    renderFinishedSemaphores.resize(imageCount);

    const VkSemaphoreCreateInfo semInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, nullptr, 0U };

    VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0U; i < maxFrames; ++i) {
        // Accessing ctx->device is allowed because ctx is a pointer to a constant VulkanContext.
        static_cast<void>(vkCreateSemaphore(ctx->device, &semInfo, nullptr, &imageAvailableSemaphores[i]));
        static_cast<void>(vkCreateFence(ctx->device, &fenceInfo, nullptr, &inFlightFences[i]));
    }

    for (uint32_t i = 0U; i < imageCount; ++i) {
        static_cast<void>(vkCreateSemaphore(ctx->device, &semInfo, nullptr, &renderFinishedSemaphores[i]));
    }
}
```

For **Manual VRAM Sub-allocation**, I implemented the `SimpleAllocator` to avoid the overhead of standard memory calls. It reserves a 256MB "Super-Block" and manages resources via bitwise alignment.

```cpp
// Logic from SimpleAllocator.cpp showing sub-allocation within the Super-Block
VkDeviceSize SimpleAllocator::allocate(const VkMemoryRequirements& requirements) {
    // Step 1: Calculate necessary padding to satisfy hardware alignment
    VkDeviceSize padding = VAL_ZERO;
    const VkDeviceSize remainder = currentOffset % requirements.alignment;

    if (remainder != VAL_ZERO) {
        padding = requirements.alignment - remainder;
    }

    const VkDeviceSize alignedOffset = currentOffset + padding;

    // Step 2: Ensure the super-block has sufficient capacity for the request
    if ((alignedOffset + requirements.size) > totalSize) {
        throw std::runtime_error("SimpleAllocator: VRAM Super-Block exhausted!");
    }

    // Step 3: Update the tracking offset and return the sub-allocation start point
    currentOffset = alignedOffset + requirements.size;
    return alignedOffset;
}
```

Data-driven design is handled through the **OBJ Parser**. The `ConfigLoader` parses `config.txt` to eliminate hard-coded transforms. By separating `Model` components into `Mesh`, `Material`, and `Texture`, the engine achieves high reusability.

```cpp
// From Vertex.h: Standard Interleaved Vertex Format descriptions
static std::array<VkVertexInputAttributeDescription, ATTRIBUTE_COUNT> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, ATTRIBUTE_COUNT> attributeDescriptions{};

        // Position: Location 0
        attributeDescriptions[LOC_POSITION].binding = BINDING_ZERO;
        attributeDescriptions[LOC_POSITION].location = LOC_POSITION;
        attributeDescriptions[LOC_POSITION].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_POSITION].offset = static_cast<uint32_t>(offsetof(Vertex, position));

        // Color: Location 1
        attributeDescriptions[LOC_COLOR].binding = BINDING_ZERO;
        attributeDescriptions[LOC_COLOR].location = LOC_COLOR;
        attributeDescriptions[LOC_COLOR].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_COLOR].offset = static_cast<uint32_t>(offsetof(Vertex, color));

        // TexCoord: Location 2
        attributeDescriptions[LOC_TEXCOORD].binding = BINDING_ZERO;
        attributeDescriptions[LOC_TEXCOORD].location = LOC_TEXCOORD;
        attributeDescriptions[LOC_TEXCOORD].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[LOC_TEXCOORD].offset = static_cast<uint32_t>(offsetof(Vertex, texcoord));

        // Normal: Location 3
        attributeDescriptions[LOC_NORMAL].binding = BINDING_ZERO;
        attributeDescriptions[LOC_NORMAL].location = LOC_NORMAL;
        attributeDescriptions[LOC_NORMAL].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_NORMAL].offset = static_cast<uint32_t>(offsetof(Vertex, normal));

        return attributeDescriptions;
    }
```

```cpp
// Logic from OBJLoader mapping deduplication

// ...

// 1. Skip comments or empty streams
if (!ss || (prefix[INDEX_FIRST] == CHAR_COMMENT)) {
    continue;
}

// 2. Parse Geometric Attributes
if (prefix == CMD_VERTEX) {
    glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
    ss >> pos.x >> pos.y >> pos.z;
    attrib_positions.push_back(pos);
}
else if (prefix == CMD_TEXCOORD) {
    glm::vec2 tex{ 0.0f, 0.0f };
    ss >> tex.x >> tex.y;
    // Vulkan's (0,0) is top-left; OBJ (0,0) is bottom-left. Invert V coordinate.
    tex.y = FLOAT_ONE - tex.y;
    attrib_texcoords.push_back(tex);
}
// ...
```

Interaction is managed by the `ConfigLoader`, which parses `config.txt` to eliminate hard-coded transforms, and the `StatsManager`, which provides real-time performance data to the IMGUI overlay via a circular buffer.

```cpp
// Circular buffer logic from StatsManager.h
void update(const float deltaTime) {
    // Prevent division by zero and extreme outliers
    if (deltaTime > 0.0f) {
        const float currentFps = 1.0f / deltaTime;

        fpsHistory[static_cast<size_t>(offset)] = currentFps;

        // Circular buffer logic using unsigned arithmetic
        offset = (offset + 1U) % HISTORY_SIZE;
    }
}
```

### Part II: Geometry, Shading & Global Illumination

**Geometry Representation (The Globe and Sand Plug)** utilizes trigonometric algorithms in `GeometryUtils` to generate a truncated hemispherical dome and a height-displaced sand plane. 

```cpp
// Logic found in GeometryUtils to generate displaced terrain
// Step 1: Generate the Top Dune Surface using sinusoidal displacement
for (uint32_t r = 0U; r <= numRings; ++r) {
    const float currentRadius = (static_cast<float>(r) / fNumRings) * rimRadius;
    for (uint32_t s = 0U; s <= segments; ++s) {
        const float angle = (static_cast<float>(s) / fSegments) * GeometryUtils::TWO_PI;
        const float x = static_cast<float>(std::cos(static_cast<double>(angle))) * currentRadius;
        const float z = static_cast<float>(std::sin(static_cast<double>(angle))) * currentRadius;

        // Apply height displacement based on dune frequencies
        const float edgeWeight = GeometryUtils::FLOAT_ONE - (currentRadius / rimRadius);
        const float h = static_cast<float>((std::sin(static_cast<double>(x * GeometryUtils::DUNE_FREQ_X)) * static_cast<double>(GeometryUtils::DUNE_AMP_X)) +
            (std::cos(static_cast<double>(z * GeometryUtils::DUNE_FREQ_Z)) * static_cast<double>(GeometryUtils::DUNE_AMP_Z))) * edgeWeight;

        const glm::vec2 uv = glm::vec2(GeometryUtils::FLOAT_HALF + (x / (rimRadius * 2.0f)), GeometryUtils::FLOAT_HALF + (z / (rimRadius * 2.0f))) * GeometryUtils::SAND_TILING;
        data.vertices.push_back({ glm::vec3(x, h, z), color, uv, glm::vec3(0.0f, 1.0f, 0.0f) });
    }
}
```

These surfaces are lit using **Dual Shading Models (Gouraud vs. Phong)**. Gouraud shading is implemented in `gouraud.vert` for high-poly efficiency.

```glsl
// Per-pixel SPECULAR logic from phong.frag
// Evaluates N, L, V, and H at the fragment level for high precision
void main() {
    // 1. INITIAL SETUP
    vec3 albedo = texture(texSampler, fragTexCoord).rgb;
    float shadow = calculateShadow(fragPosLightSpace);

    // 2. DYNAMIC AMBIENT CALCULATION
    vec3 ambientResult = albedo * (ubo.lightColor * 0.05); 

    if (ubo.useGouraud == 1) {
        // 3. GOURAUD FALLBACK MODE
        vec3 sparkContribution = calculateSparkLighting(normalize(fragNormal), fragPos, albedo);
        outColor = vec4(ambientResult + (albedo * fragGouraudColor * shadow) + sparkContribution, 1.0);
    } else {
        // 4. PHONG / PBR-LITE MODE
        float metallic = texture(metallicSampler, fragTexCoord).r;
        float roughness = texture(roughnessSampler, fragTexCoord).r;
        
        vec3 N = normalize(fragNormal);
        vec3 L = normalize(ubo.lightPos - fragPos);
        vec3 V = normalize(ubo.viewPos - fragPos);
        vec3 H = normalize(L + V);

        float diff = max(dot(N, L), 0.0);
        
        // Specular logic based on material roughness
        float specPower = mix(128.0, 2.0, roughness); 
        float spec = pow(max(dot(N, H), 0.0), specPower) * 0.5;
        
        // Metal workflow: Tint reflections and darken diffuse
        vec3 specularTint = mix(vec3(1.0), albedo, metallic);
        vec3 diffuseColor = albedo * (1.0 - metallic);

        // 5. LIGHTING COMPOSITION
        vec3 diffuseResult = (diffuseColor * diff * ubo.lightColor) * shadow;
        vec3 specularResult = (specularTint * spec * ubo.lightColor) * shadow;

        // Spark Light contribution
        vec3 sparkContribution = calculateSparkLighting(N, fragPos, albedo);

        // Final Fragment Output
        outColor = vec4(ambientResult + diffuseResult + specularResult + sparkContribution, 1.0);
    }
}
```

- Phong:
![Phong Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img2.png> "Phong Rendering")

- Gouraud:
![Gouraud Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img3.png> "Gouraud Rendering")

**Bump Mapping** was solved with the use of normal maps.

```glsl
// Normal map reconstruction in base.frag
// 2. Normal Reconstruction
// Unpacks [0, 1] texture data to [-1, 1] vector space.
vec3 mapNormal = texture(normalSampler, fragTexCoord).rgb * 2.0 - 1.0;
// Blend interpolated normal with map normal (weighted by 2.2 for increased detail depth)
vec3 N = normalize(fragNormal + mapNormal * 2.2); 
```

![Bump Mapping Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img4.png> "Bump Mapping Rendering")

Scene grounding is provided by **Shadow Mapping**. To solve "Shadow Acne" on procedural dunes, I implemented a **Slope-Scaled Depth Bias**.

```glsl
// Shadow comparison logic in base.frag
float calculateShadow(vec4 posLightSpace) {
    // 1. Perspective divide to get Normalized Device Coordinates [-1, 1]
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
    
    // 2. Transform XY from [-1,1] to [0,1] for UV sampling
    // Z remains [0,1] due to Vulkan's depth convention (GLM_FORCE_DEPTH_ZERO_TO_ONE)
    projCoords.xy = projCoords.xy * 0.5 + 0.5; 
    
    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;
    
    // 3. Shadow Acne Prevention: Bias is calibrated to prevent moiré patterns on flat desert surfaces.
    float bias = 0.002; 
    return (currentDepth - bias > closestDepth) ? 0.4 : 1.0;
}
```

![Shadow Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img5.png> "Gouraud Rendering")

Visual polish is added via a post-processing stack. The **Bloom effect** thresholding is handled in `post.frag`, extracting highlights and applying a 9-tap Gaussian blur before additive blending. One of the things I would have liked to implement is the abstraction of post-processing effects fo resuability

```glsl
// Soft-knee Bloom thresholding in post.frag
// Extracts brightness above a 0.7 threshold
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
```

![Bloom Effect Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img6.png> "Bloom Effect Rendering]")

### Part III: Dynamic Environment & GPU Simulations

The atmospheric simulation is driven by the `ClimateManager`, which interpolates ambient colors based on the sun's orbital position. The engine utilizes an `R16G16B16A16_SFLOAT` HDR buffer to preserve color detail. The ClimateManager controls the wheather timing of it all, it can easily be modified. More resuability could have been achieved by abstracting the wheater itself.
```glsl
// Soft-knee Bloom and Tone Mapping in post.frag
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
```

- Summer:
![Summer Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img6.png> "Summer Rendering]")

- Rain:
![Rain Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img8.png> "Rain Rendering]")

- Winter:
![Winter Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img9.png> "Winter Rendering]")

The **Translucent Globe & Refraction Snapshot** feature implements a multi-pass technique where the renderer copies the opaque scene into a sampler. In `glass.frag` and `water.frag`, I offset screen UVs based on surface normals.

```glsl
// Advanced refraction and Fresnel reflection in glass.frag
void main() {
    // 1. HORIZON CLAMP (The Experiment)
    // We create a 'virtual' light position that never dips below dawn/dusk height.
    // This prevents fresnel and height-based math from breaking at night.
    vec3 virtualLightPos = ubo.lightPos;
    virtualLightPos.y = max(virtualLightPos.y, -0.1); // Clamp slightly below 0.0

    // 2. SCREEN-SPACE COORDINATE CALCULATION
    vec2 screenUV = gl_FragCoord.xy / textureSize(sceneSampler, 0).xy;
    vec3 N = normalize(fragNormal);
    vec3 V = normalize(ubo.viewPos - fragPos);

    // 3. REFRACTION DISTORTION
    float distortionStrength = 0.03; 
    vec2 offset = N.xy * distortionStrength;
    
    // 4. SCENE SAMPLING
    vec2 finalUV = clamp(screenUV + offset, 0.001, 0.999);
    vec3 sceneColor = texture(sceneSampler, finalUV).rgb;

    // 5. LIGHTING & FRESNEL EFFECTS
    // Use the virtual light position to calculate height-based intensity.
    vec3 reflectionColor = ubo.lightColor * 0.4; 

    // Use virtualLightPos.y instead of ubo.lightPos.y
    float height = virtualLightPos.y / 4.0;
    float fresnelPower = 5.0;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), fresnelPower);
    
    // We add a floor of 0.005 so there is always a tiny glint on the glass
    float fresnelAlpha = clamp(height + 0.6, 0.005, 0.3);

    // Mix refracted scene with reflection.
    vec3 finalColor = mix(sceneColor, reflectionColor, fresnel * fresnelAlpha);

    // 6. FINAL ALPHA BLENDING
    // Use the height of the virtual light to keep the glass from becoming 
    // too opaque or fully invisible during the 'Night' phase.
    float alpha = clamp(0.5 + (height * 0.2), 0.4, 0.7);
    
    outColor = vec4(finalColor, alpha); 
}
```

![Glass Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img10.png> "Glass Rendering]")

Water surfaces are animated in `water.vert` using procedural sine waves to displace vertices, creating a dynamic ripple effect. Using a normal map to generate the bumping on the water surface, color is completly generated via the shader files.

```glsl
// Horizon Clamping in water.frag
void main() {
    // 1. HORIZON CLAMP (The Experiment)
    // Create a 'virtual' height to prevent reflections from breaking at night.
    // By clamping at -0.1, we maintain a "Dusk" state throughout the night phase.
    vec3 virtualLightPos = ubo.lightPos;
    virtualLightPos.y = max(virtualLightPos.y, -0.1);

    // ...
}
```

![Water Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img11.png> "Water Rendering]")

**GPU Compute Particle Systems** handle simulations for Rain, Dust, and Snow. To keep particles contained, I implemented **Spherical Boundary Enforcement** in the compute kernels.

```glsl
// Uniform volumetric distribution math from snow.comp
void main() {
    uint index = gl_GlobalInvocationID.x;

    // ... update logic
    
    if (p.position.y < -0.12 || distFromCenter > globeRadius) {
        if (ubo.spawnEnabled > 0.5) {
            // 4. SPHERICAL RESPAWN LOGIC
            float seed = float(index) + ubo.totalTime;
            
            // Generate a point within the top-hemisphere volume.
            float r = (globeRadius - 0.05) * pow(hash(seed), 0.33);
            float phi = hash(seed + 1.0) * 1.57;
            float theta = hash(seed + 2.0) * 6.28318;
            
            p.position.x = r * sin(phi) * cos(theta);
            p.position.z = r * sin(phi) * sin(theta);
            p.position.y = r * cos(phi);
            p.position.xyz += sphereCenter;

            // 5. ATTRIBUTE INITIALIZATION
            // SMALLER SIZE: (0.4 to 0.8 range) for fine snowflakes.
            p.position.w = 0.4 + hash(seed + 4.0) * 0.4;
            
            // Drift velocity: Slow downward descent
            p.velocity.y = -0.3 - (hash(seed + 3.0) * 0.2);
            p.color = vec4(0.9, 0.9, 1.0, 0.8);
        } else {
            // Kill alpha if spawn is disabled to clear the scene.
            p.color.a = 0.0;
        }
    }

    // Write back updated state to global storage
    particles[index] = p;
}
```

**Illuminating Sparks** bridges Compute and Graphics stages; `fire.comp` passes particle positions to the global UBO, allowing fragments in `base.frag` to be lit by embers.

```glsl
// Compute-to-Graphics light bridge in base.frag
vec3 calculateSparkLighting(vec3 N, vec3 fragPos, vec3 albedo) {
    vec3 totalSparkLight = vec3(0.0);
    
    for(int i = 0; i < 4; i++) {
        vec3 L_vec = ubo.sparks[i].position - fragPos;
        float dist = length(L_vec);
        vec3 L     = normalize(L_vec);
        
        // Short-range attenuation (Linear + Quadratic falloff)
        float attenuation = 1.0 / (1.0 + 2.0 * dist + 25.0 * (dist * dist));
        if (attenuation < 0.01) continue;
        
        // Wrap Lighting: Allows light to bend around curved surfaces (like cactus ribs)
        float wrap = 0.4; 
        float diff = max(0.0, (dot(N, L) + wrap) / (1.0 + wrap));
        
        totalSparkLight += (albedo * ubo.sparks[i].color * diff * attenuation);
    }
    
    // Ambient Warmth: Subtly tints the scene near the fire origin.
    vec3 fireAmbient = albedo * vec3(1.0, 0.4, 0.1) * 0.015;
    
    return totalSparkLight + fireAmbient;
}
```

![Fire Rendering](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img12.png> "Fire Rendering]")

---

## 4. Potential Extensions & Scalability

* **Potential Extensions:**
* **Deferred Rendering:** Moving to a G-Buffer approach would allow for hundreds of dynamic "Sparks" without the  performance hit of the current forward renderer.
* **Displacement Mapping:** Using Tessellation Shaders to dynamically subdivide the sand mesh would provide high geometric detail for dunes.


* **Scalability Issues:**
* **VRAM Constraints:** The `SimpleAllocator` uses a fixed block. A larger scene with 4K textures would require a "Paged" allocation strategy.
* **Draw Call Bottleneck:** Currently, every object is an individual draw call. Implementing **GPU-Driven Rendering** (via `VkDrawIndexedIndirect`) would allow the GPU to cull and draw the entire scene in a single command.

---

## 5. Final Reflection

* **Reflect on what you have learnt:** Vulkan's complexity lies in synchronization and resource ownership. Mastering explicit memory barriers and manual VRAM management through a sub-allocator. I'm pretty happy with the results and can happy that can be improved upon.
* * **Did you make any mistakes:** Yes, a lot of them. Particlary the implementation of shadows was a real struggle, the other one being post-processing and transitioning from 1-bit to 8-bit (HDR) was also quite the challenge because I already did worked on my project but at the time of implementing the GHDR changes the scenes started to appear noisy and I learnt that is because of the corruption of the process in that I planned.

- ![Skybox Error](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img13.png> "Skybox Error")
- ![Refraction Error](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img14.png> "Refraction Error")
- ![Water Error](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img15.png> "Water Error")
- ![Shadows Error](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img16.png> "Shadows Error")
- ![8-bit Error](<../markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img17.png> "8-bit Error")

* **In what way has your knowledge improved:** I transitioned from a basic understanding of graphics to a hardware-aware expertise in multi-pass rendering, compute-driven physics, and modern HDR post-processing workflows. I' pretty content that my knowledge in computer graphics increased.