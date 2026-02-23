# Claude Refactor Plan

## Engine Refactoring Plan (As of 21/02/2026)

### The Root Problems

Tree overlapping issues were identified across all 82 headers and 37 source files:

1. "System" is used for three unrelated things — ECS update drivers (`IECSystem`), a legacy lifecycle interface (`ISystem`), and a factory registry (`SystemFactory`)
2. ~50 classes live in the global namespace — only `GE::ECS::`, `GE::Components::`, `GE::Systems::`, `GE::Physics::`, and `GE::Utilities::` are properly namespaced
3. `/include/` is a flat dump of 82 files with no domain grouping

---

#### Vocabulary Contract (Enforced Going Forward)

| Suffix | Meaning |
| :--- | :--- |
| **System** | Exclusively ECS update-loop drivers (IECSystem subclasses) |
| **Backend** | GPU compute/render backends driven by a System |
| **Service** | Stateless or near-stateless engine services |
| **Manager** | Only where stateful lifecycle + ownership genuinely applies |
| **Source** | Scene illumination objects (Light hierarchy) |

---

### Section 1 - Rename Table

#### 1A - Retire the ISystem Hierarchy

Current: `ISystem`
Proposed: (deleted)
Namespace: —
Rationale: Provides no value — all subclasses only write stub update() overrides. Retire entirely.

---


Current: `ParticleSystem`
Proposed: `GpuParticleBackend`
Namespace: `GE::Graphics::`
Rationale: It is a GPU compute+draw backend, not an ECS system. Driven by `ParticleEmitterSystem`.

---


Current: `PostProcessor`
Proposed: `PostProcessBackend`
Namespace: `GE::Graphics::`
Rationale: Same pattern. Not an ECS system.

---


Current: `Light`
Proposed: `LightSource`
Namespace: `GE::`
Rationale: Disambiguates from `LightComponent` (the ECS component).

---


Current: `PointLight`
Proposed: `PointLightSource`
Namespace: `GE::`
Rationale: Follows from `LightSource`.

---


Current: `ClimateManager`
Proposed: `ClimateService`
Namespace: `GE::`
Rationale: Pure logic state, no GPU pipeline, no ECS coupling. "Service" fits.

#### 1B - ECS Systems (Already Correct — Namespace Only)

All three stay named as-is. Confirm namespace `GE::Systems::` for all:
- `TransformSystem`, `PhysicsSystem`, `ParticleEmitterSystem`

#### 1C - Orchestrator & Registry

Current: `Experience`
Proposed: `EngineOrchestrator`
Namespace: `GE::`
Rationale: "`Experience`" is domain-opaque. `EngineOrchestrator` states the RAII master role clearly.

---


Current: `SystemFactory`
Proposed: `EngineServiceRegistry`
Namespace: `GE::`
Rationale: It is a string-keyed registry for creator lambdas, not an ECS system factory.

---


Current: `ServiceLocator`
Proposed: `ServiceLocator`
Namespace: `GE::`
Rationale: Accurate name. Move into `GE::`.

#### 1D - Standalone Managers

Current: `TimeManager`
Proposed: `TimeService`
Namespace: `GE::`
Rationale: Stateless dt/totalTime calculator.

---


Current: `InputManager`
Proposed: `InputService`
Namespace: `GE::`
Rationale: GLFW callback bridge.

---


Current: `IMGUIManager`
Proposed: `DebugOverlay
Namespace: `GE::`
Rationale: Composes and renders a debug UI layer, not managing ImGui.

---


Current: `StatsManager`
Proposed: `PerformanceTracker`
Namespace: `GE::`
Rationale: A circular-buffer FPS accumulator.

---


Current: `AssetManager`
Proposed: `AssetManager`
Namespace: `GE::`
Rationale: Retains "Manager" - genuine stateful lifecycle with GPU ownership.

#### 1E - Vulkan/Graphics Layer

Current: `VulkanEngine`
Proposed: `VulkanDevice`
Namespace: `GE::Graphics::`
Rationale: Manages physical/logical device only. Not "the engine".

---


Current: `VulkanResourceManager`
Proposed: `GpuResourceManager`
Namespace: `GE::Graphics::`
Rationale: Remove API coupling from the name.

---


Current: `VulkanContext`
Proposed: `VulkanContext`
Namespace: `GE::Graphics::`
Rationale: Well-named, just needs namespacing.

---


Current: `SwapChain`
Proposed: `SwapChain`
Namespace: `GE::Graphics::`
Rationale: Just needs namespacing.

---


Current: `SyncManager`
Proposed: `FrameSyncManager`
Namespace: `GE::Graphics::`
Rationale: Disambiguates from other sync concepts.

---


Current: `Pipeline`
Proposed: `GraphicsPipeline`
Namespace: `GE::Graphics::`
Rationale: Pipeline is too generic.

---


Current: `Image`
Proposed: `GpuImage`
Namespace: `GE::Graphics::`
Rationale: Image collides mentally with every stdlib concept.

---

Current: `RenderPass`, `ShaderModule`, `Texture`, `Skybox`, `Cubemap`
Proposed: (unchanged)
Namespace: `GE::Graphics::`
Rationale: Well-named, just need namespacing.

#### 1F - Asset Layer

| Current | Proposed | Namespace |
| :--- | :--- | :--- |
| Model, Mesh, Material, OBJLoader, GeometryUtils, Vertex | (unchanged) | `GE::Assets::` |

#### 1G - Struct Member Naming (Convention Violations)

Structs that are missing the m_ prefix on members:

| Struct | Members to Prefix | File |
| :--- | :--- | :--- |
| **Particle** | position, velocity, color | Particle.h |
| **SubMesh** | mesh, material | Components.h |
| **SparkLight** | position, color | Common.h |

Access is by name only (no offsetof usage), so this is safe to rename mechanically.

---

### Section 2 - Namespace Hierarchy

| Namespace | Contents / Members |
| :--- | :--- |
| `GE::` | Scenario, EngineOrchestrator, IEngineService → (deleted), EngineServiceRegistry, ServiceLocator, AssetManager, TimeService, InputService, PerformanceTracker, DebugOverlay, ClimateService, LightSource, PointLightSource |
| `GE::ECS::` | EntityManager, Entity, ComponentArray, IComponentArray, ComponentType, IECSystem, ESystemStage |
| `GE::Components::` | ALL components in one place (see Section 3 below) |
| `GE::Systems::` | TransformSystem, PhysicsSystem, ParticleEmitterSystem |
| `GE::Scene::` | Scene, SceneLoader, EntityFactory |
| `GE::Graphics::` | VulkanContext, VulkanDevice, GpuResourceManager, SwapChain, FrameSyncManager, RenderPass, GraphicsPipeline, ShaderModule, GpuImage, Texture, Skybox, Cubemap, Renderer, GpuParticleBackend, PostProcessBackend |
| `GE::Assets::` | Model, Mesh, Material, OBJLoader, GeometryUtils, Vertex |
| `GE::Physics::` | Physics, Physics2D, Collider, Sphere, Plane, Capsule, Cylinder, Ray, RaycastHit, RaycastHit2D, Line |
| `GE::EngineConstants::` | All constants from current EngineConstants:: (moved under `GE::`) |
| `GE::Utilities::` | Logger, SafeShutdown, SafeDelete |

---

### Section 3 - Resolve the Component Namespace Split

Currently Transform and Tag live in `GE::Scene::Components::` while everything else is in `GE::Components::`. There is no semantic reason for this split.

Action: Move `Transform` and `Tag` into `GE::Components::`. Eliminate `GE::Scene::Components::` entirely.

Files requiring updates (~8 total): `TransformSystem.h/.cpp`, `PhysicsSystem.cpp`, `SceneLoader.cpp`, `Scene.cpp`,
`ParticleEmitterSystem.h`, `GenericScenario.cpp`, `SnowGlobeScenario.cpp`.

---

### Section 4 - Retire ISystem

`ISystem` provides no real value. Every subclass only writes a stub update(float) override {}. The concrete resolution:

- Remove public `ISystem` from `Light`, `ClimateManager`, `ParticleSystem`, `PostProcessor`
- Delete the stub update(float) overrides from each
- EngineOrchestrator owns these objects as specific-typed unique_ptrs and calls their real methods directly
- Delete `ISystem.h`

IECSystem stays unchanged as the sole system interface.

Note on `IECSystem` signature: `OnUpdate(float dt, VkCommandBuffer cb)` passes a Vulkan handle to `PhysicsSystem` and
`TransformSystem` which don't use it. This is an ISP violation. Flagged as Phase 3 work (see below).

---

### Section 5 - SRP Audit Findings

Experience / EngineOrchestrator — Confirmed God Class

Owns 15 distinct subsystem unique_ptrs, the pipeline registry, shader registry, mesh/model cache, global UBO,
scenario, and GLFW window. Three separable concerns:

1. Vulkan bootstrap (initVulkan, createGraphicsPipelines, initSkybox) — could be a free function bootstrapEngine(EngineOrchestrator&)
2. Frame loop (drawFrame, updateUniformBuffer, run) — tightly coupled to swapchain; Major redesign, Phase 3
3. Resource ownership vs scenario lifecycle — pipelines, shaderModules, ownedModels, meshes vectors are scenario-scoped resources sitting in the orchestrator. Moving them into Scenario or a SceneResources RAII wrapper is
the highest-value SRP fix that doesn't require redesign

Immediate actionable fix: Move the pipelines, shaderModules, ownedModels, and meshes vectors out of EngineOrchestrator
and into Scenario base class.

SceneLoader — Latent OCP Violation

Has 11 private handle*() methods — one per component type. Every new component type requires modifying SceneLoader.
The fix is a handler-map pattern. Flagged for Phase 3.

VulkanDevice — Acceptable Scope

Manages device selection, logical device, queues, swapchain, depth buffer, command pool, and MSAA. It is a lot, but it
is all in one domain. Delegation to SwapChain, Image, and RenderPass is correct. No decomposition needed now.

---

### Section 6 - File Organization

include/ proposed subfolders:

| include/    |                                                                                                                                                                                                                                       |
| :---------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| core/       | Common.h, Logger.h, libs.h, ServiceLocator.h, EngineOrchestrator.h                                                                                                                                                                    |
| ecs/        | Entity.h, EntityManager.h, ComponentArray.h, IComponentArray.h, ComponentType.h, IECSystem.h                                                                                                                                          |
| components/ | Transform.h, Tag.h, Components.h, PhysicsComponents.h, ParticleComponent.h, SkyboxComponent.h                                                                                                                                         |
| systems/    | TransformSystem.h, PhysicsSystem.h, ParticleEmitterSystem.h, EngineServiceRegistry.h                                                                                                                                                  |
| scene/      | Scene.h, SceneLoader.h, EntityFactory.h, Scenario.h, GenericScenario.h, SnowGlobeScenario.h                                                                                                                                           |
| graphics/   | VulkanContext.h, VulkanDevice.h, GpuResourceManager.h, VulkanUtils.h, SwapChain.h, FrameSyncManager.h, RenderPass.h, GraphicsPipeline.h, Renderer.h, PostProcessBackend.h, ShaderModule.h, GpuImage.h, Texture.h, Skybox.h, Cubemap.h |
| particles/  | GpuParticleBackend.h, Particle.h                                                                                                                                                                                                      |
| assets/     | AssetManager.h, Model.h, Mesh.h, Material.h, OBJLoader.h, Vertex.h, GeometryUtils.h, ConfigLoader.h                                                                                                                                   |
| physics/    | Physics.h, Physics2D.h, Ray.h, Sphere.h, Plane.h, Capsule.h, Cylinder.h, Line.h, Collider.h                                                                                                                                           |
| lighting/   | LightSource.h, PointLightSource.h                                                                                                                                                                                                     |
| services/   | TimeService.h, InputService.h, PerformanceTracker.h, DebugOverlay.h, ClimateService.h                                                                                                                                                 |
| memory/     | SimpleAllocator.h, MemoryUtilities.h                                                                                                                                                                                                  |

source/ mirrors this subfolder structure 1:1.

---

### Section 7 - Critical Bugs Found During Analysis

These are active bugs, not just style issues:

1. `ParticleComponent` includes `ParticleSystem.h` directly — a component header transitively pulls in Vulkan pipeline
types, compute shader setup, and SSBO creation. This is a layering violation. Fix: replace `shared_ptr<ParticleSystem>`
with a `uint32_t` emitter index; the ECS system holds the backends in a pool.
2. `Transform::m_children` is `std::vector<uint32_t>` inside a packed ComponentArray — this means every entity with
children performs a heap allocation within the component's packed storage, destroying cache locality. Fix: flatten the
hierarchy into a parent-index field and traverse it at system update time.
3. `Experience::meshes` stores raw non-owning `Mesh*` pointers — Model owns its `Mesh` instances; if a scenario unloads and
meshes isn't simultaneously cleared, these become dangling pointers. Fix: move meshes into Scenario ownership (the
Phase 3 SRP fix resolves this as a side effect).

---

### Section 8 - Migration Phases

#### Phase 1 - Purely Mechanical (Safe, Commit Each Step Separately)

1. Merge `Transform`/`Tag` → `GE::Components::`, delete `GE::Scene::Components::` (highest value)
2. Add `GE::Graphics::` namespace to all Vulkan/rendering classes
3. Add `GE::Assets::` namespace to `Model`, `Mesh`, `Material`, `OBJLoader`, etc.
4. Move `EngineConstants::` → `GE::EngineConstants::`
5. Rename `VulkanEngine` → `VulkanDevice`
6. Rename `SystemFactory` → `EngineServiceRegistry`
7. Rename `Experience` → `EngineOrchestrator`
8. Rename remaining classes per tables 1A–1G
9. Add `m_` prefix to `Particle`, `SubMesh`, `SparkLight` members
10. Create the include/ + source/ subfolder structure, move files, update all `#include` paths
11. Update `.vcxproj` and `.vcxproj.filters` for new paths

#### Phase 2 - Interface Cleanup (Requires Call-Site Changes)

12. Delete `ISystem` — remove inheritance from `Light`, `ClimateManager`, `ParticleSystem`, `PostProcessor`; remove stub
overrides
13. Rename `ParticleSystem` → `GpuParticleBackend`; update `ParticleComponent` shared_ptr type
14. Rename `PostProcessor` → `PostProcessBackend`; update `EngineOrchestrator`
15. Rename `Light`/`PointLight` → `LightSource`/`PointLightSource`; update all call sites

#### Phase 3 - Structural Redesign (Requires Design Decisions First)

16. Move scenario-scoped resources (pipelines, shaderModules, ownedModels, meshes) from `EngineOrchestrator` into
Scenario — resolves both the SRP violation and the dangling `Mesh*` bug
17. Fix `ParticleComponent` to use index-based emitter reference (resolves the include-graph layering violation)
18. Fix `Transform::m_children` from vector to parent-index field (resolves cache locality issue)
19. `SceneLoader` handler-map refactor (OCP fix)
20. Split `IECSystem` into `ICpuSystem` / `IGpuSystem` (ISP fix — separate design discussion required)
21. Wrap `Scenario::OnLoad` Vulkan parameters into a GpuUploadContext opaque struct

---

### Structural Invariant to Enforce

After Phase 2, this dependency rule must hold: nothing in `GE::Systems::`, `GE::Scene::`, or `GE::` (service layer) may
include a Vulkan header directly. The only remaining violation after Phase 2 will be Scenario::OnLoad's raw Vulkan
parameter list — addressed in Phase 3, item 21.

✻ Sautéed for 5m 45s

- Agent "Engine architecture refactoring plan" completed

- Let me save the key architectural findings to memory before they're lost.

- Recalled 1 memory, wrote 2 memories (ctrl+o to expand)

- Memory saved. The plan above is complete. A few things worth calling out that the agents found by reading the actual source code (not just the architecture description):

Three active bugs discovered:

1. `ParticleComponent.h` directly `#includes ParticleSystem.h` — any entity that gets a `ParticleComponent` transitively
pulls in full Vulkan pipeline types and SSBO setup into a component header. Components must never include system
implementations.
2. `IECSystem::GenerateISystemTypeID()` has a dual-definition problem — it is declared with external linkage at line 26
of `IECSystem.h` but also has an inline body at line 66. One of those definitions is dead, or there's a latent ODR
issue. Worth auditing early in Phase 1.
3. `Experience::meshes` is `vector<Mesh*>` (raw non-owning pointers) — `Model` owns the `Mesh` instances, but `Experience`
caches raw pointers separately. If a scenario unloads and meshes isn't simultaneously cleared, those are dangling
pointers. The Phase 3 SRP fix (moving those vectors into Scenario) resolves this as a side effect.

The plan is structured so Phase 1 is entirely mechanical find-and-replace work with no logic risk - good for a
dedicated refactoring session. Want me to start executing any specific phase?

✻ Churned for 1m 5s

- Agent "SOLID/naming refactor analysis" completed

- The second agent has finished - its output matches the plan I already delivered above. Everything is in sync and memory is saved.

The plan is complete. Ready to start executing whenever you are - Phase 1 is all mechanical work (no logic risk), so
it's a good place to begin. Want to tackle a specific step, or should I start from step 1 (merging `Transform`/`Tag` into
`GE::Components::`)?