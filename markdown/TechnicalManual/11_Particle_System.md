# Chapter 11 — Particle System & GPU Compute Shaders

> **Goal:** Understand how the engine simulates and renders 20,000+ particles per frame at real-
> time speeds using GPU compute shaders — and how to add a new particle type.

---

## 11.1 Why GPU Compute for Particles?

A particle system with 20,000 particles at 120 Hz requires 2,400,000 particle updates per
second. If each update takes just 100 ns on the CPU, that is 240 ms per second — more than
the entire 8 ms frame budget.

GPU compute solves this by running all 20,000 updates **in parallel** across hundreds of shader
cores. A modern GPU can complete all 20,000 particle integrations in under 0.1 ms.

### The Compute vs. Graphics Pipeline Distinction

Vulkan has two pipelines relevant here:
- **Graphics pipeline** — processes vertices and fragments for rendering.
- **Compute pipeline** — runs arbitrary parallel workloads (simulation, AI, sorting) on the GPU.

The particle system uses **both**: compute for simulation, graphics for rendering. They share
the same SSBO (Shader Storage Buffer Object) that holds particle state.

---

## 11.2 The Particle Struct

**File:** `include/particles/Particle.h`

Each particle is represented by three `glm::vec4` fields, `alignas(16)`:

```cpp
struct Particle {
    alignas(16) glm::vec4 m_position;
    // x, y, z = world position
    // w        = size (controls point-sprite radius)

    alignas(16) glm::vec4 m_velocity;
    // x, y, z = velocity (m/s)
    // w        = remaining life (0 = dead, 1 = freshly spawned)

    alignas(16) glm::vec4 m_color;
    // r, g, b  = particle color
    // a        = opacity (0 = transparent, 1 = opaque)
};
```

`sizeof(Particle) = 48 bytes`. Total SSBO size for 20,000 particles:
`20000 × 48 = 960 KB` — well within the 256 MB VRAM pool.

### Why `alignas(16)` and `glm::vec4`?

GPU shaders use a memory layout called **std430** for SSBOs, which requires all `vec4` members
to be 16-byte aligned. Packing position as `vec3` + separate `float` instead of `vec4`
introduces subtle alignment bugs. The `glm::vec4` with `alignas(16)` matches std430 exactly
and allows zero-copy reads from the GPU.

---

## 11.3 GpuParticleBackend

**File:** `include/particles/GpuParticleBackend.h`

`GpuParticleBackend` is a **non-ECS class** — it is not a system and does not subclass
`IECSystem`. Instead, it is owned by `ParticleComponent` (one backend per emitter entity) and
driven by `ParticleEmitterSystem` each frame.

### What It Owns

```
GpuParticleBackend
├── storageBuffer (VkBuffer)         ← SSBO holding all Particle structs
├── storageBufferMemory
├── uniformBuffer (VkBuffer)         ← ParticleUBO: dt, wind, emitter position, etc.
├── uniformBufferMemory
├── uniformBufferMapped (void*)      ← Persistently mapped (written every frame)
├── computePipeline (VkPipeline)     ← Simulation shader
├── computePipelineLayout
├── computeDescriptorSet             ← Binds UBO + SSBO to compute shader
├── graphicsPipeline (VkPipeline)    ← Point-sprite rendering shader
└── graphicsPipelineLayout
```

### Workgroup Size

The compute shader declares `layout(local_size_x = 256) in;`. Each dispatch invocation
processes one particle. The dispatch call:

```cpp
// In GpuParticleBackend::update():
static constexpr uint32_t COMPUTE_WORKGROUP_SIZE = 256U;
uint32_t groupCount = (particleCount + COMPUTE_WORKGROUP_SIZE - 1) / COMPUTE_WORKGROUP_SIZE;
vkCmdDispatch(commandBuffer, groupCount, 1, 1);
```

For 20,000 particles: `ceil(20000 / 256) = 79` workgroups × 256 threads = 20,224 invocations.
The last workgroup has 224 live particles and 32 idle ones (guarded with `if (idx >= count) return`).

---

## 11.4 Compute Dispatch and Memory Barriers

The particle update must happen in a specific order relative to other passes:

```
Frame N:
  1. vkCmdPipelineBarrier — release SSBO from previous frame's vertex read
  2. GpuParticleBackend::update() → vkCmdDispatch (compute: update positions, velocities, life)
  3. vkCmdPipelineBarrier — wait for compute writes before vertex shader reads
  4. Opaque/transparent render pass records draw calls
  5. GpuParticleBackend::draw() → vkCmdDraw (vertex shader reads SSBO to place point sprites)
```

### The Required Barriers

```cpp
// Before compute (release from vertex shader → compute shader):
VkBufferMemoryBarrier barrier{};
barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;      // vertex shader was reading
barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;     // compute shader will write
vkCmdPipelineBarrier(cb,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,    // was used in
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // now needed in
    0, 0, nullptr, 1, &barrier, 0, nullptr);

// After compute (release from compute → vertex shader):
barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
vkCmdPipelineBarrier(cb,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    0, 0, nullptr, 1, &barrier, 0, nullptr);
```

Forgetting these barriers creates a **write-after-read hazard**: the vertex shader may be
reading old positions while compute is writing new ones, causing visual corruption.

### The ParticleUBO

Every frame, the engine writes a small uniform buffer with simulation parameters before dispatch:

```cpp
struct ParticleUBO {
    float deltaTime;      // dt for this physics tick
    float spawnEnabled;   // 0 or 1 (imitated as float for alignment)
    float totalTime;      // engine clock (for turbulence noise functions)
    float padding;

    glm::vec3 lightColor; // for fire sparks — affect point light color
    float padding2;

    glm::vec3 emitterPos; // world position of the emitter entity
    float padding3;
};
```

The padding fields satisfy std140/std430 alignment rules — `vec3` must start on a 16-byte
boundary, so every `vec3` is followed by a padding float.

---

## 11.5 Particle Types

The engine ships five particle types, each driven by a different compute shader:

| Type | Shader | Behaviour |
|------|--------|-----------|
| Snow | `snow.comp` | Slow downward drift + lateral sway, respawn at top |
| Rain | `rain.comp` | Fast downward fall with wind deflection, respawn at top |
| Fire | `fire.comp` | Upward rise with turbulence, color gradient yellow→red→black |
| Dust | `dust.comp` | Slow radial spread, fade out |
| Smoke | `smoke.comp` | Slow upward drift with swirl, large size, fade out |

All five share the same `Particle` struct and SSBO layout. What differs is the **shader logic**:
each compute shader reads `m_position.w` (size) and `m_velocity.w` (life) and applies
different forces, turbulence patterns, and respawn conditions.

### Respawn Logic (example from snow.comp)

```glsl
// Executed per particle in compute shader
if (m_velocity.w <= 0.0) {
    // Respawn: random position at top of spawn volume
    m_position.xyz = emitterPos + vec3(rand(idx, time) * 10.0 - 5.0,
                                        5.0,
                                        rand(idx + 1, time) * 10.0 - 5.0);
    m_velocity.w = 1.0;  // reset life
} else {
    // Integrate: apply gravity + wind
    m_velocity.xyz += vec3(windX, -0.3, windZ) * deltaTime;
    m_position.xyz += m_velocity.xyz * deltaTime;
    m_velocity.w   -= 0.05 * deltaTime;  // drain life
}
```

### How the Engine Chooses a Shader

`FBSceneAdapter` reads the `emitter_type` string from the FlatBuffers table and maps it to
shader file paths:

```cpp
// In FBSceneAdapter::adaptBehaviour()
if      (emitterType == "snow")  { comp = "snow.comp.spv";  }
else if (emitterType == "rain")  { comp = "rain.comp.spv";  }
else if (emitterType == "fire")  { comp = "fire.comp.spv";  }
// ...
// Creates GpuParticleBackend with the appropriate shader paths
```

---

## 11.6 ParticleEmitterSystem

**File:** `include/systems/ParticleEmitterSystem.h`

`ParticleEmitterSystem` is an `IGpuSystem` — it runs on the **main thread** and receives a
`VkCommandBuffer` to record GPU commands. This is the only system that records compute
dispatches.

```cpp
void ParticleEmitterSystem::OnUpdate(float dt, VkCommandBuffer cb) override {
    auto* em = ServiceLocator::GetEntityManager();
    auto& particles = em->GetCompArr<ParticleComponent>();

    for (uint32_t i = 0; i < particles.GetCount(); ++i) {
        ParticleComponent& pc = particles.Data()[i];
        EntityID eid = particles.Index()[i];

        // Get emitter world position from Transform
        auto* tr = em->GetTIComponent<GE::Components::Transform>(eid);
        if (!tr || !pc.backend) continue;

        // Read climate parameters
        auto* climate = ServiceLocator::GetClimateService();
        glm::vec3 emitterPos = tr->m_worldPosition;
        bool spawnEnabled = /* check if emitter is active */;

        // Delegate to backend (records barriers + dispatch + draw)
        pc.backend->update(cb, dt, spawnEnabled,
                           ServiceLocator::GetTimeService()->totalTime(),
                           climate->getLightColor(), emitterPos);
    }
}
```

### Why IGpuSystem?

The compute dispatch (`vkCmdDispatch`) and the draw call (`vkCmdDraw`) both require a valid,
open `VkCommandBuffer`. Command buffers are only available on the main thread between
`vkBeginCommandBuffer` and `vkEndCommandBuffer`. The physics thread never receives one.

Systems that only modify CPU data (positions, velocities) use `ICpuSystem`. Systems that record
GPU commands use `IGpuSystem`.

---

## 11.7 Emission: CPU-Side Spawn

The GPU compute shader does NOT spawn new particles — it only updates existing ones. Spawning
(choosing which particle slots to revive and where to place them) happens on the CPU.

### How It Works

The SSBO is mapped to CPU-visible memory on creation. When the emitter's spawn rate dictates
a new particle, the CPU:
1. Scans the `m_velocity.w` (life) field for a slot where life ≤ 0 (dead particle).
2. Writes the new particle's initial position, velocity, life, and color into that slot.
3. The next frame's compute dispatch will see the initialized particle and begin updating it.

This is possible because the SSBO is created with
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` (for the initial
upload path). Alternatively, a staging buffer is used for the initial population.

---

## 11.8 Rendering Particles (Point Sprites)

Each particle is rendered as a single **point** — a screen-aligned quad whose size is determined
by `gl_PointSize` in the vertex shader. This is the cheapest way to render a large number of
independent billboards.

### Vertex Shader (simplified)

```glsl
// shaders/snow.vert (representative)
layout(location = 0) in vec4 inPosition;  // xyz = position, w = size
layout(location = 1) in vec4 inVelocity;  // xyz = vel,      w = life
layout(location = 2) in vec4 inColor;

void main() {
    gl_Position  = proj * view * vec4(inPosition.xyz, 1.0);
    gl_PointSize = max(inPosition.w * 50.0 / gl_Position.w, 1.0); // size with depth falloff
    fragColor    = inColor;
}
```

### Fragment Shader (point sprite circle)

```glsl
// Discard corners to make the square point into a circle:
void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;  // [-1,1] in point's local space
    if (dot(coord, coord) > 1.0) discard;     // outside unit circle
    outColor = fragColor;
}
```

### Pipeline Settings for Particles

The particle graphics pipeline uses:
- `VK_PRIMITIVE_TOPOLOGY_POINT_LIST` — each vertex is one point
- `VK_BLEND_FACTOR_SRC_ALPHA` / `VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA` — alpha blending for
  soft edges
- Depth test **enabled** (particles sort correctly behind opaque geometry)
- Depth write **disabled** (particles do not occlude each other — avoids sorting artifacts)

---

## 11.9 Exercise: Add a New Particle Type

Follow these steps to add a "Sparks" particle type that shoots outward from a point and fades.

### Step 1 — Write the compute shader

Create `shaders/sparks.comp`:

```glsl
#version 450

struct Particle {
    vec4 position;  // xyz = pos, w = size
    vec4 velocity;  // xyz = vel, w = life
    vec4 color;
};

layout(std430, binding = 1) buffer ParticleSSBO { Particle particles[]; };
layout(binding = 0) uniform ParticleUBO {
    float deltaTime;
    float spawnEnabled;
    float totalTime;
    float padding;
    vec3  lightColor;
    float padding2;
    vec3  emitterPos;
    float padding3;
} ubo;

layout(local_size_x = 256) in;

void main() {
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= particles.length()) return;

    Particle p = particles[idx];

    if (p.velocity.w <= 0.0) {
        // Respawn with random outward velocity
        float angle = fract(sin(idx * 12.9898 + ubo.totalTime) * 43758.5453) * 6.28318;
        float speed = 2.0 + fract(sin(idx * 78.233) * 43758.5453) * 3.0;
        p.position.xyz = ubo.emitterPos;
        p.velocity.xyz = vec3(cos(angle) * speed, 1.5 + speed * 0.5, sin(angle) * speed);
        p.velocity.w   = 1.0;
        p.color        = vec4(1.0, 0.6, 0.0, 1.0);  // orange
    } else {
        p.velocity.xyz += vec3(0.0, -9.8, 0.0) * ubo.deltaTime;  // gravity
        p.position.xyz += p.velocity.xyz * ubo.deltaTime;
        p.velocity.w   -= 0.8 * ubo.deltaTime;  // fast fade
        p.color.a       = p.velocity.w;
    }

    particles[idx] = p;
}
```

### Step 2 — Compile the shader

Add to `shaders/compile.bat`:
```bat
glslc.exe sparks.comp -o sparks_comp.spv
```

Run `compile.bat`.

### Step 3 — Register the emitter type in FBSceneAdapter

Find the emitter type dispatch in `source/scene/FlatBuffersScenario.cpp` (or `FBSceneAdapter.cpp`)
and add:

```cpp
else if (emitterType == "sparks") {
    comp = "shaders/sparks_comp.spv";
    vert = "shaders/snow_vert.spv";  // reuse snow point-sprite vertex shader
    frag = "shaders/snow_frag.spv";  // reuse snow fragment shader
}
```

### Step 4 — Author a scene JSON referencing it

```json
{
  "name": "SparksEmitter",
  "position": { "x": 0, "y": 2, "z": 0 },
  "behaviour": {
    "type": "ParticleEmitter",
    "emitter_type": "sparks",
    "max_particles": 500
  }
}
```

Compile and load — you should see orange sparks arcing from the origin.

---

## 11.10 Particle System Debugging Checklist

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No particles visible | SSBO not written; emitter position is wrong | Log `emitterPos` before dispatch; verify `ParticleComponent.backend != nullptr` |
| Particles frozen in place | Barrier not inserted before compute | Check that `update()` inserts the pre-compute barrier |
| GPU crash / device lost | Compute shader out-of-bounds access | Add `if (idx >= count) return;` at top of main() |
| Particles all at world origin | `m_worldPosition` not computed (TransformSystem hasn't run yet) | Ensure system runs after `ESystemStage::Transform` |
| Visual corruption (flickering) | Missing post-compute barrier before vertex shader | Insert `compute → vertex` barrier after `vkCmdDispatch` |
| Particles behind all geometry | Depth write enabled on particle pipeline | Set `depthWriteEnable = VK_FALSE` in pipeline creation |

---

*Next: [Chapter 12 — ImGui, Input & Debug Systems](12_ImGui_Input_Debug.md)*
