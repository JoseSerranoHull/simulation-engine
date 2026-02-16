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

# EXTRAS ANSWERS
Would you like me to provide the finalized C++ code for handleRigidBody, handleSphereCollider, and handlePlaneCollider?
Check Experience couase it needs to load the scenarios there or something.

Particle Systems: Delete dustParticleSystem, fireParticleSystem, etc., from Experience.h.

Weather Logic: Delete syncWeatherToggles() and performFullReset(). These are specific to the Climate simulation.

Specific Light Member: Delete std::unique_ptr<PointLight> mainLight;. We now query this via the ECS.

SceneKeys Namespace: Delete the SceneKeys namespace at the top of Experience.cpp. Hardcoded string lookups are the enemy of an agnostic engine

The Plan: Particle-to-ECS Migration
To achieve this, we need to transition from "Object-Oriented" ownership to "Data-Oriented" rendering. Here is the step-by-step plan:

1. Define the ParticleComponent
We create a component that stores the GPU state (buffers, pipelines) for a particle effect.

Change: Add ParticleComponent to PhysicsComponents.h or a new ParticleComponents.h.

Data: This component will hold the VkBuffer for the particles and the specific compute/graphics pipelines.

2. Rework the Renderer
Instead of taking 5 specific pointers (Fire, Dust, etc.), the Renderer will now perform a "System Query."

Change: Modify Renderer::recordFrame to accept a reference to the EntityManager.

Logic: It iterates through all entities with a ParticleComponent and records their draw calls.

3. Agnostic Particle Updating
The ParticleSystem logic (compute shader dispatch) should move into an ECS System (e.g., ParticleUpdateSystem).

Change: Create ParticleUpdateSystem inheriting from IECSystem.

Logic: Every frame, this system finds all particle entities and dispatches the compute shaders to update their positions on the GPU.

Would you like me to show you the code for the new ParticleComponent and how the Renderer queries it? This is the final step to making your rendering pipeline 100% agnostic.

With the Renderer and Experience both revamped and agnostic, you are officially ready to run the Sanity Test. Would you like me to walk you through the ParticleComponent definition next so we can fill in that recordParticles logic?