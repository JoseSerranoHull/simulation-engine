# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Hard Rules

- **Never modify ImGui library files.** Files under the Visual Studio filter "IMGUI Files" (`external-libraries/imgui/`) are read-only. You may read them to look up APIs, flags, and struct layouts, but must never edit them.

## Build System

**Platform:** Windows only. Build via Visual Studio 2022 or MSBuild — no CMake or Makefile.

```bash
# Release build
msbuild simulation-engine.sln /p:Configuration=Release /p:Platform=x64

# Debug build
msbuild simulation-engine.sln /p:Configuration=Debug /p:Platform=x64
```

Output binary: `x64\Release\simulation-engine.exe` or `x64\Debug\simulation-engine.exe`

Shaders (GLSL `.vert`, `.frag`, `.comp`) must be compiled to SPIR-V separately using `glslc` or `glslangValidator`. Compiled `.spv` files are committed to the repo.

## Architecture Overview

This is a real-time 3D simulation engine built on Vulkan. The architecture combines **ECS (Entity-Component-System)** with a **State Pattern** for scenario management.

### Project Structure

```
simulation-engine/
├── simulation-engine.sln / .vcxproj   # Visual Studio 2022 solution & project
├── config/                             # Scenario config files
│   ├── snow_globe.ini                  # Legacy .ini scenarios (GenericScenario / SceneLoader)
│   ├── simulation_lab2.ini
│   ├── simulation_lab3.ini
│   └── flatbufferConfig/               # FlatBuffers binary scenes (FlatBuffersScenario)
│       ├── showcases/                    # Active demo scenes (.bin loaded directly at runtime)
│       │   ├── 01_multiplayer.bin
│       │   ├── 02_shapes_and_materials.bin
│       │   ├── 03_animation_platforms.bin
│       │   ├── 04_spawner_factory.bin
│       │   ├── 05_cloth_simulation.bin
│       │   └── 06_flocking_boids.bin
│       ├── old_showcases/               # Legacy scenes (kept for backward compat)
│       └── tests/                       # Test scenes (cloth_test.bin, flock_test.bin, etc.)
├── flatbuffers/                        # FlatBuffers schema + generated header
│   ├── Scene.fbs                       # Schema source — edit this to extend scene format
│   └── Scene_generated.h               # Patched by hand (do NOT regenerate via flatc — version mismatch)
├── include/                            # Headers organised by domain subfolder
│   ├── core/         Common.h, Logger.h, libs.h, ServiceLocator.h, EngineOrchestrator.h,
│   │                 NetworkBridge.h, SimulationState.h
│   ├── ecs/          Entity.h, EntityManager.h, ComponentArray.h, IComponentArray.h, ComponentType.h, IECSystem.h
│   ├── components/   Transform.h, Tag.h, Components.h, PhysicsComponents.h, ParticleComponent.h,
│   │                 SkyboxComponent.h, ClothComponent.h, FlockingComponent.h, AnimationComponents.h,
│   │                 ScriptComponent.h
│   ├── systems/      TransformSystem.h, PhysicsSystem.h, ParticleEmitterSystem.h, EngineServiceRegistry.h,
│   │                 ClothSystem.h, FlockingSystem.h, AnimationSystem.h, SpawnerSystem.h,
│   │                 SpringSystem.h, ColliderVisualizerSystem.h, ScriptSystem.h
│   ├── scene/        Scene.h, SceneLoader.h, Scenario.h, GenericScenario.h, SnowGlobeScenario.h,
│   │                 FlatBuffersScenario.h, FlatBuffersLoader.h, ISceneAdapter.h, EntityFactory.h
│   │   └── fb/       FBSceneAdapter.h, FBSceneContext.h, Scene_generated.h
│   ├── networking/   NetworkService.h, Packets.h
│   ├── graphics/     VulkanContext.h, VulkanDevice.h, GpuResourceManager.h, SwapChain.h, FrameSyncManager.h,
│   │                 GraphicsPipeline.h, ShaderModule.h, Renderer.h, PostProcessBackend.h, Skybox.h, Cubemap.h,
│   │                 GpuImage.h, Texture.h, RenderPass.h
│   ├── particles/    GpuParticleBackend.h, Particle.h
│   ├── assets/       AssetManager.h, Model.h, Mesh.h, Material.h, OBJLoader.h, Vertex.h, GeometryUtils.h
│   ├── physics/      Physics.h, Physics2D.h, Ray.h, Sphere.h, Plane.h, Capsule.h, Cylinder.h, Line.h, Collider.h
│   ├── lighting/     LightSource.h, PointLightSource.h
│   ├── services/     TimeService.h, InputService.h, PerformanceTracker.h, DebugOverlay.h, ClimateService.h
│   └── memory/       SimpleAllocator.h
├── source/                             # Mirrors include/ subfolder structure
│   ├── main.cpp                        # Entry point → EngineOrchestrator
│   ├── core/         EngineOrchestrator.cpp, NetworkBridge.cpp, Logger.cpp
│   ├── systems/      PhysicsSystem.cpp, ClothSystem.cpp, FlockingSystem.cpp, AnimationSystem.cpp,
│   │                 SpawnerSystem.cpp, SpringSystem.cpp, TransformSystem.cpp, ...
│   └── networking/   NetworkService.cpp
├── shaders/                            # GLSL sources + compiled .spv (committed)
│   ├── compile.bat                     # Batch script to recompile all shaders
│   ├── Lighting: phong.vert/frag, gouraud.vert/frag
│   ├── Particles: snow, rain, fire, dust, smoke (.vert/.frag/.comp)
│   └── Passes: shadow, skybox, post, water, glass, transparent, sand
├── models/                             # OBJ assets (cacti, grass, rocks, sorceress, etc.)
├── textures/                           # Texture assets + cubemaps
├── external-libraries/                 # Vendored dependencies
│   ├── glfw-3.4.bin.WIN64/
│   ├── glm/
│   ├── imgui/
│   └── stb/
├── markdown/                           # Lab book reference documents (see below)
├── markdown-resources/                 # Images and assets referenced by markdown files
│   └── Simulation-Lab/2|3|4|Workshop/  # Per-lab image resources
└── books/                              # Reference textbooks (Real-Time Rendering, PBR, etc.)
```

### Top-Level Orchestration

`main.cpp` → `EngineOrchestrator` (RAII master orchestrator) owns all subsystems:
- Initializes `VulkanDevice` → `VulkanContext` → `GpuResourceManager`
- Loads a `Scenario` (`.ini` via `SceneLoader` / `.bin` via `FlatBuffersScenario` + `FBSceneAdapter`)
- Runs **three threads**:
  - **Graphics thread** (main): input → `drawFrame()` → render. Consumes the front `SimulationState` snapshot.
  - **Physics jthread**: fixed-timestep loop — `SpringSystem` → `EntityManager::UpdateCpuStages()` → `NetworkBridge::BroadcastOwnedStates()` → `NetworkBridge::UpdateRemoteEntities()` → snapshot to back-buffer `SimulationState`.
  - **Networking jthread**: `NetworkService::Poll()` loop, pinned to Cores 2–3 via `SetThreadAffinityMask`.
- Double-buffered `SimulationState` (front/back swap) decouples physics and render rates.
- Shadow pipeline is engine-scoped (`m_shadowPipeline`); material pipelines are scenario-scoped via `Scenario::GetPipelines()`

**Scene-change safety rule (CRITICAL):** Always call `requestScenarioChange(path)` to queue a deferred scene switch. It is processed inside `drawFrame()` after `vkDeviceWaitIdle()`, making it GPU-safe. Never call `changeScenario()` directly from ImGui callbacks or any non-render context — it destroys Vulkan pipelines synchronously and causes validation errors.

### ECS Core (`include/ecs/EntityManager.h`)

- `EntityManager` manages entity lifecycles and component arrays
- Components are plain structs: `Transform`, `MeshRenderer`, `LightComponent`, `RigidBody`, `PhysicsComponent`, `ParticleComponent`, `SkyboxComponent`, `ClothComponent`, `FlockingComponent`, `AnimatedObjectComponent`, `SpawnerComponent`
- Systems implement **`ICpuSystem`** (CPU-only work, overrides `OnUpdate(float dt)`) or **`IGpuSystem`** (GPU work, overrides `OnUpdate(float dt, VkCommandBuffer cb)`). Both are sub-interfaces of `IECSystem`. Never subclass `IECSystem` directly.
- Systems are registered via `EngineServiceRegistry` (registry pattern); scenario-specific systems are registered in `Scenario::OnLoad()` and unregistered in `Scenario::OnUnload()`.
- Namespace: `GE::ECS::`

### Rendering Pipeline (`include/graphics/Renderer.h`)

Multi-pass forward renderer, ECS-aware (queries component arrays each frame):
1. **Shadow pass** — depth-only render (engine-scoped pipeline)
2. **Opaque pass** — Phong/Gouraud forward shading
3. **Transparent pass** — alpha-blended geometry + particle effects

`VulkanDevice` manages the Vulkan device/swapchain. `GpuResourceManager` handles GPU buffer/image allocation.

### Scenario & Scene System

- `Scenario` — abstract base (State Pattern); concrete implementations: `GenericScenario`, `SnowGlobeScenario`, `FlatBuffersScenario`
- Each `Scenario` owns its material pipelines (`m_pipelines`, `m_shaderModules`) — created in `createMaterialPipelines()`
- **Legacy path:** `SceneLoader` parses `.ini` files → `GenericScenario` / `SnowGlobeScenario`
- **FlatBuffers path (primary):** `FlatBuffersScenario` reads `.bin` files via `FlatBuffersLoader`, then `FBSceneAdapter::Adapt()` converts FlatBuffers tables into ECS entities. The schema is `flatbuffers/Scene.fbs`; `Scene_generated.h` is patched by hand (do not regenerate — flatc version mismatch produces incompatible enum style).
- `FlatBuffersScenario` registers scenario-specific systems (`ClothSystem`, `FlockingSystem`, `AnimationSystem`, `SpawnerSystem`, `PhysicsSystem`) in `OnLoad()` and unregisters them in `OnUnload()`.
- `.bin` scenes are loaded from `config/flatbufferConfig/` (including subdirectories — the scanner uses `recursive_directory_iterator`).
- `Scene` maps named entities to `EntityID`s for `.ini`-based scenarios only.

### Service Locator

`include/core/ServiceLocator.h` provides global access to core services (input, time, assets, etc.) — used instead of passing references through the call stack.

## Code Conventions

- **Namespaces:** `GE::` (engine root), `GE::ECS::`, `GE::Components::`, `GE::Graphics::`, `GE::Systems::`, `GE::Scene::`, `GE::Assets::`, `GE::Physics::`, `GE::Networking::`
- **Naming:** `CamelCase` for classes/methods, `m_camelCase` for member variables, `UPPER_SNAKE_CASE` for constants
- **Vocabulary:** `System` = IECSystem subclass; `Backend` = GPU compute/render backend; `Service` = stateless engine service; `Source` = scene illumination object
- **Memory:** RAII throughout; prefer `std::unique_ptr`. Custom `SimpleAllocator` (TLSF) used for GPU memory
- **C++ standard:** C++20 (`/std:c++20`)
- Headers go in `/include/<domain>/`, implementations in `/source/<domain>/`

## Key Subsystems at a Glance

| Subsystem | Entry Point | Notes |
|---|---|---|
| Physics | `systems/PhysicsSystem.h/.cpp` | Gravity, RigidBody (mass/inertia/torque), Euler/Semi-Implicit/RK4, sphere-sphere + sphere-plane impulse response, MaterialInteractionRegistry |
| Cloth sim | `systems/ClothSystem.h/.cpp`, `components/ClothComponent.h` | Verlet integration, explicit spring list (structural/shear/flexion), tearing, burning with heat accumulation |
| Flocking | `systems/FlockingSystem.h/.cpp`, `components/FlockingComponent.h` | Reynolds boids (sep+align+cohesion), obstacle avoidance; spatial modes: BruteForce / UniformGrid / Octree |
| Animation | `systems/AnimationSystem.h/.cpp`, `components/AnimationComponents.h` | Waypoint interpolation, LINEAR/SMOOTHSTEP easing, STOP/LOOP/REVERSE path modes |
| Spawner | `systems/SpawnerSystem.h/.cpp` | Entity pool pre-creation, SingleBurst / Repeating, FixedLocation / RandomBox / RandomSphere, owner-assigned peer |
| Spring | `systems/SpringSystem.h/.cpp` | Hooke + damping springs between entity pairs |
| Networking | `networking/NetworkService.h`, `core/NetworkBridge.h`, `networking/Packets.h` | Winsock2 UDP P2P (4 peers), dead reckoning + blend correction (120ms), scene-change sync |
| FlatBuffers | `scene/FlatBuffersScenario.h`, `scene/fb/FBSceneAdapter.h` | Data-driven scene loading from `.bin`; schema in `flatbuffers/Scene.fbs` |
| Particles | `particles/GpuParticleBackend.h`, `systems/ParticleEmitterSystem.h` | GPU compute shaders (snow, rain, fire, dust, smoke) |
| Camera | `graphics/Camera.h` | Projection/view matrices |
| Input | `services/InputService.h/.cpp` | GLFW callbacks |
| UI | `services/DebugOverlay.h/.cpp` | ImGui debug overlay — use `requestScenarioChange()` here, never `changeScenario()` |
| Assets | `assets/AssetManager.h/.cpp` | OBJ loading, texture caching |
| Climate | `services/ClimateService.h` | Wind/temperature driving particle behaviour |
| Lighting | `lighting/LightSource.h`, `PointLightSource.h` | Scene illumination objects |
| Logging | `core/Logger.h/.cpp` | Severity-level logging |
| Common defs | `core/Common.h` | Global constants, error codes, UBO structs |
| Post-process | `graphics/PostProcessBackend.h` | Offscreen + composite passes |
| Orchestrator | `core/EngineOrchestrator.h` | RAII master; owns all subsystems; 3-thread model |

## Markdown Reference Documents

All lab and workshop documents are in `markdown/`. Images are in `markdown-resources/Simulation-Lab/<N>/`.

| File | Purpose |
|---|---|
| `SimulationWorkshops.md` | Workshop briefs for 700105_A25_T2 (Simulation & Concurrency). Workshop 2.2: sandbox creation (state pattern, primitives, start/stop/pause, timestep). Workshop 3.2: add velocity, integration methods, basic collision. |
| `SimulationLab2.md` | Lab 2 brief — ECS foundations, scenario switching, snow globe. |
| `SimulationLab3.md` | Lab 3 brief — PhysicsObject/RigidBody, integration methods (Euler/Semi-Implicit/RK4), gravity, sphere-plane collision. |
| `SimulationLab4.md` | Lab 4 brief — Collision response via impulse: fixed-object, same/different-mass ball-ball, elasticity. |
| `Simulation and Concurrency - Final Lab.md` | **Primary final lab brief** for 700105. Specifies Level 1/2/3 targets, threading requirements, networking requirements, cloth and flocking Level 2 features. |
| `Test Cases.md` | **TC-001–TC-010** step-by-step test cases covering scene reset, cloth wind/tearing/burning, flocking, spawner networking, animation, dead reckoning, affinity, and scene switching. |
| `Claude - Refactor Plan.md` | Full 21-step refactoring plan. Phases 1 & 2 complete; Phase 3 mostly complete (only Steps 17 & 18 remain). |
| `Gemini - Refactor Plan.md` | Broader 4-phase architectural roadmap. |
| `C++ Programming and Design - Real-time Graphics - Final Lab README.md` | Submission README for the other module's final lab. |

## Refactoring Status

The engine has undergone a 3-phase refactor. **Phases 1, 2, and most of Phase 3 are complete.**

See `markdown/Claude - Refactor Plan.md` for the full 21-step plan.

**Done:** Steps 1–21 except 17 and 18. Notably:
- Step 19 (`SceneLoader` handler-map OCP refactor) — done via `FBSceneAdapter` replacing the old inline switch
- Step 20 (`ICpuSystem` / `IGpuSystem` ISP split) — **done**; `IECSystem.h` contains both sub-interfaces
- Step 21 (`GpuUploadContext` struct wrapping `Scenario::OnLoad` Vulkan params) — **done**

**Remaining technical debt (Steps 17 & 18 only):**
- **Step 17**: `ParticleComponent` still holds `shared_ptr<GpuParticleBackend>` — pointer chase every particle tick; change to `uint32_t` emitter index
- **Step 18**: `Transform::m_children` is `vector<uint32_t>` — heap alloc per entity in packed component array; change to a parent-index field
