# 700105_A25_T2: Simulation and Concurrency

---

## Simulation Lab 3
In this lab you will build upon your sandbox application, using  your simulation library to achieve some basic simulation functionality.

So far in your physics engine you have created Collider classes that allow you to tell if one object is colliding with another. This can be used for all sorts of things in games, such as a trigger to trigger to open a door when a character enters an area. For this module we want to use the colliders as part of a simulation. To do that we will create a PhysicsObject class. The PhysicsObject class will store data concerning the position of the object, the rotation and be used to calculate motion using physics (or you can use what your project already uses, like an entity component system with transform components or RigidBody components). It will have a collider object to help determine what forces are being applied to the object.

![See attach SimulationLab3_Image3.png. Simple UML Diagram of how a Concrete scenario uses the physics objects and colliders](../markdown-resources/Simulation-Lab/3/SimulationLab3_Image1.png)

At this point you may want to keep the position vector as part of the Collider class, or you may want to move the position into the PhysicsObject class and provide the collider with a reference to the current position, or a reference to the PhysicsObject, you may want to have a position in both where the Collider class's position is relative to the PhysicsObject it belongs to, or you may want to create a separate object that manages the frame of reference of anything in your application.

---

### Q1 Reflect upon the implications of how you manage position (Summative - due in lab on or before 26/02/26)
Where could you store the position of your physics objects and what implications would that have on the rest of your system?
- What options did you consider?
- What were the advantages and disadvantages of each?
- What was your final decision and why?

---

### Q2  Be able to move a ball through space (Summative - due in lab on or before 26/02/26)
You should render your scene 60 times a second. How you update your scene depends on your windowing framework. Some frameworks expose a message loop, whilst others provide the ability to add a callback when certain events trigger, and others provide methods that can be overridden that are called at specific times. Whatever windowing system you have you should create an update method that updates your simulation. Your update method should take a float parameter called seconds that tells the method how many seconds you should update your simulation by. As highlighted previously, you should be able to adjust the size of your timestep and set up your code so that your simulation can run several times inbetween render calls. For example if you render at 60 frames a second (0.016 seconds pass between frames) and you want to run you simulation four times between render calls your timestep will be 0.016 / 4, or 0.004 seconds.

![See attach SimulationLab3_Image2.png. Simple sequence diagram to represent the lifecycle of the physics simulations](../markdown-resources/Simulation-Lab/3/SimulationLab3_Image2.png)

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

---

### Q3  Be able to make a sphere fall under the effect of gravity (Summative - due in lab on or before 26/02/26)
As part of your physics loop you should accumulate forces acting on each physics object.

You should store the mass of your objects in your PhysicsObject class or Rigidbody component. It is also useful to store 1/mass (inverseMass) as this will be used every frame to calculate the Force due to gravity. You can assume that acceleration due to gravity is contant, and is 9.81 ms-2 in the negative y direction. Remember, eventually you will be accumulating forces over a frame so you can apply them all at the same time.

![See attach SimulationLab3_Image3.png. Demostration of mass storation and forces bwing applied](../markdown-resources/Simulation-Lab/3/SimulationLab3_Image3.png)

You should be able to do the following things:

- Change the size of the fixed timestep
- Choose between at least two integration methods - for example
    - Euler
    - Semi-Implicit Euler
    - RK4
    - Another method based on your own research
- Develop test cases to ensure that with specific starting conditions your sphere is in the correct place after a specific period of time
    - Again, you can test against the implicit formula `s = ut + (0.5)(at²)` however this time as an acceleration is being applied your integration methods will result in a small error


---

### Q4  Be able to detect a collision between spheres and planes (Summative - due in lab on or before 26/02/26
Next add collisions to your system. You should use the colliders in your physics objects to determine if two objects have collided. If you detect the collision you should set the velocity of your object to zero. 

You should be able to do the following things:

- Collide a moving sphere against a stationary sphere
- Collide a moving sphere against a stationary plane
    - Ensure to create scenarios with different planes including planes that are not axis aligned 
-Develop test cases to ensure that with specific starting conditions your sphere is in the correct place after a specific period of time

**Bonus Features**
Instead of setting the object velocity to zero try reflecting the object velocity with respect to the collision normal.