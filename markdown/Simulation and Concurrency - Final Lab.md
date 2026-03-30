# 700105 Simulation and Concurrency - Final Lab

## MSc Computer Science for Games Programming

**Course**: 700105 - Simulation and Concurrency

**Name**: JOSE JAVIER SERRANO SOLIS

**Instructors:** Simon Grey & Warren Viant 

---

## Introduction

The purpose of this assessment is to design and implement an efficient network-based simulation of a series of physics-based scenes that will be loaded from flat buffers. Any missing values in the flat buffer should be given an appropriate default value.

## Concept: Networked Simulator

You are required to create a distributed "simulation" for FOUR users. In each scene, objects will be assigned to one of the four users, based on ownership information stored within the flat buffer. In addition, the list of features is a guide to the building blocks and functionality expected of the solution. Where appropriate, you should include User Interface elements to demonstrate functionality. Interactions with the user interface are either local or global. Local user interface means that the effects of that user interface are only applied to the local version of the simulation. Global user interface means that the effects of that user interface can be changed by any user and should be applied to all networked users.

For example, your simulation should be capable of displaying objects in the scene that are visually distinct according to the material the object is made of, or the user who owns the object. For example, objects are to be colour-coded according to the user - Red, Green, Blue and Yellow. Switching between displaying materials and assigned users is an example of a ***Local User Interface***. **Any user can change it, and it should only change the local display for that user**.

You should also be able to swap between named scenes without restarting the application. This is an example of a ***Global User Interface***. **Any user can change it, and it should change for all users**.

Your simulation is to make efficient use of the compute capacity within the PCs in RBB-335.

## Level 1: Core Features

### Core Simulation Features (~30%)

These features, when properly implemented, will provide approximately 30% of the final mark.

### Scenes

Scenes should be loaded from a flat buffer with a schema as described throughout this specification. In the schema, units are metres, kilograms, seconds and degrees. Angles are expressed using Euler angles to make authoring and understanding scenes more intuitive. That does not mean that your software should use Euler angles internally.

Scenes consist of a unique name to enable scene selection at runtime. A description of what happens in the scene. A boolean value to specify if gravity should initially be active, a number of camera views, objects and spawners that create objects, and information about interactions between pairs of materials. You should provide a global user interface control to swap between scenes. 

```
table Scene {
 name: string;
 description: string;
 gravity_on: bool = true;
 cameras: [Camera];
 objects: [Object];
 spawners: [SpawnerType];
 interactions: [MaterialInteraction];
}
```

Various objects in the scene will require transformation and orientation information. This will be provided using a Transform as described below.

```
struct Vec3 {
  x: float;
  y: float;
  z: float;
}

struct RotationEuler {
  yaw: float;   // rotation around Y axis (degrees)
  pitch: float; // rotation around X axis (degrees)
  roll: float;  // rotation around Z axis (degrees)
}

struct Transform {
  position: Vec3;
  orientation: RotationEuler;
  scale: Vec3;
}
```

### Cameras

A scene can have one or more cameras. Cameras should be named so that you can switch between them, and you should provide a local user interface control to swap between camera views. You should also be able to control cameras using the mouse and keyboard to change the local view. Cameras can give an orthographic or perspective view depending on their type. The flat buffer schema for cameras is as follows: 

```
union CameraType {
 PerspectiveCamera,
 OrthographicCamera
}

table Camera {
 name: string;
 transform: Transform;
 camera_type: CameraType;
}

table PerspectiveCamera {
 fov: float;
 near: float;
 far: float;
}

table OrthographicCamera {
 size: float;
 near: float;
 far: float;
}
```

### Simulated Physics Objects

Physics objects have a name so that they can be identified and edited via a global user interface. If the name is missing, you should assign an appropriate unique alternative, such as "object 1". Physics objects have a transform to describe their position and orientation in the world. Physics objects have a material, which is used to determine things like the coefficient of restitution or friction constants, which are dependent on pairs of materials. The Physics objects references the name of the material as a string. Physics objects also have a shape, which can be a Sphere, Capsule, Cylinder, Plane or Cuboid. Shapes can either be solid or, more rarely, containers for other shapes. For example, if you were to create a scene that simulates several spheres moving inside a cylinder, the spheres would be solid, and the cylinder would be a container. Containers should be rendered appropriately so that the user can see the inside surface of the container.

Physics objects are described as follows. 

```
enum CollisionType : byte {
  SOLID,        // normal object (outside is empty, inside is solid)
  CONTAINER     // inverted (inside is empty, outside is solid)
}

table Object {
  name: string;
  transform: Transform;
  material: string;
  shape: Shape;
  behaviour: Behaviour;
  collision_type: CollisionType = SOLID;
}
```

#### Materials

Materials have a name and a density, which is used to calculate the mass of each object. The name is used to describe constants between pairs of materials in the material interactions table.

```
table Material {
  name: string;
  density: float;
}
```

Material interactions, which are referenced in the Scene table, store details about the coefficient of restitution, the static and dynamic friction you should use between pairs of objects. The material_a and material_b strings refer to the name of a Material. 

```
table MaterialInteraction {
  material_a: string;
  material_b: string;
  restitution: float;
  static_friction: float;
  dynamic_friction: float;
}
```

#### Shapes

There are five types of shapes that you should include. Sphere, Cuboid, Cylinder, Capsule and Plane. In all cases, the position in the transform of the Object represents the centre of the shape. 

```
union Shape {
 Sphere,
 Cuboid,
 Cylinder,
 Capsule,
 Plane
}

table Sphere { radius: float; }
table Cube { size: Vec3; }
table Cylinder { radius: float; height: float; } // aligned to y axis in local space, rotated by object transform
table Capsule { radius: float; height: float; } // aligned to y axis in local space, rotated by object transform
table Plane { normal: Vec3; } // local-space normal, rotated by object transform
```

#### Behaviours

There are three categories of behaviour that physical objects can have. They are static, animated or simulated.

```
union Behaviour {
  StaticObject,
  AnimatedObject,
  SimulatedObject
}
```

##### Static Behaviour

Static behaviour objects are the simplest type of objects.

Each user owns their own copy of a static object.

```
table StaticObject {
}
```

##### Simulated Behaviour

Simulated behaviour objects are the most important object type in the simulation. These objects model physical behaviour. They have a linear and angular velocity. Simulated objects are supported by a PhysicsState struct that stores the initial linear and angular velocities of the object.

```
enum ObjectOwnerType : byte { ONE, TWO, THREE, FOUR }

struct PhysicsState {
  linear_velocity: Vec3; // metres/second
  angular_velocity: Vec3; // degrees/second
}

table SimulatedObject {
  initial_state: PhysicsState;
  owner: ObjectOwnerType;
}
```

Simulated objects should be models as full rigid bodies - with different shapes, sizes, masses, and linear and angular velocity, momentum and inertia.

It is suggested that during development, you move from simpler to more complex cases - starting from treating objects as particles and raising complexity as you introduce new features. The majority of the marks available are for the simulation of objects. Do NOT spend time creating animated objects before being able to simulate objects that can interact with the animated objects!

Ownership of simulated objects is described by the ObjectOwnerType. The owner of a simulated object is responsible for simulating that object and calculating its collision response. (see concurrent section)

##### Animated Behaviour

Animated behaviour objects follow a set path of two or more waypoints, a total duration time for the animation, an easing type and a path mode. The path mode specifies what the object should do at the end of the path. The total duration is significant depending on the path mode. Each waypoint includes a position, an orientation and a time specifying the position and orientation that the object should be at and the total time from the start that the object should take to reach that waypoint from the start. For example, if the waypoints A, B and C have times 0, 3 and 5 then the object should start at A, then after 3 seconds the object should have travelled from A to B, then the object should take 2 seconds to travel from B to C. If the path mode is STOP then the object should stop at the end of the path - effectively becoming a static object (A -> B -> C -> Stop). If the path mode is REVERSE, then when the object has reached all the waypoints, it should traverse the same path backwards (A -> B-> C -> B -> A etc.). For both STOP and REVERSE path modes the total_duration should be the same as the time it takes to reach the last waypoint (5 in the previous example). The last path mode is LOOP. This means that the object should return to the first waypoint. In this case, the total_duration should be used to calculate the time the object should take to travel from the last waypoint to the first. So in the previous example, if the total duration was 9, then after 3 seconds the object should be at B, 2 seconds later the object should be at C, then the object should take 4 seconds to travel from C to A and the process starts again.

The easing type indicates whether the object should move from one point to another using simple linear interpolation or smoothstep interpolation.

```
enum EasingType : byte { LINEAR, SMOOTHSTEP }
enum PathMode : byte { STOP, LOOP, REVERSE }

table Waypoint {
  position: Vec3;
  rotation: RotationEuler;
  time: float; // absolute time to reach this waypoint
}

table AnimatedObject {
  waypoints: [Waypoint];
  total_duration: float;
  easing: EasingType;
  path_mode: PathMode;
}
```

Each user owns their own copy of an animated object.

Note - When a simulated object collides with an animated object, the collision response for the simulated object should be treated as colliding with a moving object, transferring momentum from the moving object to the simulated object, however the path and velocity of the animated object should not be adjusted. This means that in this type of collision, momentum will NOT be conserved.

### Object Spawners

An object spawner is a system that creates new objects over time, with some randomness in where and how they appear. When a scene uses the term many to indicate the number of objects, it implies that you decide the quantity your application can include and still be able to run at a reasonable framerate. For example many in a simple simulation might be as high as 10k - 100k. Spawners are positioned in the scene and should add objects to the scene.  There are two types of spawner, SingleBurstSpawn and RepeatingSpawn. SingleBurstSpawn will spawn a specified number of objects at a particular time. RepeatingSpawn will spawn objects constantly at a set interval up to a maximum number of objects.

```
table RepeatingSpawn {
  interval: float;
  max_count: uint;
}

table SingleBurstSpawn {
  count: uint;
}

union SpawnType {
  SingleBurstSpawn,
  RepeatingSpawn
}
```

For simplicity, spawners are separated by the object type that they spawn. Spawners are supported by two structs that represent a range of scalar and vector values that are used to help create objects with random data - such as velocity.

```
struct FloatRange {
  min: float;
  max: float;
}

struct Vec3Range {
  min: Vec3;
  max: Vec3;
}

enum SpawnerOwnerType : byte { ONE, TWO, THREE, FOUR, SEQUENTIAL } 
```

All spawners have a base type. The BaseSpawner has a name string, a start time when the spawner should become active - a SpawnType (SingleBurstSpawn or RepeatingSpawn), a SpawnLocation, a range of initial linear and angular velocities to be assigned to spawned objects, and the name of the material the spawned objects should have. Objects spawned from spawners always Simulated Objects, and always have collision_type: CollisionType = SOLID;

```
table BaseSpawner {
  name: string;
  start_time: float = 0; // time (seconds) when spawning starts
  spawn_type: SpawnType;
  location: SpawnLocation;
  linear_velocity: Vec3Range; // initial linear velocity range (metres/second)
  angular_velocity: Vec3Range; // initial angular velocity range (degrees/second)
  material: string; // material name for spawned objects
  owner: SpawnerOwnerType;
}
```

Ownership of a spawner is described by the SpawnerOwnerType. The owner of a spawner is responsible for creating objects of the appropriate type, setting the ownership of each object, and distributing each object across the network. (see concurrent section)

The SpawnLocation represents the range of locations that objects can be spawned in to. This represents the range of values that could be assigned to the position of the centre of an object. It can be a FixedLocation, RandomBox or RandomSphere.

```
union SpawnLocation {
  FixedLocation,
  RandomBox,
  RandomSphere
}

table FixedLocation {
  transform: Transform;
}

table RandomBox {
  min: Vec3;
  max: Vec3;
}

table RandomSphere {
  center: Vec3;
  radius: float;
}
```

The SpawnerType is used to determine the shape of the object that is being spawned, and provide ranges of values for the data members of those specific shapes.

```
union SpawnerType {
  SphereSpawner,
  CylinderSpawner,
  CapsuleSpawner,
  CuboidSpawner
}

table SphereSpawner {
  base: BaseSpawner;
  radius_range: FloatRange;
}

table CylinderSpawner {
  base: BaseSpawner;
  radius_range: FloatRange;
  height_range: FloatRange;
}

table CapsuleSpawner {
  base: BaseSpawner;
  radius_range: FloatRange;
  height_range: FloatRange;
}

table CuboidSpawner {
  base: BaseSpawner;
  size_range: Vec3Range;
}
```

The final scene.fbs file is provided here [Scene.fbs](../flatbuffers/Scene.fbs) Download Scene.fbs 

Some example scenes will be provided. You are encouraged to author your own scenes to best demonstrate the functionality of your solutions.

### Core Concurrency Features (~30%)

#### Distributed Architecture

The application is to be designed as a distributed system. Each node (peer) within the system will have its own dedicated graphics, dedicated physics and a common state. The rendered images displayed on each peer must be identical.

A peer-to-peer network infrastructure is required with a minimum of two peers.

#### Distributed Ownership

Each object within the scene has an owner, regardless of whether it was loaded from the flat buffer or created by a spawner.

Static and animated objects are each owned by all peers, so they are effectively local.  

Simulated objects are owned according to their definition, if loaded from the flat buffer, or are assigned by the spawner upon creation.  Spawners either assign ownership to a specific peer or assign ownership "sequentially", whereupon they assign ownership in rotation ie. peer 1, peer 2, peer 3, peer 4, peer 1, etc.

The owner of a simulated object is responsible for both its simulation and collision response.  The dynamic state of a simulated object is transferred to all other peers every frame.

A client-server architecture must not be implemented, and will be penalised.

#### Networking

Either TCP or UDP network protocols can be used to transfer data between peers.

The minimum requirement is two peers on separate physical PCs in RBB-335.

Marks will be awarded for ease of network configuration.

#### Parallel Architecture

The major components within each application are required to operate asynchronously. The choice of threading architecture is not defined.

As part of the final demonstration, you will be required to run various parts of your system at different frequencies. For example, the graphics may be requested to be run at 30 frames per second (Hz), whilst the simulation is run at 1000 Hz. This should be controlled via ImGui

This ability of individual components to run asynchronously is a major part of this assessment. Failure to implement this correctly will greatly reduce your marks.

#### Process Affinity

To ease debugging and to simplify the assessment process, you will be required to use a very specific thread-to-processor mapping.

Core Application

- Visualisation - core 1
- Networking - core 2 - 3
- Simulation - core 4+

You can use any number of threads in your application, provided you have sufficient to meet the processor mapping in the table above. For example, it is acceptable to use 4 networking threads on the server, provided that they are all located on core 2 and 3.

## Level 2: Advanced Features 

### Advanced Simulation Features (~10%)

Choose one advanced simulation feature to add to your simulation. You should conduct your own independent research to implement advanced features. You should include appropriate debugging visualisation and UI controls to be able to change values at runtime, and reliably demonstrate how your implementation works. You should extend the flat buffer schema to include your chosen advanced feature. 

#### Cloth Simulation

Simulate a cloth object using the spring and dashpot model, including damping, shear springs and flexion springs. Include a dynamic bounding object and collision detection with all other objects.

#### Compound Rigid Bodies

Add the ability to create objects made up of other rigid bodies. Include a model to adjust the centre of mass of an object, and consider how to deal with conservation of angular momentum. Include bounding object and collision detection with all other objects.

#### Hinged Objects

Add the ability to create hinged objects that are joined together with constraints on transforms and/or orientation. Include a dynamic bounding object and collision detection with all other objects.

#### Flocking and Steering

Create an implementation of flocking and steering with cohesion, alignment and separation forces in addition to collision avoidance forces, combined using techniques such as weighted truncated sum, or prioritised dithering. 

Reynolds, C.W., 1987, August. Flocks, herds and schools: A distributed behavioural model. In *Proceedings of the 14th annual conference on Computer graphics and interactive techniques* (pp. 25-34).

Buckland, M., 2005. *Programming game AI by example*. Jones & Bartlett Learning. Chapter 3

### Advanced Concurrency Features (~10%)

#### Distributed Architecture

Three or more peers can be implemented.

#### Data integrity

The application should ensure that the data stored in each peer is as accurate as possible. With the Simulation being performed on each peer, the data will inevitably begin to drift. The application should seek to address any inconsistencies. Correction algorithms will be required e.g. interpolation.

The process of correcting the data will itself create more communications, as the corrections are distributed to other peers.

Marks will be awarded for the smoothness of the correction process, from the user's perspective.

#### Networking

A network simulation tool will be inserted between the peers to simulate network latency and packet loss. Your application must be able to cope with the following worst-case network quality of service:

Latency: 100ms ± 50ms

Packet loss: 20%

## Level 3 : Extended Advanced Features

### Extended Simulation Features (~10%)

To extend advanced features, you may have to do your own deeper research into various topic areas.

#### Extended Cloth Simulation

Extend your cloth simulation to include basic wind, tearing and burning of your cloth.

#### Extended Compound Rigid Bodies

Extend compound rigid bodies to include fracturing one object into several objects when placed under sufficient stress/strain.

#### Extended Hinged Objects

Extend your hinged objects to create ragdoll objects. 

#### Extended Flocking and Steering

Implement at least two different spatial segmentation techniques (e.g. Uniform Grid, Octree) to speed up flocking calculations. Include meaningful comparisons of performance between the two spatial segmentation techniques and using no spatial segmentation techniques, considering the memory used, processing of collision detection and processing of physics updates.

### Extended Concurrency Features (~10%)

#### Compute shaders

Elements of the simulation can be implemented on the GPU using Compute Shaders.

## Implementation

The software MUST be demonstrated on the PCs in RBB-335.

### Graphics

Graphics should be implemented using Vulkan with an ImGui user interface.

### Simulation

You are to implement all of the physics. You may use the example code that has been provided to you in the lab work.

Only glm math library is permitted.

### Networking

Only the Winsock 2 library is permitted.

### Threads

Either the Win32 or C++ 23 threading library is permitted.

The PC's in RBB-335 are considered the target platform. Threading is to be used to leverage the performance of these processors.

## Report

You should include a report and reflection of your work in Markdown in your GitHub repository. The structure of the report is as follows:

- System architecture, including where threads and networking have been used (1000 words max), plus UML diagrams
- How the motion physics has been implemented for the balls, and how the collision detection and response have been implemented between the balls and the other elements in the scene (1000 words max)

Marks will be lost if the word limit is exceeded.

## Source code

Source code should be submitted via GitHub

## Video

A narrated video showing each implemented feature should be submitted via Canvas.

## Mark scheme

A detailed mark scheme will be provided.
