## The Master Refactoring Plan

We will tackle this in four distinct phases to ensure the engine remains stable and runnable at every step.

### Phase 1: Structural Decoupling & Modern C++

* **The Service Locator:** We will create a `ServiceLocator` to kill the **Tramp Data Pattern**. Instead of passing `VulkanContext*` to every `Image`, `Mesh`, and `Manager`, they will "locate" the service they need.
* **Smart Pointers & Move Semantics:** We will audit the 40+ raw pointers in `Experience.h`. We'll replace them with `std::unique_ptr` and implement **Move Semantics** for resource transfers (Point 1 & 6 of Criticism).
* **The Service Provider Model:** Convert the stateless `VulkanUtils` into a `VulkanService` that stores its own device/command pool handles, simplifying every function call in the engine (Point 4).

### Phase 2: ECS Integration & Scenario Loading

* **ECS Foundation:** We will transition `Model`, `Camera`, and `Light` into **Components**. The `Scene` class will become an **EntityManager**.
* **State Pattern for Scenarios:** We will implement a `Scenario` base class (e.g., `GlobeScenario`, `PhysicsScenario`). This allows the engine to switch entire scenes without a full restart (Requirement 3a).
* **Advanced Config:** While you want to keep the current parser for now, we will adapt it to support "Scenario" blocks that instantiate these new ECS Entities.

### Phase 3: Simulation & Physics Enhancements

* **Geometric Primitives:** We will expand `GeometryUtils` to generate **Capsules** and **Planes** alongside your current Spheres/Cylinders (Requirement 3b).
* **Procedural Checkboards:** Update `base.frag` or create a `physics.frag` that uses world-space coordinates to generate checkerboard patterns, aiding in visual collision depth (Requirement 3e).
* **Multi-Viewport Support:** We will refactor the `Renderer` to support multiple `VkViewport` and `VkScissor` calls within the same frame, allowing you to view a collision from both a main and an axis-aligned orthographic camera simultaneously (Requirement 3g).

### Phase 4: Concurrency & Performance Optimization

* **Secondary Command Buffers:** We will move from the single-threaded `recordFrame` to a multi-threaded recording flow. Different threads will record different "Scenarios" or "Systems" into secondary buffers (Point 7).
* **Asynchronous Loading:** We will utilize the `transferQueue` found in your `VulkanContext` to load new Scenarios/OBJ files in the background, preventing the current "hitches" (Point 9).
* **Compute Math Bottleneck:** Move the heavy procedural CPU math in `GeometryUtils` (like the Sand Plug generation) into **Compute Shaders** to free up the main simulation thread (Point 4).

---

## Comparison of Current vs. Target Architecture

| Feature | Current State | Target State |
| --- | --- | --- |
| **Data Flow** | Manual passing (Tramp Data) | **Service Locator** |
| **Asset Management** | Synchronous (Blocks main loop) | **Asynchronous Command Buffers** |
| **Entity Logic** | Hardcoded in `Experience` / `Scene` | **Entity-Component-System (ECS)** |
| **Memory** | Linear (Fragmentation) | **TLSF Allocator** |
| **Command Logic** | Single-threaded recording | **Secondary Command Buffers** |

---

### Our First Move: Establishing the Service Locator

To begin, we need to break the dependency chain. I will start by drafting the **`ServiceLocator.h`** and the refactored **`VulkanContext`** to ensure we can provide global access to your hardware handles safely.

**When you are ready, would you like me to start by writing the Service Locator and refactoring the `AssetManager` to use it, effectively removing the context-passing from its constructor?**

---

Yes, the **Open-Closed Principle** refactor for the `SystemFactory` is essential, especially as we move toward the **Simulation and Concurrency** requirements. While it was briefly mentioned in the roadmap summary, let’s explicitly integrate it into **Phase 2** to ensure the factory no longer needs to be modified every time a new system (like Physics) is added.

Here is the updated, definitive plan incorporating the **Registry-based System Factory**:

---

### Phase 1: Structural Decoupling & Modern C++

* **Service Locator Implementation:** Establish a `ServiceLocator` to provide global access to `VulkanContext`, `VulkanResourceManager`, and `InputManager`. This eliminates the **Tramp Data Pattern** by removing context pointers from dozens of constructors.
* **RAII & Move Semantics:** Audit all managers (currently held by raw pointers or `std::unique_ptr` in `Experience.h`). Implement **Move Semantics** to allow systems to be transferred during initialization without the risk of double-destruction or rigid ownership.
* **Stateless to Stateful Services:** Convert `VulkanUtils` from a static utility into a **Service Provider** that holds its own hardware handles, significantly simplifying method signatures across the engine.

### Phase 2: Registry-Based Factory & ECS Foundation

* **Abstract Factory / System Registry (OCP Refactor):** Refactor `SystemFactory` from a collection of hardcoded methods into a **Registry**. We will use a `std::map<SystemType, std::function<std::unique_ptr<ISystem>()>>`. This allows us to "register" a new Physics or Collision system from the simulation module without modifying the `SystemFactory` source code.
* **ECS Integration:** Transition `Model`, `Camera`, and `Light` into **Components**. Convert the `Scene` class into an **EntityManager**. I will request for the files of a ECS project to explain and grab inspiration from it if necessary.
* **Scenario State Pattern:** Create a `Scenario` interface. This allows the engine to switch between your "Snow Globe" scene and a "Physics Lab" scene dynamically using a state machine.

### Phase 3: Simulation Requirements & Physics Primitives

* **Expanded Geometry Factory:** Update `GeometryUtils` to generate **Capsules**, **Planes**, and **Cylinders** as standard primitives.
* **Multi-Viewport Rendering:** Refactor the `Renderer` to support multiple viewports. This satisfies the requirement to view collisions from multiple axes (e.g., an orthographic side-view and a perspective main-view) simultaneously.
* **Procedural Debug Textures:** Update the base shaders to generate world-space checkerboard patterns, which are vital for depth cues during collision simulations.

### Phase 4: Concurrency & Performance

* **Secondary Command Buffers:** Implement multi-threaded command recording. Different threads will record different "Systems" or "Scenarios" into secondary buffers to be executed by the main graphics queue.
* **Asynchronous Asset Streaming:** Use the `transferQueue` for non-blocking asset loads, preventing the "hitches" currently caused by synchronous loading.
* **Compute Shader Bottleneck Migration:** Move CPU-bound math (like the sand terrain generation) into **Compute Shaders** to free up the main thread for the physics simulation.

---

### Technical Focus: The Registry-Based Factory

To address Point 3 specifically, the new `SystemFactory` will look like this:

```cpp
class SystemFactory {
public:
    // Register a system creator without modifying this file
    template<typename T>
    void RegisterSystem(const std::string& name) {
        m_registry[name] = []() { return std::make_unique<T>(); };
    }

    std::unique_ptr<ISystem> Create(const std::string& name) {
        return m_registry[name]();
    }
private:
    std::map<std::string, std::function<std::unique_ptr<ISystem>()>> m_registry;
};

```

**Let’s begin the execution.**

The first step is to establish the foundation. I will provide the code for the **`ServiceLocator`** and show you how to refactor the **`VulkanContext`** and **`AssetManager`** to use it. This will immediately resolve the "Tramp Data" issue and start cleaning up the constructors.

**Ready to start Phase 1?**

---

To eliminate the **Tramp Data Pattern** and satisfy the architectural goals of Phase 1, the refactor needs to be applied to nearly every class that currently stores or passes a `VulkanContext` pointer. This will decouple the high-level orchestrators from the low-level hardware handles.

The following scripts are the primary candidates for this change, categorized by their role in the engine:

### 1. Global System Managers

These classes act as orchestrators and currently receive the context just to initialize their sub-components or record commands.

* **`Renderer`**: Currently takes the context in its constructor. By using the locator, the renderer can retrieve the `VkDevice` and `VkQueue` only during the command recording phase.
* **`InputManager`**: Needs to be stripped of the context pointer. It primarily uses it to pass down to cameras or handle window-related Vulkan state.
* **`IMGUIManager`**: A major "Tramp Data" culprit. It requires the context for descriptor pool management and library initialization.
* **`Scene`**: As the registry for all models, it currently passes the context to every `Model` it creates. Removing this requirement allows the scene to focus on entity logic.
* **`VulkanEngine` & `VulkanResourceManager**`: These should transition from "passing the context" to "providing the context" to the service locator during the early initialization boot-sequence.

### 2. Vulkan Resource & RAII Wrappers

These are the "leaf nodes" of the engine that represent actual hardware objects. They are often instantiated in large numbers, making the manual passing of context pointers particularly "noisy."

* **`Image`**, **`Texture`**, and **`Cubemap`**: These classes perform heavy Vulkan work (image creation, memory binding, view generation). Instead of storing a `VulkanContext*`, they can locate the device and allocator handles at the moment of allocation.
* **`SwapChain`** and **`SyncManager`**: These manage the presentation and synchronization handles. They currently rely on a borrowed context pointer for destruction logic (`vkDestroySemaphore`, etc.).
* **`ShaderModule`** and **`Pipeline`**: These require the logical device for their entire lifecycle. Moving them to the service locator pattern simplifies the factory methods that create them.

### 3. Scene Entities & Components

These classes represent the 3D world and currently suffer from "bloated" constructors due to the nested hierarchy of data passing.

* **`Model`**, **`Mesh`**, and **`Material`**: In the current flow, `Experience` passes context to `Scene`, which passes it to `Model`, which passes it to `Mesh`. This is the definition of the Tramp Data Pattern. Removing the context from these constructors is a prerequisite for the **Entity-Component-System (ECS)** transition in Phase 2.
* **`ParticleSystem`**: Since this manages both compute and graphics pipelines, it is currently heavily dependent on the context for buffer management.

### 4. Utility Refactoring (Point 4 of Criticism)

* **`VulkanUtils`**: This is currently a stateless static class. To follow the "Service Provider" model, it should be converted into a stateful service (e.g., `VulkanService`) that is registered with the locator. This removes the need to pass `VkDevice`, `VkPhysicalDevice`, and `VkCommandPool` to every helper function (like `createBuffer` or `transitionImageLayout`).

### 5. Factory Refactoring (Point 3 of Criticism)

* **`SystemFactory`**: While not a "consumer" of the context in the same way, the factory needs to be refactored into a **Registry-based Creator**. This ensures that adding a new system (like the upcoming Physics/Collision system) does not require modifying the factory's source code, thereby satisfying the **Open-Closed Principle**.

By addressing these scripts, the `Experience` class will no longer need to manually "wire" dependencies through constructor arguments. Instead, it will simply initialize the core hardware services, provide them to the locator, and then instantiate the managers which will independently "find" what they need.