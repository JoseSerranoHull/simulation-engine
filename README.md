# 700106/700120 Final Lab: Sandy-Snow Globe / Snowy-Sand Globe

---

## MSc Computer Science for Games Programming

---

**Course**: 700120 - C++ Programming and Design | 700106 - Real-Time Graphics

**Name**: JOSE JAVIER SERRANO SOLIS

**Instructor:** Warren Viant | Dr. Qingde Li, Dr. Xihui Ma

---

# Introduction

This is the final lab assignment for the course 700106/700120 by JOSE JAVIER SERRANO SOLIS for the subjects of Real-Time Graphics and C++ Programming and Design for the course of **MSc Computer Science for Games Programming**. 
In this lab, you will create a Sandy-Snow Globe or Snowy-Sand Globe using various programming techniques and concepts learned throughout the course.

![Project Running](<./markdown-resources/Final Lab Book - Real-Time Graphics (700106)/img1.png> "Program running")

---

# Project Preferences

Currently the project is set both for Release and Debug configurations with  the paths set for the include directories and library directories. Make sure to adjust these paths according to your local setup if necessary.
- General:
    - Configuration Type: Application (.exe)
    - Windows SDK Version: 10.0 (latest installed version)
    - Platform Toolset: Visual Studio 2022 (v143)
    - C++ Language Standard: ISO C++20 Standard (/std:c++20)
    - C Language Standard: Default (Legacy MSVC)

- C/C++ Additional Include Directories:
	- C:\Software\tinyobjloader\include (This is here because of the Vulkan Tutorial Template, it is not used)
	- C:\Software\stb\include
	- C:\Software\glfw-3.4.bin.WIN64\
	- $(ProjectDir)\external-libraries\glm
	- $(ProjectDir)\external-libraries\imgui\imgui-1.92.5
	- $(ProjectDir)\external-libraries\glfw-3.4.bin.WIN64\include
	- $(ProjectDir)\external-libraries\stb
	- $(VulkanSDK)\1.4.313.2\Include (RB335)
	- $(VulkanSDK)\1.4.321.1\Include (Student's laptop)

- Linker Additional Library Directories:
	- C:\VulkanSDK\1.4.328.1\Lib
	- C:\Software\glfw-3.4.bin.WIN64\lib-vc2022
	- $(ProjectDir)\external-libraries\glfw-3.4.bin.WIN64\lib-vc2022
	- C:\VulkanSDK\1.4.313.2\Lib (RB335)
	- C:\VulkanSDK\1.4.321.1\Lib (Student's laptop)


---

# VS Solution File Structure

The project is organized into the following directories:
- **Source Files**: Filter that contains all the .cpp files. `main.cpp` is the entry point of the application.
- **Header Files**: Filter that contains all the .h/.hpp files. Some files do not have a corresponding .cpp file as they are header-only libraries.
- **Shader Files**: Filter that contains all the GLSL shader files used in the project.
- **Config Files**: Filter that contains configuration files such as `txt` files.
- **Markdown Files**: Filter that contains all the .md files. Here's where the Lab Books are located.
    - You can access the final lab book for **Real-Time Graphics** [here](</markdown/Final Lab Book - Real-Time Graphics (700106).md>).
    - You can access the complete lab books (1 - 8) for **Real-Time Graphics** [here](</markdown/Complete Lab Book (1 - 8) - Real-Time Graphics.md>).
    - You can access the final lab book for **C++ Programming and Design** [here](</markdown/Final Lab Book - C++ Programming and Design (700120).md>).
    - You can access the complete lab books (A - H) for **C++ Programming and Design** [here](</markdown/Complete Lab Book (A - H) - C++ Programming and Design.md>).
- **IMGUI Files**: Filter that contains all the files related to the IMGUI library.

---

# Project Folder File Structure

The project folder is organized into the following folders:
- **config**: Contains configuration files such as `txt` files.
- **external-libraries**: Contains the libraries for `glm`, `glfw` and `imgui`.
- **markdown**: Contain must of the markdown files (With an exception of the README).
- **markdown-resources**: Contains the media files needed for the markdown files.
- **models**: Contains the diverse folders that have `obj` files used in the project.
- **report**: Contains the parasoft report files in `html` format.
- **shaders**: Contains the diverse folders that have `vert`, `frag`, `comp` and its respective `spv` shader files used in the project.
- **textures**: Contains the diverse folders for the texture used in the project.
- **videos**: Contains the video showcase of the final lab project.

---

# Technical Manual

A full chapter-by-chapter technical manual for the engine is located in [`markdown/TechnicalManual/`](markdown/TechnicalManual/README.md). It covers ECS architecture, threading, Vulkan rendering, physics, cloth & flocking, networking, scripting, and the spawner system.

---

# Engine Systems

## Spawner System

The spawner system creates physics entities at runtime from **PrefabTemplate** blueprints — the same pattern used in Unity (`Instantiate`) and Unreal (Spawn Actor). It replaced an earlier entity-pool approach where entities were pre-created at scene load and "teleported" into position.

### How It Works

1. **Scene Load** — `FBSceneAdapter::adaptPrefabs()` reads each `[Prefab]` block from the `.bin` scene file and builds a `PrefabTemplate` struct containing: shape kind, physics parameters (mass, restitution, inertia tensor), and pre-uploaded GPU meshes for up to four owner-color variants plus one material/textured fallback.

2. **Runtime Spawn** — `SpawnerSystem::OnUpdate()` advances a timer each physics tick. When the timer fires (after `startTime`, every `interval` for repeating spawners, or once for burst), it calls `EntityFactory::InstantiatePrefab`:

   - Creates a new `EntityID`
   - Adds `Transform` (position + pre-computed world matrices so no flash appears at the origin)
   - Adds `RigidBody` (gravity enabled, initial linear + angular velocity from random ranges)
   - Adds the correct collider (`SphereCollider`, `BoxCollider`, `CapsuleCollider`, `CylinderCollider`)
   - Adds `MeshRenderer` pointing to the selected owner-color or material mesh
   - Adds `ScriptComponent` if the prefab declares a script type

3. **Hierarchy Display** — Spawned entities are root ECS entities (no parent) to keep physics correct. The DebugOverlay hierarchy panel uses `SpawnerComponent::spawnedEntityIds` to display them visually nested under their spawner node.

4. **Networking** — When `ownerPeerId > 0`, only the owning peer fires the spawner. After local instantiation, it sends a `SpawnObject` UDP packet so remote peers call `EntityFactory::InstantiatePrefab` with the same parameters and entity ID.

### Spawn Modes

| Mode | Behaviour |
|------|-----------|
| **Single Burst** | Spawns `burstCount` entities once, after `startTime` seconds |
| **Repeating** | Spawns one entity every `interval` seconds, up to `maxCount` total |

### Spawn Locations

| Location | Behaviour |
|----------|-----------|
| **Fixed** | Always at the same world position |
| **Random Box** | Uniform random inside an AABB defined by `boxMin`/`boxMax` |
| **Random Sphere** | Uniform random inside a sphere (uses cube-root sampling for unbiased volume distribution) |

### ImGui Controls (Spawners menu)

Each spawner in the scene appears as a row in the **Spawners** top menu:
- **Progress bar** — `spawnedCount / maxCount`
- **Pause / Resume** — freeze the timer without resetting count
- **Reset** — restart the timer and count from zero; old spawned entities remain in the world as independent physics objects
- **Fire** — immediately spawn one entity regardless of timer state

### Color Assignment

When `use_owner_colors = true` in the scene JSON, each spawner selects a mesh color based on:
- **Sequential mode** (`is_sequential = true`): cycles colors 0→1→2→3 per spawn, useful for visual variety in single-player scenes
- **Owner mode**: all entities from peer N use peer N's color (red/blue/green/yellow)

---

# Builded Release Executable

The repository includes a `exe` named "Vulkan-clean" that can be executed directly without the need to build the project again. This executable is located in the **root folder**.

---

# Video Showcase

The video showcase of the final lab project can be found at:

lab-i-final-lab-JoseSerranoHull/
 |- videos/
	|- FinalLabVideo.mp4


<video controls>
  <source src="/videos/FinalLabVideo.mp4" type="video/quicktime">
  Your browser does not support MP4 videos.
</video>
