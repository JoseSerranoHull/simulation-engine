# 700105_A25_T2: Simulation and Concurrency

---

## Simulation Lab 5
In this lab you will build upon your physics engine library to achieve some functionality for manipulating the orientation of objects.

To begin with you will need to add some method to represent the orientation of your physics object. To begin with it is probably easiest to use a 3x3 matrix to represent orientation.

---

### Q1  Add orientation and angular displacement (Summative - due in lab 19/03/26)
Add appropriate tests to your testing framework to apply an angular displacement to your physics objects. Values you might like to test include 90 ( pi/2 radians), 180 (pi radians), 270 ( 3 pi / 2 radians) and 360 (2 pi radians) degree rotations in x, y and z - and some combination of both.  Later it will be important that angles are expressed as radians, but here degrees are used to aid comprehension. You should create a table in your markdown files associated with the module to record your tests.

Once you have added appropriate tests add the functionality to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Using your PhysicsEngine project be able to apply an angular displacement (a rotation) to a physics object.  
- Verify that your rotations work as expected in your testing project.
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative [but fun] element)***

---

### Q2 Add angular velocity (Summative - due in lab 19/03/26)
Add appropriate tests to your testing framework to apply an angular velocity to your physics object. Generate test data to simulate an angular velocity with a fixed timestep for a specific period of time, then check the values of the cardinal axis are correct. You should create a table in your markdown files associated with the module to record your tests.

Values you might like to test include 90 ( pi/2 radians), 180 (pi radians), 270 ( 3 pi / 2 radians) and 360 (2 pi radians) degree rotations per second in x, y and z - and some combination of both. The run your simulation for 1, 2, 3 and 4 seconds (integrating over timesteps) and check that your orientation is correct.

Once you have added appropriate tests add the functionality to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Using your PhysicsEngine project be able to apply an angular velocity (a rotation in radians per second) to a physics object to make it rotate over time.  
- Verify that your angular velocity implementation works as expected in your testing project.
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative [but fun] element)***

---

### Q3 Reflect on the Different Approaches on Storing Orientation (Summative - due in lab 19/03/26)
How have you stored your orientation? What other options could you choose? What are the advantages and disadvantages of each approach?