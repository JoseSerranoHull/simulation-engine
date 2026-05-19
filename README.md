# 700105 Final Lab: Simulation and Concurrency

---

## MSc Computer Science for Games Programming

---

**Course**: 700105 — Simulation and Concurrency

**Name**: JOSE JAVIER SERRANO SOLIS

**Instructors:** Simon Grey & Warren Viant

---

# Introduction

This is the final lab assignment for **700105 — Simulation and Concurrency** by JOSE JAVIER SERRANO SOLIS, submitted as part of the **MSc Computer Science for Games Programming** programme.

The project implements a real-time networked physics simulation built on a custom Vulkan engine. Up to four peers connect over LAN and each simulates their own colour-coded entities (Red / Green / Blue / Yellow) using a fixed-step physics accumulator, impulse-based collision response, cloth simulation, Reynolds boid flocking (CPU and GPU), and a data-driven FlatBuffers scene system. Three concurrent threads — graphics, physics, and networking — are pinned to dedicated CPU cores via `SetThreadAffinityMask`.

<!-- TODO: Run 01_multiplayer.bin with 2+ peers connected. Screenshot the engine window showing the arena with colour-coded player spheres and animated platforms. Save as markdown-resources/FinalLab700105/readme_hero.png -->
![Engine running: four-peer multiplayer session in the open-box arena with colour-coded player spheres, animated platforms, and static geometry](markdown-resources/FinalLab700105/readme_hero.png)

---

# Project Preferences

The project targets **Windows only** and is built with **Visual Studio 2022**. Both Release and Debug configurations are included. Adjust include/library paths for your local setup if necessary.

- **General**
  - Configuration Type: Application (.exe)
  - Windows SDK Version: 10.0 (latest installed version)
  - Platform Toolset: Visual Studio 2022 (v143)
  - C++ Language Standard: ISO C++20 Standard (`/std:c++20`)
  - C Language Standard: Default (Legacy MSVC)

- **C/C++ Additional Include Directories**
  - `$(ProjectDir)\external-libraries\glm`
  - `$(ProjectDir)\external-libraries\imgui`
  - `$(ProjectDir)\external-libraries\glfw-3.4.bin.WIN64\include`
  - `$(ProjectDir)\external-libraries\stb`
  - `$(VulkanSDK)\1.4.313.2\Include` (RBB-335 lab machines)
  - `$(VulkanSDK)\1.4.328.1\Include` (student laptop)

- **Linker Additional Library Directories**
  - `$(ProjectDir)\external-libraries\glfw-3.4.bin.WIN64\lib-vc2022`
  - `C:\VulkanSDK\1.4.313.2\Lib` (RBB-335)
  - `C:\VulkanSDK\1.4.328.1\Lib` (student laptop)

### Building

```bash
# Release build
msbuild simulation-engine.sln /p:Configuration=Release /p:Platform=x64

# Debug build
msbuild simulation-engine.sln /p:Configuration=Debug /p:Platform=x64
```

Output binary: `x64\Release\simulation-engine.exe` or `x64\Debug\simulation-engine.exe`

Shaders (GLSL `.vert`, `.frag`, `.comp`) must be compiled to SPIR-V separately using `glslc` or `glslangValidator`. Pre-compiled `.spv` files are committed to the repository. To recompile all shaders run `shaders\compile.bat`.

---

# VS Solution File Structure

The Visual Studio solution is organised into the following filters:

- **Source Files** — all `.cpp` files; `main.cpp` is the application entry point.
- **Header Files** — all `.h`/`.hpp` files, grouped by domain subfolder (`core/`, `ecs/`, `components/`, `systems/`, `scene/`, `graphics/`, `particles/`, `assets/`, `physics/`, `lighting/`, `services/`, `memory/`, `networking/`).
- **Shader Files** — GLSL sources (`.vert`, `.frag`, `.comp`) and compiled `.spv` binaries.
- **Config Files** — FlatBuffers scene binaries (`config/flatbufferConfig/`) and legacy `.ini` scenes.
- **Markdown Files** — lab books and technical documentation.
  - Final lab book for **Simulation and Concurrency**: [`markdown/Final Lab Book - Simulation and Concurrency (700105).md`](markdown/Final%20Lab%20Book%20-%20Simulation%20and%20Concurrency%20%28700105%29.md)
  - Full Technical Manual: [`markdown/TechnicalManual/README.md`](markdown/TechnicalManual/README.md)
- **IMGUI Files** — vendored Dear ImGui library (read-only; never modified).
- **FlatBuffers Files** — `flatbuffers/Scene.fbs` schema and `flatbuffers/Scene_generated.h`.

---

# Project Folder File Structure

```
simulation-engine/
├── config/
│   ├── flatbufferConfig/
│   │   ├── showcases/          # Active demo scenes (.bin loaded at runtime)
│   │   │   ├── 01_multiplayer.bin
│   │   │   ├── 02_shapes_and_materials.bin
│   │   │   ├── 03_animation_platforms.bin
│   │   │   ├── 04_spawner_factory.bin
│   │   │   ├── 05_cloth_simulation.bin
│   │   │   └── 06_flocking_boids.bin
│   │   └── tests/              # Test scenes
│   └── *.ini                   # Legacy scenarios
├── external-libraries/         # glm, glfw, imgui, stb (vendored)
├── flatbuffers/                # Scene.fbs schema + generated header
├── include/                    # Headers organised by domain
├── markdown/                   # Lab books and technical manual
├── markdown-resources/         # Images referenced by markdown files
├── models/                     # OBJ assets
├── setup_firewall.bat          # Run as admin on each lab machine (UDP ports 54000-54003)
├── shaders/                    # GLSL sources + compiled .spv
├── source/                     # C++ implementation files
└── textures/                   # Texture assets and cubemaps
```

---

# Technical Manual

A full chapter-by-chapter technical manual is located in [`markdown/TechnicalManual/`](markdown/TechnicalManual/README.md). It covers:

| Chapter | Topic |
|---|---|
| 00 | Getting Started — build, run, firewall setup |
| 01 | ECS Core — EntityManager, ComponentArray, system stages |
| 02 | Threading and Concurrency — 3-thread model, affinity, double buffer |
| 03 | Vulkan and Rendering — multi-pass pipeline, shadow, particles |
| 04 | Scene Management — FlatBuffers path, SceneLoader, scenario switching |
| 05 | Physics System — integration methods, 10-pass collision, impulse response |
| 06 | Cloth and Flocking — Jakobsen constraints, wind, burning, Reynolds boids, GPU compute |
| 07 | UDP Networking — P2P discovery, dead reckoning, ownership, packet format |
| 08 | Scripting and Services — ScriptComponent, ServiceLocator |
| 09 | Animation and Spawning — waypoint interpolation, entity pool, prefab templates |
| 10 | Asset Pipeline — OBJ loading, texture caching, GeometryUtils |
| 11 | Particle System — GPU compute shaders, GpuParticleBackend |
| 12 | ImGui, Input, Debug — DebugOverlay, InputService, ColliderVisualizer |

---

# Engine Systems

## Threading Model

Three threads run concurrently from startup, each pinned to dedicated CPU cores:

| Thread | Core(s) | Affinity mask | Frequency |
|---|---|---|---|
| Graphics (main thread) | Core 1 | `0x01` | 0–300 Hz (slider) |
| Networking (`std::jthread`) | Cores 2–3 | `0x06` | Tight poll, 1 ms yield |
| Physics (`std::jthread`) | Core 4 | `0x08` | 1–2000 Hz (slider) |

Physics and graphics rates are decoupled via a double-buffered `SimulationState[2]`. After each physics tick the completed snapshot is published atomically with `memory_order_release`; the renderer acquires it with `memory_order_acquire` so neither thread ever blocks the other.

<!-- TODO: Open Windows Resource Monitor (resmon.exe) → CPU tab while the engine is running. Screenshot the per-core CPU graph showing distinct activity on Core 1, Cores 2–3, and Core 4. Save as markdown-resources/FinalLab700105/readme_thread_affinity.png -->
![Windows Resource Monitor: Core 1 driven by the render loop, Cores 2–3 by the networking poll, Core 4 by the physics accumulator](markdown-resources/FinalLab700105/readme_thread_affinity.png)

## Physics

`PhysicsSystem` runs a fixed-step accumulator (default 120 Hz, up to 2000 Hz). Three integration methods are selectable at runtime: **Euler**, **Semi-Implicit Euler** (default — symplectic, energy-conserving), and **RK4**. Gravity is applied as a force each tick; damping uses framerate-independent exponential decay.

Collision resolution runs **10 sequential narrow-phase passes** per tick covering sphere–plane, sphere–sphere, sphere–box, box–plane, capsule–plane, cylinder–plane, capsule–box, sphere–capsule, box–box (SAT), and cylinder–sphere/box. Impulse-based response with positional correction ensures non-penetration in a single step. A `MaterialInteractionRegistry` maps material-name pairs to per-pair restitution and friction values.

<!-- TODO: Run 02_shapes_and_materials.bin. Drop several shapes so they land and stack on the floor plane. Screenshot showing visible collisions. Save as markdown-resources/FinalLab700105/readme_physics.png -->
![Rigid-body shapes — sphere, cuboid, capsule — resting on the ground after impulse-based collision resolution, colour-coded by owner](markdown-resources/FinalLab700105/readme_physics.png)

## Cloth Simulation

`ClothSystem` uses **Jakobsen positional constraint projection** — unconditionally stable at any physics frequency. Wind is modelled as a per-triangle aerodynamic panel force with sinusoidal gusts and per-particle hash turbulence. Burning uses **Laplacian heat diffusion** across the spring graph with multiple simultaneous `BurnSource` origins. A four-stage heat colour gradient (cold → yellow → orange → red → charred) is applied to both the PBR fabric texture and a flat-colour pass. Stress accumulation on springs enables realistic tear propagation. Cloth geometry can be resized and density-adjusted at runtime without reallocating GPU buffers (pre-allocated at load time to 80×80 vertices).

<!-- TODO: Run 05_cloth_simulation.bin. Apply wind and burn in several spots. Screenshot the cloth with the four-stage heat gradient and some torn springs visible. Save as markdown-resources/FinalLab700105/readme_cloth.png -->
![Cloth simulation: aerodynamic wind deformation and heat-diffusion burning showing the four-stage colour gradient with torn spring edges](markdown-resources/FinalLab700105/readme_cloth.png)

## Flocking and Steering

`FlockingSystem` implements **Reynolds boids** (separation, alignment, cohesion) with static obstacle avoidance via Weighted Truncated Sum. Three CPU spatial-partitioning backends are available at runtime: **BruteForce** (O(N²)), **UniformGrid** (O(N + k)), and **Octree** (O(N log N + k)). A fourth mode — **GPU BruteForce** — offloads the entire steering loop to the `flock_brute.comp` Vulkan compute shader using ping-pong SSBOs, with a `VkBufferMemoryBarrier` synchronising compute writes to the point-sprite vertex pass. Switching modes live is supported without restarting the simulation.

<!-- TODO: Run 06_flocking_boids.bin. Switch to Octree or GPU mode in the ImGui Flocking menu. Screenshot the agents mid-flight with the Flocking panel visible. Save as markdown-resources/FinalLab700105/readme_flocking.png -->
![60 flocking agents using Octree spatial partitioning; Reynolds boids produce emergent grouping around static sphere obstacles](markdown-resources/FinalLab700105/readme_flocking.png)

## Networking

Up to four peers connect over LAN using **Winsock2 UDP**. Auto-discovery broadcasts `DiscoveryHello` packets carrying `scenePath[128]`; the host replies and relays one `DiscoveryResponse` per already-connected peer, giving a joiner the full topology in a single round trip. Peers on different scenes are filtered by an atomic `m_acceptedPeerMask` bitmask. Each entity carries an `OwnerComponent` (ONE–FOUR); only the owning peer runs physics for its entities. Remote entities use **dead reckoning** (120 ms blend window) so state updates absorb network jitter without visible teleporting. Scene changes are **global UI** events — any peer can switch the scene for everyone via `DebugOverlay` → scene list.

<!-- TODO: With 2+ peers connected, open the ImGui Network menu. Screenshot the Connected Peers list showing peer IDs, IP addresses, and the current scene path. Save as markdown-resources/FinalLab700105/readme_network.png -->
![ImGui Network panel: Connected Peers list with peer IDs, IP:port entries, and scene-matched auto-connect status](markdown-resources/FinalLab700105/readme_network.png)

> **Firewall note:** Run `setup_firewall.bat` as Administrator on each lab machine before connecting. It opens inbound UDP on ports 54000–54003.

## Spawner System

`SpawnerSystem` creates physics entities at runtime from **PrefabTemplate** blueprints loaded from the FlatBuffers scene. Two spawn modes are supported: **Single Burst** (N entities once, after a start delay) and **Repeating** (one entity every interval, up to a maximum count). Three location modes: **Fixed**, **Random Box**, and **Random Sphere**. When `ownerPeerId > 0`, only the owning peer fires the spawner; a `SpawnObject` UDP packet replicates the spawn to all other peers with the same entity ID.

## Animation System

`AnimationSystem` interpolates entities along waypoint paths using **LINEAR** or **SMOOTHSTEP** easing, with three path modes: **STOP**, **LOOP**, and **REVERSE**. Animated platforms contribute their `linearVelocity` to collision impulse calculations, producing correct conveyor-belt momentum transfer to physics objects without being deflected themselves.

## Particle System

Five GPU compute particle effects are implemented via `GpuParticleBackend`: **snow**, **rain**, **fire**, **dust**, and **smoke**. Each effect advances particle state entirely on-device per frame with no CPU readback.

## Demo Scenes

| Scene | Description |
|---|---|
| `01_multiplayer.bin` | Open-box arena — 4 player spheres, animated platforms, static geometry |
| `02_shapes_and_materials.bin` | All shape types (sphere, cuboid, capsule, cylinder) with material interaction registry |
| `03_animation_platforms.bin` | Waypoint-animated platforms with momentum transfer |
| `04_spawner_factory.bin` | Burst and repeating spawners with owner-colour prefabs |
| `05_cloth_simulation.bin` | Interactive cloth with wind, burning, tearing, and resize controls |
| `06_flocking_boids.bin` | 60 boid agents with BruteForce / UniformGrid / Octree / GPU spatial modes |

---

# Video Showcase

<!-- TODO: Record a walkthrough video demonstrating: multiplayer connection, physics/collision, cloth burning, flocking GPU mode, and scene switching. Save as videos/FinalLabVideo_700105.mp4 -->

The video showcase of the final lab project can be found at:

```
simulation-engine/
└── videos/
    └── FinalLabVideo_700105.mp4
```
