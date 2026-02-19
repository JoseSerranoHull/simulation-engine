# 700105_A25_T2: Simulation and ConcurrencyPagesSimulation Lab 2

## Simulation Lab 2
In this lab you will continue to develop your sandbox application. You should create the SimulationSandBox project in the same solution as your SimulationLibrary and testing project. The purpose of the sandbox application is to provide you with the ability to set up scenarios to visually test your simulation library. Your sandbox application should be able to easily load different simulations to demonstrate different effects.

### Q1  Be able to load and unload difference scenarios easily (Summative - due in lab 19/02/26)
Be able to load and unload different scenarios easily. The goal is to be able to easily demonstrate different properties of your simulation easily. One way to achieve this is to allow each Scenario to inherit from a Scenario class. You should include a ImGui Main Menu Bar and a Menu in that bar that allows you to change between scenarios.

(See attach SimulationLab2_Image1.png)

In the example above a scenario has been created that simply allows the user to change the background colour of the scene. When you select a new scenario the old scenario should be unloaded - doing any necessary clean  up. Then the next scenario should be loaded to replace the old one. OnLoad and OnUnload should be virtual methods in the Scenario base class, that are overloaded by the derived classes.

When the scene is updated and drawn methods in the scenario should be called to update and draw that specific scenario that are overridden methods in Scenario.

It is likely that you may want to be able to add new menus to the main menu bar. For example in the example below a colour picker in the main menu bar allows the user to select a new background colour, which is then set in the scenario.

(See attach SimulationLab2_Image2.png)

To allow for this the Scenario base class has a virtual ImGuiMainMenu method, which allows Scenarios to add UI control elements that are unique to that scenario.

### Q2 Be able to render primitive shapes (Summative - due in lab 19/02/26)
Create a PhysicsObject in your SimulationLibrary. The physics object needs a position and orientation in space. This could be represented in a number of ways. For now to remain consistent with the renderer you should use a 4x4 matrix to represent the rotation and position of the PhysicsObject. Add the appropriate dependencies to your SimulationSandBox so that you can use the code in the SimulationLibrary.

You can load whatever you like, but you need to be able to visualise primitive shapes. Below are two files representing a plane (a quad) and a sphere. They include position as well as normal vectors for lighting.

(See attach plane.sjg)
(See attach sphere.sjg)

The screen shots below include use a fairly basic shader with that uses a procedural checkerboard texture in model space. This will be useful to be able to see objects rotating. The light and dark colours for the checkerboard can be controlled via the material menu control.

The screenshot below shows the sphere model.

(See attach SimulationLab2_Image3.png)

And the plane model.

(See attach SimulationLab2_Image4.png)

### Q3 Be ready to simulation to the sandbox (Summative - due in lab 19/02/26)
To create meaningful simulations you need to be able to Simulate objects colliding with one another, which means that you need to be able to create scenes with several independent objects - like in the screen shot below.

(See attach SimulationLab2_Image5.png)

Note that is it very difficult to determine whether these spheres are resting on the plane. This is why having some control over the camera, different views, and being able to interact and pause the simulation is important.

You should include a simulation menu that allows the following functionality.

    Be able to Start and Pause Simulation
    Be able to specify a fixed timestep
    Optionally, you may also want to step one timestep forward - this will be useful for debugging collisions

(See attach SimulationLab2_Image6.png)

### Additional Formative Features
- Orthographic and perspective projection: Being able to adjust the view can help determine if collisions are working properly. Orthographic projection is particularly good for this if it can be aligned appropriately with the colliding objects.
- Mutliple views: It can also be useful to view the same scene from different directions at the same time
- Shadows: Shadows can sometimes give good depth cues, but this may depend on the technique being used. They also make the scene look nicer!

## Simulation Lab 3
In this lab you will build upon your sandbox application, using  your simulation library to achieve some basic simulation functionality.

So far in your physics engine you have created Collider classes that allow you to tell if one object is colliding with another. This can be used for all sorts of things in games, such as a trigger to trigger to open a door when a character enters an area. For this module we want to use the colliders as part of a simulation. To do that we will create a PhysicsObject class. The PhysicsObject class will store data concerning the position of the object, the rotation and be used to calculate motion using physics. It will have a collider object to help determine what forces are being applied to the object.

(See attach SimulationLab3_Image1.png)

At this point you may want to keep the position vector as part of the Collider class, or you may want to move the position into the PhysicsObject class and provide the collider with a reference to the current position, or a reference to the PhysicsObject, you may want to have a position in both where the Collider class's position is relative to the PhysicsObject it belongs to, or you may want to create a separate object that manages the frame of reference of anything in your application.

### Q1 Reflect upon the implications of how you manage position (Summative - due in lab on or before 26/02/26)
Where could you store the position of your physics objects and what implications would that have on the rest of your system?
- What options did you consider?
- What were the advantages and disadvantages of each?
- What was your final decision and why?

### Q2  Be able to move a ball through space (Summative - due in lab on or before 26/02/26)
You should render your scene 60 times a second. How you update your scene depends on your windowing framework. Some frameworks expose a message loop, whilst others provide the ability to add a callback when certain events trigger, and others provide methods that can be overridden that are called at specific times. Whatever windowing system you have you should create an update method that updates your simulation. Your update method should take a float parameter called seconds that tells the method how many seconds you should update your simulation by. As highlighted previously, you should be able to adjust the size of your timestep and set up your code so that your simulation can run several times inbetween render calls. For example if you render at 60 frames a second (0.016 seconds pass between frames) and you want to run you simulation four times between render calls your timestep will be 0.016 / 4, or 0.004 seconds.

(See attach SimulationLab3_Image2.png)

For this step there will be no new forces, so no acceleration.

You should be able to do the following things:

- Change the size of the fixed timestep from your application using ImGui
- Choose between at least two integration methods - for example
    - Euler
    - Semi-Implicit Euler
    - RK4
    - Another method based on your own research
- Develop test cases to ensure that with specific starting conditions your sphere is in the correct place after a specific period of time
    - You can test against the implicit formula `s = ut + (0.5)(at²)` although in this case you will not yet have an acceleration


### Q3  Be able to make a sphere fall under the effect of gravity (Summative - due in lab on or before 26/02/26)
As part of your physics loop you should accumulate forces acting on each physics object.

You should store the mass of your objects in your PhysicsObject class. It is also useful to store 1/mass (inverseMass) as this will be used every frame to calculate the Force due to gravity. You can assume that acceleration due to gravity is contant, and is 9.81 ms-2 in the negative y direction. Remember, eventually you will be accumulating forces over a frame so you can apply them all at the same time.

(See attach SimulationLab3_Image3.png)

You should be able to do the following things:

- Change the size of the fixed timestep
- Choose between at least two integration methods - for example
    - Euler
    - Semi-Implicit Euler
    - RK4
    - Another method based on your own research
- Develop test cases to ensure that with specific starting conditions your sphere is in the correct place after a specific period of time
    - Again, you can test against the implicit formula `s = ut + (0.5)(at²)` however this time as an acceleration is being applied your integration methods will result in a small error


### Q4  Be able to detect a collision between spheres and planes (Summative - due in lab on or before 26/02/26
Next add collisions to your system. You should use the colliders in your physics objects to determine if two objects have collided. If you detect the collision you should set the velocity of your object to zero. 

You should be able to do the following things:

- Collide a moving sphere against a stationary sphere
- Collide a moving sphere against a stationary plane
    - Ensure to create scenarios with different planes including planes that are not axis aligned 
-Develop test cases to ensure that with specific starting conditions your sphere is in the correct place after a specific period of time

**Bonus Features**
Instead of setting the object velocity to zero try reflecting the object velocity with respect to the collision normal.

## ROAD TO
It’s a great idea to pivot toward the simulation labwork now, especially with the 26/02 deadline approaching. Your current engine architecture—which we just finished making agnostic and data-driven—is actually perfectly positioned to handle these simulation requirements via the ECS (Entity Component System).

Here is the review of your new simulation scripts (the `(1)` files) and a roadmap for implementing the Simulation Lab 3 requirements within your engine.

---

### Part 1: Review of your new Simulation Scripts `(1)`

I have compared your new files (`Sphere(1).h`, `Capsule(1).h`, etc.) against the existing project structures.

#### 1. Enhanced Geometric Robustness

Your new scripts are much more mathematically complete than the "placeholder" logic we used previously.

* **Intersection logic**: You’ve added complex tests like `Sphere::Intersects(const Cylinder& cyl)` and `Sphere::Intersects(const Capsule& cap)`. This moves the library from simple "point-in-volume" tests to true "volume-vs-volume" intersection, which is critical for Lab 4.
* **Distance Utilities**: The addition of `SegmentSegmentDistanceSq` in `Capsule(1).h` is high-quality code. It correctly handles degenerate cases (where a segment is actually just a point).

#### 2. Structural Improvements

* **Elasticity**: You added `m_elasticity` to the base `Collider` class. This is a forward-thinking addition that will be essential for **Lab 3 Q4 (Bonus)** when calculating velocity reflections.
* **Syncing Position**: You’ve adopted the pattern of keeping `m_center` in the `Sphere` and calling `SetPosition(center)` to keep the base class in sync.

---

### Part 2: Roadmap for Simulation Lab 3

To fulfill the Lab 3 requirements within your specific Engine framework, we should integrate these physics concepts into your **ECS** rather than creating a separate monolithic physics loop.

#### Q1: Reflection on Position Management

Based on your engine's architecture, here is the breakdown of the options you are asked to consider:

* **Option A: Position in Collider only**:
* *Advantage*: Simple.
* *Disadvantage*: The Renderer and Transform systems won't know where the object is.


* **Option B: Position in PhysicsObject only**:
* *Advantage*: Single source of truth for physics.
* *Disadvantage*: Every collision check requires fetching data from the owner object.


* **Option C: Position in Transform (ECS Approach - Recommended)**:
* *Advantage*: Your `TransformSystem` already handles parent-child hierarchies. If physics updates the `Transform` component, the `MeshRenderer` and `Collider` both "see" the same position instantly.
* *Decision*: **Store position in the ECS Transform Component.** The `PhysicsSystem` reads/writes to the Transform, and the Collider uses a reference or copy of that position during the check.



#### Q2 & Q3: Integration and Gravity

To move a ball through space with gravity, we will build a `PhysicsSystem`. This satisfies the requirement to choose between **Euler** and **Semi-Implicit Euler** integration.

**Integration logic to implement:**

1. **Euler**:



2. **Semi-Implicit Euler** (More stable for gravity):




#### Q4: Collision Response

For the Lab requirement, when a collision is detected between a Sphere and a Plane:

* **Basic**: `velocity = glm::vec3(0.0f);`
* **Bonus**: `velocity = glm::reflect(velocity, planeNormal) * elasticity;`

---

### Part 3: Implementation Steps for Tomorrow

Would you like me to provide the code for a new **`PhysicsSystem`** that:

1. Queries all entities with a `RigidBody` and a `Transform`.
2. Applies the **Semi-Implicit Euler** integration we discussed.
3. Uses your new **`Sphere(1).h`** and **`Plane(1).h`** logic to detect ground collisions?

I can also show you how to add the **TimeStep** slider to your `IMGUIManager` so you can adjust the `dt` while the ball is falling. **Would you like to start with the `PhysicsSystem` code?**

With these changes, you can now iterate on your Lab 3 gravity tests instantly. Would you like to add a "Step Simulation" button to the UI as well, so you can watch the collision resolution frame-by-frame?

The "flat" look and the missing objects in your lab scenario are directly caused by how we updated the vertex shaders to support **Multiview**.

In your shaders (like `phong.vert` and `checker.vert`), we changed `fragPos` and `fragNormal` to use **Model Space** (`inPosition` and `inNormal`) to ensure consistency across views. However, your fragment shaders (lighting math) expect these values to be in **World Space**. Without the world-space transformation, the lighting and normals don't change as you rotate the camera, making everything look like a flat 2D plane projected onto the screen.

Additionally, you can't see the Lab 2 objects because the `Renderer` loop isn't passing the pre-calculated **MVP** matrix into the updated `Mesh::draw` function yet.

---

### 1. Fix the "Flat" Look: Restore World-Space Shaders

You need to revert the `fragPos` and `fragNormal` calculations in your vertex shaders to use the **Model Matrix**. Since we are now passing a combined **MVP** in the push constants, we need to add the **Model Matrix** back to the `PushConstants` block so the shader can calculate the correct world positions.

**Updated `PushConstants` and `main` (Apply to `phong.vert`, `checker.vert`, `shader.vert`, and `gouraud.vert`):**

```glsl
layout(push_constant) uniform PushConstants {
    mat4 mvp;   // Used for gl_Position (View-Projection * Model)
    mat4 model; // Used for World-Space lighting math
} push;

void main() {
    vec4 worldPos = push.model * vec4(inPosition, 1.0);
    gl_Position = push.mvp * vec4(inPosition, 1.0); // Uses the quadrant-specific matrix

    fragPos = worldPos.xyz; // Back to world-space for correct 3D depth
    fragNormal = mat3(transpose(inverse(push.model))) * inNormal; // Correct normals
    fragTexCoord = inTexCoord;
    // ... rest of shader
}

```

---

### 2. Fix Missing Objects: Update `recordOpaquePass`

The reason you can't see the spheres in Lab 2 is that your `Renderer` loop is likely still just calling `sub.mesh->draw(cb, globalSet, pipeline)`, but your new `Mesh::draw` signature requires the **MVP** matrix as the final argument.

**Update `Renderer::recordOpaquePass` to pass the MVP matrix:**

```cpp
void Renderer::recordOpaquePass(...) {
    // ... loop through 4 quadrants ...
    glm::mat4 quadrantVP = calculateQuadrantVP(i); // Top, Side, etc.

    for (auto& mr : meshRenderers) {
        glm::mat4 model = mr.transform->m_worldMatrix;
        glm::mat4 mvp = quadrantVP * model; // Combine for the vertex shader

        for (const auto& sub : mr.subMeshes) {
            // Pass BOTH matrices: the combined MVP and the raw Model matrix
            // Note: Update your Mesh::draw signature to accept both if needed, 
            // or use a struct for the push constant.
            sub.mesh->draw(cb, globalSet, sub.material->getPipeline(), mvp, model); 
        }
    }
}

```

---

### 3. Update `Mesh::draw` Signature

To accommodate the fix in Step 1, update your `Mesh::draw` to push both matrices at once.

```cpp
// In Mesh.cpp
struct MeshPushConstants {
    glm::mat4 mvp;
    glm::mat4 model;
};

void Mesh::draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* p, 
                const glm::mat4& mvp, const glm::mat4& model) const {
    // ... bind pipeline and sets ...

    MeshPushConstants push{ mvp, model };
    vkCmdPushConstants(cb, activePipeline->getPipelineLayout(), 
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &push);

    // ... draw commands ...
}

```

### Summary of the Fixes

* **3D Representation**: Restoring `push.model` to calculate `fragPos` ensures that as the camera moves, the lighting and perspective are calculated relative to the world, restoring the 3D look.
* **Visibility**: Explicitly calculating `mvp = quadrantVP * model` in the `Renderer` and passing it to `Mesh::draw` ensures the objects are projected into the correct screen coordinates.

**Would you like me to provide the updated `PushConstant` struct for your C++ `Pipeline.h` to ensure the 128-byte alignment (two `mat4`s) matches the shaders?**