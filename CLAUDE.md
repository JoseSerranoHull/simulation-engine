# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

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
├── config/                             # Scenario .ini config files
│   ├── snow_globe.ini
│   ├── simulation_lab2.ini
│   └── simulation_lab3.ini
├── include/                            # Headers organised by domain subfolder
│   ├── core/         Common.h, Logger.h, libs.h, ServiceLocator.h, EngineOrchestrator.h
│   ├── ecs/          Entity.h, EntityManager.h, ComponentArray.h, IComponentArray.h, ComponentType.h, IECSystem.h
│   ├── components/   Transform.h, Tag.h, Components.h, PhysicsComponents.h, ParticleComponent.h, SkyboxComponent.h
│   ├── systems/      TransformSystem.h, PhysicsSystem.h, ParticleEmitterSystem.h, EngineServiceRegistry.h
│   ├── scene/        Scene.h, SceneLoader.h, Scenario.h, GenericScenario.h, SnowGlobeScenario.h
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
│   └── main.cpp                        # Entry point → EngineOrchestrator
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
- Loads a `Scenario` via `SceneLoader` (reads `.ini` config files)
- Runs the main loop: input → update systems → render
- Shadow pipeline is engine-scoped (`m_shadowPipeline`); material pipelines are scenario-scoped via `Scenario::GetPipelines()`

### ECS Core (`include/ecs/EntityManager.h`)

- `EntityManager` manages entity lifecycles and component arrays
- Components are plain structs: `Transform`, `MeshRenderer`, `LightComponent`, `PhysicsComponent`, `ParticleComponent`, `SkyboxComponent`
- Systems implement `IECSystem` and are registered via `EngineServiceRegistry` (registry pattern)
- Namespace: `GE::ECS::`

### Rendering Pipeline (`include/graphics/Renderer.h`)

Multi-pass forward renderer, ECS-aware (queries component arrays each frame):
1. **Shadow pass** — depth-only render (engine-scoped pipeline)
2. **Opaque pass** — Phong/Gouraud forward shading
3. **Transparent pass** — alpha-blended geometry + particle effects

`VulkanDevice` manages the Vulkan device/swapchain. `GpuResourceManager` handles GPU buffer/image allocation.

### Scenario & Scene System

- `Scenario` — abstract base (State Pattern); `GenericScenario` and `SnowGlobeScenario` are concrete implementations
- Each `Scenario` owns its material pipelines (`m_pipelines`, `m_shaderModules`) — created in `createMaterialPipelines()`
- `SceneLoader` parses `.ini` files in `/config/` and instantiates entities
- `Scene` maps named entities to `EntityID`s and drives procedural animations
- Active scenario config files: `config/snow_globe.ini`, `config/simulation_lab2.ini`, `config/simulation_lab3.ini`

### Service Locator

`include/core/ServiceLocator.h` provides global access to core services (input, time, assets, etc.) — used instead of passing references through the call stack.

## Code Conventions

- **Namespaces:** `GE::` (engine root), `GE::ECS::`, `GE::Components::`, `GE::Graphics::`, `GE::Systems::`, `GE::Scene::`, `GE::Assets::`, `GE::Physics::`
- **Naming:** `CamelCase` for classes/methods, `m_camelCase` for member variables, `UPPER_SNAKE_CASE` for constants
- **Vocabulary:** `System` = IECSystem subclass; `Backend` = GPU compute/render backend; `Service` = stateless engine service; `Source` = scene illumination object
- **Memory:** RAII throughout; prefer `std::unique_ptr`. Custom `SimpleAllocator` (TLSF) used for GPU memory
- **C++ standard:** C++20 (`/std:c++20`)
- Headers go in `/include/<domain>/`, implementations in `/source/<domain>/`

## Key Subsystems at a Glance

| Subsystem | Entry Point | Notes |
|---|---|---|
| Physics | `systems/PhysicsSystem.h/.cpp` | Gravity, AABB collisions, `Physics.h` for math |
| Particles | `particles/GpuParticleBackend.h`, `systems/ParticleEmitterSystem.h` | GPU compute shaders (snow, rain, fire, dust, smoke) |
| Camera | `graphics/Camera.h` | Projection/view matrices |
| Input | `services/InputService.h/.cpp` | GLFW callbacks |
| UI | `services/DebugOverlay.h/.cpp` | ImGui debug overlay |
| Assets | `assets/AssetManager.h/.cpp` | OBJ loading, texture caching |
| Climate | `services/ClimateService.h` | Wind/temperature driving particle behaviour |
| Lighting | `lighting/LightSource.h`, `PointLightSource.h` | Scene illumination objects |
| Logging | `core/Logger.h/.cpp` | Severity-level logging |
| Common defs | `core/Common.h` | Global constants, error codes, UBO structs |
| Post-process | `graphics/PostProcessBackend.h` | Offscreen + composite passes |
| Orchestrator | `core/EngineOrchestrator.h` | RAII master; owns all subsystems |

## Markdown Reference Documents

All lab and workshop documents are in `markdown/`. Images are in `markdown-resources/Simulation-Lab/<N>/`.

| File | Purpose |
|---|---|
| `SimulationWorshops.md` | Workshop briefs for 700105_A25_T2 (Simulation & Concurrency). Workshop 2.2: sandbox creation (state pattern, primitives, start/stop/pause, timestep). Workshop 3.2: add velocity, integration methods, basic collision. |
| `SimulationLab2.md` | Lab 2 brief — ECS foundations, scenario switching, snow globe. |
| `SimulationLab3.md` | Lab 3 brief (due 26/02/26) — PhysicsObject class (and/or with use of a RigidBody component), ball movement with integration methods (Euler/Semi-Implicit/RK4), gravity, sphere-plane collision detection. |
| `SimulationLab4.md` | Lab 4 brief (due 05/03/26) — Collision response via impulse: fixed-object, same-mass and different-mass ball-ball collisions, elasticity. |
| `Claude - Refactor Plan.md` | Full 21-step refactoring plan (our active execution guide). Phases 1 & 2 complete; Phase 3 in progress. |
| `Gemini - Refactor Plan.md` | Broader 4-phase architectural roadmap (replaced original MasterPlan.md). |
| `Final Lab README.md` | Submission README for the final lab books. |

## Refactoring Status

The engine has undergone a 3-phase refactor. **Phases 1 and 2 are complete** (Steps 1–15). **Phase 3 is in progress** (Step 16 done).

See `markdown/Claude - Refactor Plan.md` for the full 21-step plan. Remaining Phase 3 steps (17–21):
- **Step 17**: Fix `ParticleComponent` to use `uint32_t` emitter index instead of `shared_ptr<GpuParticleBackend>`
- **Step 18**: Fix `Transform::m_children` from `vector<uint32_t>` to a parent-index field
- **Step 19**: `SceneLoader` handler-map OCP refactor
- **Step 20**: Split `IECSystem` into `ICpuSystem` / `IGpuSystem` (ISP fix)
- **Step 21**: Wrap `Scenario::OnLoad` Vulkan parameters into a `GpuUploadContext` struct
