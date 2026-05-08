# Chapter 10 — Asset Pipeline & GPU Resource Management

> **Goal:** Understand how a `.obj` file on disk becomes rendered triangles on screen — and how to
> add your own assets to the engine.

---

## 10.1 What Is the Asset Pipeline?

Every visible object in the engine starts as data on the CPU (vertices, textures) and must be
uploaded to the GPU before it can be drawn. This journey has several distinct stages:

```
Disk                CPU Memory              GPU Memory (VRAM)
─────               ──────────              ─────────────────
.obj file     →    OBJLoader parses      →  VkBuffer (vertex + index)
.png texture  →    stb_image decodes     →  VkImage + VkImageView + VkSampler
                   Material assembled    →  VkDescriptorSet (set 1)
                                         →  Draw call references both
```

The engine does not stream assets at runtime — everything needed for a scene is uploaded
during `Scenario::OnLoad()`, before the first frame renders. When you switch scenes, old GPU
resources are destroyed and new ones are allocated.

---

## 10.2 The AssetManager

**File:** `include/assets/AssetManager.h`

`AssetManager` is the single point of contact for loading geometry and textures. It is registered
with the `ServiceLocator` during `EngineOrchestrator` construction and is accessible from
anywhere in the engine.

```cpp
// Getting the asset manager from any system or scenario:
auto* assets = ServiceLocator::GetAssetManager();
```

### What It Does

| Method | Description |
|--------|-------------|
| `loadTexture(path)` | Returns a `shared_ptr<Texture>` — cached by path, so loading the same texture twice costs nothing |
| `loadModel(path, materialSelector, ...)` | Parses `.obj`, uploads geometry, creates material |
| `createMaterial(albedo, normal, ao, metallic, roughness, pipeline)` | Allocates a descriptor set for the PBR texture set |
| `processMeshData(data, material, ...)` | Uploads vertex + index data to GPU, wraps into `Mesh` |

### Why `shared_ptr<Texture>`

Textures are expensive (VRAM). If ten different meshes share the same brick texture, they
should all point at the same `VkImage` — not each allocate their own copy. `AssetManager`
maintains a `textureCache` (a `std::unordered_map<string, shared_ptr<Texture>>`). The
second call to `loadTexture("textures/brick.png")` returns the cached pointer immediately.

When all meshes sharing a texture are destroyed (going out of scope), the `shared_ptr`
reference count hits zero and the texture is destroyed. No manual tracking required.

---

## 10.3 OBJ Loading in Detail

**File:** `include/assets/OBJLoader.h`

The OBJ format is a plain-text 3D file format with `v`, `vn`, `vt`, and `f` directives for
vertex positions, normals, UV coordinates, and faces respectively.

### What OBJLoader Does

1. **Parse positions, normals, UVs** from `v`/`vn`/`vt` lines.
2. **Expand faces:** OBJ faces use a `position/texcoord/normal` index triple per corner. The
   loader expands these into an interleaved `Vertex` struct, deduplicating via a hash map.
3. **Triangulate:** OBJ polygons can have more than 3 vertices. The loader fan-triangulates them
   (`v0, v1, v2`, `v0, v2, v3`, etc.).
4. **Generate normals if absent:** If the OBJ has no `vn` directives, per-face normals are
   computed from the cross product of two edges.
5. **Output:** A `MeshData` struct containing a `std::vector<Vertex>` and a
   `std::vector<uint32_t>` for indices.

### UV Handedness

OpenGL and Vulkan differ in their Y-axis convention for UVs. OBJ stores UVs with Y pointing
up; Vulkan textures have Y pointing down. The loader flips V: `v_vulkan = 1.0 - v_obj`.

---

## 10.4 The Vertex Struct and GPU Alignment

**File:** `include/assets/Vertex.h`

Every vertex uploaded to the GPU uses this struct:

```cpp
// include/assets/Vertex.h
namespace GE::Assets {

struct Vertex {
    glm::vec3 position;   // Location 0: XYZ world position
    glm::vec3 color;      // Location 1: RGB per-vertex tint (default white)
    glm::vec2 texcoord;   // Location 2: UV texture coordinate
    glm::vec3 normal;     // Location 3: surface normal (unit vector)
    glm::vec3 tangent;    // Location 4: tangent for TBN normal mapping
};

} // namespace GE::Assets
```

Memory layout (sizeof = 56 bytes):
```
Offset  0: position  [12 bytes, float×3]
Offset 12: color     [12 bytes, float×3]
Offset 24: texcoord  [ 8 bytes, float×2]
Offset 32: normal    [12 bytes, float×3]
Offset 44: tangent   [12 bytes, float×3]
```

### Telling Vulkan How to Read It

`Vertex::getBindingDescription()` returns a `VkVertexInputBindingDescription`:
```cpp
bindingDescription.stride    = sizeof(Vertex);          // 56 bytes
bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // one per vertex
```

`Vertex::getAttributeDescriptions()` returns 5 `VkVertexInputAttributeDescription` entries,
one per member, mapping the byte offset and format to the shader `layout(location = N)` input.

### Every Shader Must Declare All 5 Attributes

Because a single `VkPipeline` is created with all 5 attribute slots, every vertex shader must
declare `layout(location = 4) in vec3 inTangent` even if it does not use tangents — Vulkan
validates the binding layout against the pipeline at draw time, and a mismatch is a hard error.

```glsl
// All vertex shaders (even flatcolor.vert) must include:
layout(location = 4) in vec3 inTangent;
// Usage: (void)inTangent;  ← suppress unused warning
```

---

## 10.5 Uploading Geometry to the GPU (Staging Buffer Pattern)

Vulkan separates memory into two types:
- **Host-visible memory** (`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`): CPU can write to it, but
  GPU reads are slow (goes over PCIe bus).
- **Device-local memory** (`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`): GPU accesses it at full
  bandwidth, but the CPU cannot write directly.

For static geometry (vertices that never change after upload), you want **device-local** memory.
But you cannot write to it directly. The solution is the **staging buffer pattern:**

```
Step 1:  CPU allocates a host-visible "staging" buffer
Step 2:  CPU copies vertex data into the staging buffer (memcpy)
Step 3:  GPU command: vkCmdCopyBuffer(staging → device-local)
Step 4:  Staging buffer is destroyed after the copy completes
```

### In Code (simplified from `AssetManager::processMeshData`)

```cpp
// 1. Create host-visible staging buffer
VkBuffer     stagingBuf;
VkDeviceMemory stagingMem;
createBuffer(vertDataSize,
             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
             stagingBuf, stagingMem);

// 2. Copy CPU data into staging
void* data;
vkMapMemory(device, stagingMem, 0, vertDataSize, 0, &data);
memcpy(data, vertices.data(), vertDataSize);
vkUnmapMemory(device, stagingMem);

// 3. Create device-local vertex buffer
VkBuffer     vertexBuf;
VkDeviceMemory vertexMem;
createBuffer(vertDataSize,
             VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
             vertexBuf, vertexMem);

// 4. Record copy command (into an already-open command buffer)
VkBufferCopy copyRegion{ 0, 0, vertDataSize };
vkCmdCopyBuffer(cmdBuffer, stagingBuf, vertexBuf, 1, &copyRegion);

// 5. Staging buffer lives until the command completes (managed via stagingBuffers vector)
```

The staging buffers are collected in a `std::vector<VkBuffer>` passed into `processMeshData` and
destroyed by the caller after `vkQueueWaitIdle()` confirms the copy is done.

### Ownership Transfer Barrier

When different queue families are involved (e.g. transfer queue for the copy, graphics queue
for drawing), a **queue family ownership transfer** must be performed via a
`VkBufferMemoryBarrier`. The engine uses a single queue family for simplicity in most paths,
but the barrier is still recorded for correctness.

---

## 10.6 Texture Loading

**Files:** `include/graphics/Texture.h`, `assets/AssetManager.h`

### Loading Steps

```
1. stb_image decodes .png / .jpg → raw RGBA bytes in CPU RAM
2. Staging buffer created, bytes copied into it
3. VkImage created (VK_IMAGE_USAGE_TRANSFER_DST | SAMPLED)
4. Image layout transitioned: UNDEFINED → TRANSFER_DST_OPTIMAL
5. vkCmdCopyBufferToImage copies from staging buffer → VkImage
6. Mip chain generated via repeated vkCmdBlitImage (halving resolution each level)
7. Final layout transition: TRANSFER_DST_OPTIMAL → SHADER_READ_ONLY_OPTIMAL
8. VkImageView created (lets shaders sample specific mip levels / array layers)
9. VkSampler created (filtering = LINEAR, mipmap mode = LINEAR, anisotropy = 16×)
```

### Why Mipmaps?

A mipmap is a pre-filtered sequence of progressively smaller versions of the texture
(full res → half → quarter → ...). When a textured surface is far from the camera and occupies
only a few pixels, the GPU samples the small mipmap level instead of averaging hundreds of
full-resolution texels. This eliminates aliasing ("shimmering") on distant surfaces.

### PBR Texture Set (5 textures per material)

The Phong pipeline (pipeline index 0) uses 5 textures per object, all bound at descriptor
set 1:

| Binding | Texture | Purpose |
|---------|---------|---------|
| 0 | Albedo | Base color |
| 1 | Normal | Tangent-space normals (RGB encoded) |
| 2 | AO (Ambient Occlusion) | Darkens crevices |
| 3 | Metallic | 0 = dielectric, 1 = metal |
| 4 | Roughness | 0 = mirror, 1 = fully diffuse |

The flat-color pipeline (pipeline index 8) uses zero textures and only vertex colors —
this is what the owner-color mode uses for networked entities.

---

## 10.7 The SimpleAllocator (TLSF)

**File:** `include/memory/SimpleAllocator.h`

Vulkan allows at most ~4096 `vkAllocateMemory` calls per device (hardware limit). A complex
scene with 200 objects would exhaust this limit if each mesh allocated its own memory.

The engine avoids this by pre-allocating a **single 256 MB VRAM pool** at startup and then
**sub-allocating** from it for every buffer and image.

### What is TLSF?

**TLSF (Two-Level Segregated Fit)** is a real-time memory allocator designed for constant-time
allocation and deallocation. It maintains free-block lists organised by size class (two levels
of segregation) and can find a fit in O(1) time without fragmentation growth.

The engine uses a C++ wrapper around the reference TLSF implementation. Each call to
`GpuResourceManager::createBuffer()` internally calls `SimpleAllocator::Allocate(size, alignment)`,
which returns an offset into the pre-allocated pool.

### The Pool Lifecycle

```cpp
// EngineOrchestrator constructor (simplified):
auto* alloc = new SimpleAllocator();
alloc->Init(vulkanDevice, EngineConstants::VRAM_POOL_SIZE);  // 256 MB
ServiceLocator::Provide(alloc);

// Every buffer creation goes through:
auto allocation = alloc->Allocate(requiredSize, alignment);
// → returns { VkDeviceMemory = poolMemory, offset = N }
```

When a buffer is freed, `SimpleAllocator::Free(allocation)` returns the range to the pool
immediately — no garbage collection delay.

---

## 10.8 GpuResourceManager

**File:** `include/graphics/GpuResourceManager.h`

`GpuResourceManager` sits between the engine's systems and the raw Vulkan API. Every buffer
and image in the engine is created and destroyed through this manager.

### Key Methods

```cpp
// Creating a vertex or index buffer:
VkBuffer vertexBuf = resources->createBuffer(
    size,
    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
);

// Creating an image (texture):
VkImage img = resources->createImage(
    width, height, mipLevels,
    VK_FORMAT_R8G8B8A8_SRGB,
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
);

// Cleanup (called during Scenario::OnUnload):
resources->destroyBuffer(vertexBuf);
resources->destroyImage(img);
```

### Lifetime Management

`GpuResourceManager` does not own the individual buffers and images — the caller does. The
manager provides allocation services; the caller is responsible for calling `destroyBuffer` /
`destroyImage` before the VkDevice is destroyed.

In practice this is handled by:
- `AssetManager` destroys textures and meshes when their `shared_ptr` ref count reaches zero.
- `Scenario::OnUnload()` destroys all scene-specific buffers (cloth vertex buffers, etc.).
- The `ClothComponent`'s Vulkan resources are destroyed explicitly in `FlatBuffersScenario::OnUnload()`.

---

## 10.9 Exercise: Add a New 3D Model

Follow these steps to add your own `.obj` file to the engine:

### Step 1 — Place the model file

Copy your `.obj` file into the `models/` folder. For this example: `models/my_sculpture.obj`.

### Step 2 — Write a JSON scene referencing it

Open or create a scene JSON in `config/flatbufferConfig/` (e.g. copy `01_multiplayer.json`).
Add an object entry:

```json
{
  "name": "MySculpture",
  "position": { "x": 0, "y": 1, "z": -5 },
  "rotation": { "x": 0, "y": 0, "z": 0 },
  "scale": { "x": 1, "y": 1, "z": 1 },
  "shape": {
    "type": "Cuboid",
    "half_extents": { "x": 1, "y": 1, "z": 1 }
  },
  "behaviour": {
    "type": "StaticObject",
    "mesh_path": "models/my_sculpture.obj",
    "pipeline_index": 0
  }
}
```

> **Note:** The collider shape (`Cuboid`) is a physics approximation. The renderer uses the
> actual `.obj` mesh. For irregular shapes, choose the collider that best approximates the
> object's collision volume.

### Step 3 — Compile the scene to binary

```bat
C:\Users\your_name\flatbuffers\Release\flatc.exe ^
    --binary ^
    -o config/flatbufferConfig/showcases/ ^
    flatbuffers/Scene.fbs ^
    config/flatbufferConfig/showcases/my_scene.json
```

### Step 4 — Load in the engine

Launch the engine, open the **Scenes** menu, and click on your `.bin` file.

### Step 5 — Add a PBR material (optional)

If you have texture maps for the model, place them in `textures/`. In `FBSceneAdapter.cpp`,
look for how `adaptBehaviour()` calls `AssetManager::loadTexture()` and `createMaterial()` for
`SimulatedObject` entries. Mirror that pattern for your new scene entry.

---

## 10.10 Common Asset Pipeline Mistakes

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| White object (no texture) | Pipeline index 8 (flat-color) used instead of 0 (Phong) | Set `pipeline_index: 0` in the scene JSON |
| Stretched texture | OBJ UV coordinates not exported | Re-export OBJ with UV data from your DCC tool |
| Model not visible | Vertex buffer empty (OBJ parse failed) | Check the Logger output for parsing errors |
| Crash on scene load | Vulkan validation: `stagingBuffer` destroyed before copy completes | Ensure staging buffers live until `vkQueueWaitIdle()` |
| Very slow first load | Mipmap generation on CPU | Expected — GPU-side mip generation is a future optimisation |
| Object invisible from one side | Backface culling | Flat-color pipeline has culling off; Phong has culling on. Check `enableCulling` flag in `createMaterialPipelines()` |

---

*Next: [Chapter 11 — Particle System & GPU Compute](11_Particle_System.md)*
