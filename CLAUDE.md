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
├── include/                            # ~70 headers (all subsystems)
│   ├── Components.h                    # All ECS component structs
│   ├── EntityManager.h                 # ECS core — entity lifecycle & component arrays
│   ├── IECSystem.h / SystemFactory.h   # ECS system interface & registry
│   ├── Experience.h                    # RAII master orchestrator
│   ├── VulkanEngine.h                  # Device/swapchain management
│   ├── VulkanContext.h                 # Vulkan instance/surface/device
│   ├── VulkanResourceManager.h         # GPU buffer/image allocation
│   ├── Renderer.h                      # Multi-pass forward renderer
│   ├── Scenario.h / Scene.h            # State pattern base + scene graph
│   ├── SceneLoader.h                   # .ini parser → entity instantiation
│   ├── ServiceLocator.h                # Global service access
│   ├── PhysicsSystem.h / Physics.h     # Gravity, AABB, math helpers
│   ├── ParticleSystem.h / ParticleEmitterSystem.h  # GPU compute particles
│   ├── ClimateManager.h                # Wind/temperature → particle behaviour
│   ├── AssetManager.h                  # OBJ loading, texture caching
│   ├── Camera.h                        # Projection/view matrices
│   ├── InputManager.h                  # GLFW callbacks
│   ├── IMGUIManager.h                  # ImGui debug overlay
│   ├── Logger.h                        # Severity-level logging
│   ├── Common.h                        # Global constants, error codes, UBO structs
│   └── SimpleAllocator.h               # TLSF GPU memory allocator
├── source/                             # ~37 .cpp implementations
│   └── main.cpp                        # Entry point → Experience
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
├── markdown/                           # Lab book reference documents
└── books/								# Collection of relevant textbooks (e.g. Real-Time Rendering, Physically Based Rendering)
```

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
