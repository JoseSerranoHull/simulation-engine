# 700105_A25_T2: Simulation and Concurrency

---

## Simulation Lab 7
In this lab you will build upon your physics engine library to add spring like forces, and use those to connect together other objects.

---

### Q1  Add a Spring (Formative)
Add appropriate tests to your testing framework to ensure that your spring like force generates the correct force under various conditions using Hooke's law F=-kx where x = L - Lr - modifying the spring constant k and the resting length L, and the current length Lr

Values you might like to test include a checking there is no force generated what there is no extension or compression, and testing different lengths of spring, and different spring constants.

Once you have added appropriate tests add a spring like force generator to your to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Add a spring like force generator using Hooke's law F=-kx where x = L - Lr
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project. Use the spring to connect a sphere to an imaginary point in a scenario. What happens to the movement with different values of k? What happens when you apply a temporary force to the sphere? ***(Formative element)***

---

### Q2 Add dampeners to your springs (Formative)
Without dampening simulations that use spring will be in motion forever, and because of errors in integration are susceptible to becoming unstable. Add a dampening coefficient to your spring (previous tests should still pass if the dampening coefficient is set to 0).

- Add a damping force to you springs F=−kx−bv where v is the relative velocity of the two spheres
- What happens with different values for k and b?
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative element)***

---

### Q3 Add a rope scenario (Formative)
Create a line of masses joined by springs to create something that behaves like a rope.

You should be able to demonstrate the following things:

- Create a lineof masses joined by springs to create something that behaves like a rope.
- Include flexion springs - note the difference as you modify the values of those springs.

---

### Q4 Add a cloth scenario (Formative)

Create a grid of masses joined by springs to create something that behaves like a cloth.

You should be able to demonstrate the following things:

- Create a grid of masses joined by springs to create something that behaves like a cloth.
- Include flexion springs - note the difference as you modify the values of those springs.
- Include shear springs - note the difference as you modify the values for those springs.
- If your cloth is not stable you might consider tracking the amount of energy in the system and dynamically adapting dampening coefficients if energy increases significantly.
