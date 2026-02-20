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

### Top-Level Orchestration

`main.cpp` → `Experience` (RAII master orchestrator) owns all subsystems:
- Initializes `VulkanEngine` → `VulkanContext` → `VulkanResourceManager`
- Loads a `Scenario` via `SceneLoader` (reads `.ini` config files)
- Runs the main loop: input → update systems → render

### ECS Core (`include/EntityManager.h`)

- `EntityManager` manages entity lifecycles and component arrays
- Components are plain structs: `Transform`, `MeshRenderer`, `LightComponent`, `PhysicsComponent`, `ParticleComponent`, `SkyboxComponent` (all defined in `include/Components.h`)
- Systems implement `IECSystem` and are registered via `SystemFactory` (registry pattern)
- Namespace: `GE::ECS::`

### Rendering Pipeline (`include/Renderer.h`)

Multi-pass forward renderer, ECS-aware (queries component arrays each frame):
1. **Shadow pass** — depth-only render
2. **Opaque pass** — Phong/Gouraud forward shading
3. **Transparent pass** — alpha-blended geometry + particle effects

`VulkanEngine` manages the Vulkan device/swapchain. `VulkanResourceManager` handles GPU buffer/image allocation.

### Scenario & Scene System

- `Scenario` — abstract base (State Pattern); `GenericScenario` and `SnowGlobeScenario` are concrete implementations
- `SceneLoader` parses `.ini` files in `/config/` and instantiates entities
- `Scene` maps named entities to `EntityID`s and drives procedural animations
- Active scenario config files: `config/snow_globe.ini`, `config/simulation_lab2.ini`, `config/simulation_lab3.ini`

### Service Locator

`include/ServiceLocator.h` provides global access to core services (input, time, assets, etc.) — used instead of passing references through the call stack.

## Code Conventions

- **Namespaces:** `GE::` (engine root), `GE::ECS::`, `GE::Components::`, `GE::Graphics::`
- **Naming:** `CamelCase` for classes/methods, `m_camelCase` for member variables, `UPPER_SNAKE_CASE` for constants
- **Memory:** RAII throughout; prefer `std::unique_ptr`. Custom `SimpleAllocator` (TLSF) used for GPU memory
- **C++ standard:** C++20 (`/std:c++20`)
- Headers go in `/include/`, implementations in `/source/`

## Key Subsystems at a Glance

| Subsystem | Entry Point | Notes |
|---|---|---|
| Physics | `PhysicsSystem.h/.cpp` | Gravity, AABB collisions, `Physics.h` for math |
| Particles | `ParticleSystem.h`, `ParticleEmitterSystem.h` | GPU compute shaders (snow, rain, fire, dust, smoke) |
| Camera | `Camera.h` | Projection/view matrices |
| Input | `InputManager.h/.cpp` | GLFW callbacks |
| UI | `IMGUIManager.h/.cpp` | ImGui debug overlay |
| Assets | `AssetManager.h/.cpp` | OBJ loading, texture caching |
| Climate | `ClimateManager.h` | Wind/temperature driving particle behaviour |
| Logging | `Logger.h/.cpp` | Severity-level logging |
| Common defs | `Common.h` | Global constants, error codes, UBO structs |
