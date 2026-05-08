# Chapter 12 — ImGui, Input & Debug Systems

> **Goal:** Understand how the engine's runtime UI is built, how input is handled, and how to
> use the debug and logging systems to diagnose problems.

---

## 12.1 What Is Dear ImGui?

**Dear ImGui** is an **immediate-mode GUI** library. Unlike retained-mode UIs (HTML, Qt, WinForms)
where you create widgets once and they persist, ImGui widgets are described every frame from scratch.

Think of it like a painter re-painting the entire canvas every frame, versus a sculptor who
shapes clay once and leaves it in place.

### Why Immediate-Mode Fits a Game Loop

```
Retained-mode (traditional):
  - Startup: create Button "Pause"
  - On event: if button pressed → callback fires
  - Requires: widget lifetime management, event routing

Immediate-mode (ImGui):
  - Every frame: if (ImGui::Button("Pause")) { isPaused = !isPaused; }
  - No callbacks. The return value IS the event.
  - Widget lifetime = one frame.
```

This is natural for game loops because the game loop runs every frame anyway. There are no
dangling widget references, no event queues, no lifecycle complexity.

### The Basic ImGui Frame Pattern

```cpp
// 1. Start new frame (integrations update input state):
ImGui_ImplVulkan_NewFrame();
ImGui_ImplGlfw_NewFrame();
ImGui::NewFrame();

// 2. Describe UI for this frame:
if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("Simulation")) {
        ImGui::Checkbox("Paused", &m_paused);
        ImGui::SliderFloat("Physics Hz", &physicsHz, 1.0f, 2000.0f);
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

// 3. Finalise draw data:
ImGui::Render();

// 4. Submit draw commands (inside Vulkan render pass):
ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
```

The engine wraps this in `DebugOverlay::update()` (steps 1–3) and `DebugOverlay::draw()` (step 4).

---

## 12.2 Integrating ImGui with Vulkan

**Files:**
- `external-libraries/imgui/backends/imgui_impl_vulkan.h` (read-only)
- `external-libraries/imgui/backends/imgui_impl_glfw.h` (read-only)
- `include/services/DebugOverlay.h`

### Requirements for ImGui + Vulkan

1. **Separate descriptor pool** — ImGui needs its own `VkDescriptorPool` for its internal
   font atlas and texture bindings. The engine creates a dedicated pool with large limits:
   ```cpp
   // In DebugOverlay::init()
   VkDescriptorPoolSize pool_sizes[] = {
       { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
       { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
       // ... other types ...
   };
   VkDescriptorPoolCreateInfo pool_info{};
   pool_info.flags   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
   pool_info.maxSets = 1000;
   vkCreateDescriptorPool(device, &pool_info, nullptr, &imguiPool);
   ```

2. **Render pass compatibility** — ImGui renders into the swapchain's final color attachment
   during the last subpass. The render pass passed to `ImGui_ImplVulkan_Init` must match the
   render pass used in `draw()`.

3. **Draw order** — ImGui commands must be recorded **after** all 3D passes, so it composites
   on top of the scene. In `drawFrame()`, `uiManager->draw(cb)` is the last call before
   `vkEndRenderPass`.

4. **Font upload** — After init, `ImGui_ImplVulkan_CreateFontsTexture()` must be called with a
   one-time command buffer, then `ImGui_ImplVulkan_DestroyFontUploadObjects()` once submitted.

### Init Sequence (DebugOverlay::init)

```cpp
void DebugOverlay::init(GLFWwindow* window, const VulkanDevice* engine) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Backend init:
    ImGui_ImplGlfw_InitForVulkan(window, true);

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance      = context->instance;
    init_info.PhysicalDevice = context->physicalDevice;
    init_info.Device        = context->device;
    init_info.Queue         = context->graphicsQueue;
    init_info.DescriptorPool = imguiPool;
    init_info.MinImageCount = 2;
    init_info.ImageCount    = swapchainImageCount;
    init_info.MSAASamples   = context->msaaSamples;

    ImGui_ImplVulkan_Init(&init_info, renderPass);
}
```

---

## 12.3 DebugOverlay

**File:** `include/services/DebugOverlay.h`

`DebugOverlay` owns the ImGui Vulkan integration lifecycle and coordinates all debug UI.

### Key Methods

| Method | Called By | Purpose |
|--------|----------|---------|
| `init(window, engine)` | `EngineOrchestrator` constructor | Sets up ImGui context and descriptor pool |
| `update(input, stats, light, time, climate)` | `drawFrame()` before command buffer | Calls `ImGui::NewFrame()`, builds menus |
| `draw(cb)` | `drawFrame()` inside render pass | Calls `ImGui::Render()` + `ImGui_ImplVulkan_RenderDrawData()` |
| `cleanup()` | `EngineOrchestrator` destructor | Destroys pool, calls `ImGui_ImplVulkan_Shutdown()` |

### Delegation to Scenarios

`DebugOverlay::update()` calls `activeScenario->OnGUI()` to let each scenario add its own
menus. This is why the Cloth, Flocking, Spawner, and Network menus only appear when a
`FlatBuffersScenario` is active:

```cpp
// In DebugOverlay::update()
void DebugOverlay::update(...) const {
    // ... begin frame, draw global menus (Scenes, Display, Debug) ...

    auto* scenario = ServiceLocator::GetOrchestrator()->GetCurrentScenario();
    if (scenario) {
        scenario->OnGUI();  // FlatBuffersScenario adds: Physics, Cloth, Flocking, etc.
    }

    ImGui::EndMainMenuBar();
}
```

### The Entity Inspector

`DebugOverlay` contains an `Inspector m_inspector` and tracks `m_selectedEntity`. Clicking on
any entity in the hierarchy tree (via `DrawEntityNode`) selects it and shows its components:
transform values, RigidBody velocity, physics flags, etc.

---

## 12.4 The Runtime Menu Tree

Every menu in the engine corresponds to a specific system or service it writes to:

```
Main Menu Bar
│
├── Scenes          → DebugOverlay: calls requestScenarioChange(path)
├── Display
│   ├── Shading     → InputService: setGouraudEnabled, usePhong
│   ├── Shadows     → InputService: setGlobalShadowsEnabled
│   └── Bloom       → InputService: setBloomEnabled
│
├── Simulation      → FlatBuffersScenario: m_paused, physicsHz, graphicsHz
│   ├── Pause/Step
│   └── Hz Sliders
│
├── Physics         → PhysicsSystem members:
│   ├── Integration method (Euler/SemiImplicit/RK4)
│   ├── Gravity on/off
│   ├── Solver iterations
│   └── Restitution override
│
├── Cloth           → ClothComponent fields on active cloth entity:
│   ├── Wind controls (windX, windZ, dragCoeff, gustAmplitude)
│   ├── Burn (add source, clear, burnRate)
│   ├── Tearing (tearThreshold, tearRoughness, stressTransferRate)
│   └── Debug Visualization sub-menu
│
├── Flocking        → FlockingSystem + FlockingComponent:
│   ├── Spatial mode (BruteForce / UniformGrid / Octree)
│   ├── Grid cell size
│   ├── Weight sliders (separation, alignment, cohesion)
│   ├── Freeze/Restart
│   └── Performance counters (neighbour checks, update time)
│
├── Network         → NetworkBridge + NetworkService:
│   ├── Auto Connect (host IP, status)
│   └── Manual Config (peer IPs/ports, Connect button)
│
├── Spawner         → SpawnerSystem: active spawner count display
│
├── Climate         → ClimateService: wind vector, temperature
│
└── Debug
    ├── Collider Wireframes → ColliderVisualizerSystem toggle
    ├── Cloth Debug Lines   → ClothSystem debug buffer toggle
    ├── FPS / Frame Time    → PerformanceTracker
    └── Entity Inspector    → DebugOverlay entity tree
```

### Finding a Menu in Code

If a menu item does not work as expected, trace it:
1. Find the `ImGui::MenuItem` / `ImGui::Checkbox` / `ImGui::SliderFloat` in
   `FlatBuffersScenario::OnGUI()` (`source/scene/FlatBuffersScenario.cpp`).
2. Note which member variable it writes to.
3. Find where that member is read — usually in the system's `OnUpdate()`.

---

## 12.5 InputService

**File:** `include/services/InputService.h`

`InputService` handles all GLFW input events and exposes them to systems and scripts.

### Registration

GLFW callbacks are registered once in `EngineOrchestrator::initWindow()`:

```cpp
glfwSetKeyCallback(window, EngineOrchestrator::keyCallback);
glfwSetCursorPosCallback(window, EngineOrchestrator::mouseCallback);
```

The static callbacks forward to `InputService`:

```cpp
void EngineOrchestrator::keyCallback(GLFWwindow* w, int key, int scan, int action, int mods) {
    auto* self = static_cast<EngineOrchestrator*>(glfwGetWindowUserPointer(w));
    self->inputManager->handleKeyEvent(key, scan, action, mods);
}
```

### Two Input Modes

| Mode | Method | When to Use |
|------|--------|-------------|
| **Polled (continuous)** | `IsKeyDown(GLFW_KEY_W)` | Movement, held actions |
| **Event (single press)** | `handleKeyEvent()` stores edge-state flags | One-shot toggles |

`IsKeyDown` calls `glfwGetKey(window, key)` every frame — it returns `GLFW_PRESS` while
the key is held. This is the correct way to implement movement.

For detecting a single keypress (e.g., Tab to cycle cameras), `handleKeyEvent` stores whether
the key was pressed this frame and not the last, implementing "edge detection":

```cpp
// In InputService (conceptual — actual implementation uses flags):
void handleKeyEvent(int key, int scan, int action, int mods) {
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        activeCameraIndex = (activeCameraIndex + 1) % CAM_TOTAL;
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        resetRequested = true;
    }
}
```

### Using InputService from a Script

```cpp
// In a GameScriptComponent subclass:
void Update(float dt) override {
    auto* input = ServiceLocator::GetInputService();
    auto* rb    = ServiceLocator::GetEntityManager()->TryGetTIComponent<RigidBody>(m_entityID);
    if (!rb) return;

    const float force = 10.0f;
    if (input->IsKeyDown(GLFW_KEY_W)) rb->forceAccum.z -= force;
    if (input->IsKeyDown(GLFW_KEY_S)) rb->forceAccum.z += force;
    if (input->IsKeyDown(GLFW_KEY_A)) rb->forceAccum.x -= force;
    if (input->IsKeyDown(GLFW_KEY_D)) rb->forceAccum.x += force;
    if (input->IsKeyDown(GLFW_KEY_SPACE)) rb->forceAccum.y += force * 3.0f;
}
```

---

## 12.6 Camera System

**File:** `include/services/Camera.h`, `include/services/InputService.h`

The camera is not an ECS entity — it is a standalone object owned by `InputService`. This
reflects the fact that cameras don't participate in physics or scripting; they are purely a
rendering concern.

### Camera Presets

`InputService` maintains three camera instances:

| Index | Label | Description |
|-------|-------|-------------|
| 0 | Front | Third-person perspective, follows scene |
| 1 | Bird's Eye | Top-down overhead view |
| 2 | Orbit | Rotates around the scene centre automatically |

**Tab** cycles between them. The active camera is returned via `getActiveCamera()` and its
view/projection matrices are uploaded to the UBO each frame.

### Mouse Look

When right-click is held, the cursor is captured. Mouse movement updates the active camera's
yaw and pitch:

```cpp
// In InputService::handleMouseEvent():
if (cursorCaptured) {
    float dx = xpos - lastX;
    float dy = lastY - ypos;  // inverted: Y grows down in screen space
    lastX = xpos;
    lastY = ypos;
    getActiveCamera()->processMouseMovement(dx, dy);
}
```

`Camera::processMouseMovement` applies `sensitivity * dx/dy` to the yaw and pitch Euler angles,
clamps pitch to ±89°, and recomputes the view direction vector.

---

## 12.7 Logger

**File:** `include/core/Logger.h`, `source/core/Logger.cpp`

The Logger provides severity-filtered output to the console (and optionally to a file).

### API

```cpp
GE::Logger::Log(GE::Logger::Level::INFO,  "Scenario loaded: " + path);
GE::Logger::Log(GE::Logger::Level::WARN,  "Entity count near limit: " + std::to_string(count));
GE::Logger::Log(GE::Logger::Level::ERROR, "VkResult != VK_SUCCESS in " + funcName);
GE::Logger::Log(GE::Logger::Level::FATAL, "Cannot recover — aborting");
```

### Severity Levels

| Level | Meaning | Behaviour |
|-------|---------|-----------|
| `DEBUG` | Low-level trace info | Compiled out in Release |
| `INFO` | Normal operation events | Printed to stdout |
| `WARN` | Unexpected but recoverable | Printed with `[WARN]` prefix |
| `ERROR` | Serious issue, still running | Printed with `[ERROR]` prefix |
| `FATAL` | Unrecoverable error | Printed, then calls `std::abort()` |

### Convenience Macros

The Logger header defines macros for common patterns:

```cpp
GE_LOG_INFO("Physics system initialized at " + std::to_string(hz) + " Hz");
GE_LOG_WARN("Cloth burn source count exceeds recommended limit");
GE_LOG_ERROR("Failed to create VkDescriptorSet for entity " + std::to_string(id));
```

### Where to Look for Output

- **Release mode:** Output to the console window. If running from VS, see the Output panel.
- **Debug mode:** Output to the console AND VS's Debug Output window (via `OutputDebugString`).
- **Vulkan validation:** Also printed to the console via the debug messenger callback registered
  in `initVulkan()`.

---

## 12.8 PerformanceTracker

**File:** `include/services/PerformanceTracker.h`

`PerformanceTracker` measures frame time, FPS, and per-system timing. It is populated each
frame and rendered via the **Debug** menu in the ImGui overlay.

### What It Tracks

| Metric | Description |
|--------|-------------|
| `fps` | Frames per second (1 / frameTime) |
| `frameTimeMs` | Total frame time in milliseconds |
| `physicsTickMs` | Time spent in the last physics tick |
| `renderMs` | Time from command buffer start to vkQueueSubmit |

### Adding a New Timing Point

To time a new code section:

```cpp
// In any system OnUpdate():
auto start = std::chrono::high_resolution_clock::now();

// ... your work ...

auto end  = std::chrono::high_resolution_clock::now();
float ms  = std::chrono::duration<float, std::milli>(end - start).count();

// Report to tracker:
ServiceLocator::GetPerformanceTracker()->setCustomTiming("MySystem", ms);
```

Then add a line to the Debug menu in `FlatBuffersScenario::OnGUI()`:
```cpp
ImGui::Text("MySystem: %.2f ms", stats->getCustomTiming("MySystem"));
```

---

## 12.9 ColliderVisualizerSystem

**File:** `include/systems/ColliderVisualizerSystem.h`

`ColliderVisualizerSystem` is a debug rendering system that draws wireframe outlines of
physics colliders and cloth debug geometry.

### How It Works

1. Every physics tick, the system iterates all collider components (SphereCollider, BoxCollider,
   CapsuleCollider, CylinderCollider) and writes line segment pairs into a pre-allocated
   host-visible line vertex buffer.
2. A dedicated `collider_wire` pipeline (pipeline index 7 in the Phong pipeline table) renders
   these lines in a single draw call.
3. The cloth system contributes its own debug lines (spring lines, particle crosses, normal
   vectors) into the same buffer pool — up to 50,000 line vertices.

### Toggling

The Debug menu in the ImGui overlay has a **Collider Wireframes** checkbox that sets
`ColliderVisualizerSystem::m_enabled`. When false, the system still runs but clears the buffer
and skips the draw call. This avoids recreating GPU resources when toggling.

### Color Legend for Cloth Debug Lines

| Color | Meaning |
|-------|---------|
| White | Structural spring (intact) |
| Cyan | Shear spring (intact) |
| Yellow | Flexion spring (intact) |
| Red | Torn spring (stress > threshold) |
| Orange | Hot spring (heat > 0.3) |
| Green | Surface normal vector at particle |
| Rainbow gradient | Particle cross — color indicates heat level |

---

## 12.10 Common ImGui and Debug Mistakes

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Menu item changes nothing | Writing to a local copy instead of the system member | Check the field name — it must write to the system's `m_` member, not a copy |
| `ImGui::SliderFloat` range wrong | Min/max swapped | Verify `SliderFloat(label, &val, min, max)` order |
| ImGui rendering on top of missing geometry | `draw()` called before 3D passes | Ensure `uiManager->draw(cb)` is the **last** call before `vkEndRenderPass` |
| ImGui text invisible | Style color is white-on-white | Call `ImGui::StyleColorsDark()` in `DebugOverlay::init()` |
| Validation error on `imguiPool` | Pool not large enough for ImGui font atlas | Increase `maxSets` and `descriptorCount` in pool creation |
| Collider wireframes invisible | System not registered for the active scenario | `ColliderVisualizerSystem` is registered as an engine-global system — check it was not accidentally unregistered |
| Logger output truncated | FATAL triggered `abort()` before more output | Check last message before `[FATAL]` for the root cause |

---

*Return to: [README — Technical Manual Index](README.md)*
