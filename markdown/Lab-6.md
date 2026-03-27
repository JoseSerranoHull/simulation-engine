# 700105 Simulation and Concurrency Lab Book

## Simulation Lab 6

**26/03/2026**

---

In this lab you will build upon your physics engine library to achieve some functionality for manipulating the angular velocity of objects using torque.

---

### Q1  Add accumulating torque and angular acceleration (Summative - due in lab 26/03/26)
Add appropriate tests to your testing framework to apply a torque to a physics objects. Remember torque is equal to the component of force applied at a point that is perpendicular to the centre of mass multiplied by the distance to that mass. Your torque can be represented as a 3d vector with magnitude equal to the size of the torque, pointing in the direction of the axis of rotation according to the right hand rule.

 Values you might like to test include a force that only has a component that is perpendicular to the centre of mass as different distances from the centre of mass, a force that only has a parallel component (and so should produce no torque).

Once you have added appropriate tests add the functionality to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Using your PhysicsEngine project be able to apply torque producing force to a physics object.
- Verify that your force results in the appropriate torque, angular rotation and angular velocity as expected in your testing project.
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative element)***

### Solution

Torque τ = r × F, where r is the position vector from the centre of mass to the point of force application and F is the applied force. The cross product encodes both the magnitude of the torque (|r||F|sinθ, where θ is the angle between r and F) and the axis of rotation via the right-hand rule. A force applied parallel to r (θ = 0°) produces zero torque; one applied perpendicular (θ = 90°) produces maximum torque for that force and distance.

In the engine, torque is accumulated per-frame into `torqueAccum` on the `RigidBody` component, applied as angular acceleration α = I⁻¹·τ during integration, then cleared. A companion `constantTorque` field is re-injected into `torqueAccum` at the start of every `Integrate()` call, allowing data-driven spin-up demos from `.ini` files without per-frame scripting. At the Q1 stage, `invInertiaTensor` is identity (I₃), so α = τ directly; Q2 replaces this with the sphere inertia tensor.

**Snippet 1 — RigidBody torque fields** (`include/components/PhysicsComponents.h`):
```cpp
// Accumulated torque τ for the current frame (N·m, world space).
// Cleared to zero after each Integrate() call.
glm::vec3 torqueAccum     { 0.0f };

// Lab 6: persistent per-frame torque (N·m, world space).
// Re-injected into torqueAccum every Integrate() step before clearing.
// Drives spin-up demos via .ini without needing per-frame scripting.
// Zero vector = disabled.
glm::vec3 constantTorque{ 0.0f };
```

**Snippet 2 — PhysicsSystem angular integration** (`source/systems/PhysicsSystem.cpp`):
```cpp
// --- 4. Angular integration ---
// Re-inject constant per-frame torque (Lab 6: drives spin-up demos via ini).
rb.torqueAccum += rb.constantTorque;

// α = I_world⁻¹ · τ_world
const glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;

// dR/dt = Skew(ω) · R  →  R_new = R + dt · Skew(ω) · R
if (glm::dot(rb.angularVelocity, rb.angularVelocity) > 1e-12f) {
    rb.orientation = rb.orientation + dt * SkewSymmetric(rb.angularVelocity) * rb.orientation;
    Orthogonalise(rb.orientation);
}

// Clear torque accumulator for next frame
rb.torqueAccum = glm::vec3(0.0f);
```

**Snippet 3 — SceneLoader ConstantTorque parsing** (`source/scene/SceneLoader.cpp`, inside `handleRigidBody`):
```cpp
// Lab 6: constant per-frame torque for scenario demos (N·m, world space)
if (props.count("ConstantTorque"))
    rb.constantTorque = parseVec3(props.at("ConstantTorque"));
```

**Snippet 4 — Example ini entry** (`config/simulation_lab6_q1_torque.ini`):
```ini
; Sphere_Base: m=1, r=1, τ_y=3 N·m → I=0.4, α = 3/0.4 = 7.5 rad/s²
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 0 3 0
[SphereCollider]
Radius = 1
```

The sandbox scenario `simulation_lab6_q1_torque.ini` defines four floating spheres with no gravity. `Ref_NoTorque` has no `ConstantTorque` and stays stationary — the baseline. `Sphere_Base` (m=1, r=1) spins up at 7.5 rad/s² about Y. Pressing "Reset Angular Velocities" in the Lab 6 ImGui panel zeroes all ω, letting you watch each body spin up from rest again.

**Test data:**

All tests use identity `invInertiaTensor` (I₃ — no collider inertia set, Q1 stage only). Torque τ applied as `constantTorque` every frame. After t = 1 s at dt = 0.001 s (1000 steps), ω = α × t = τ × t. Tolerance ε = 1×10⁻⁴.

| Test ID | Force F | Point r from CoM | τ = r×F | invInertiaTensor | α = τ | ω after t=1 s |
|---------|---------|-------------------|---------|-----------------|-------|----------------|
| Tx_perp_d1 | (10, 0, 0) | (0, 0, 1) | (0, 10, 0) | I₃ | (0, 10, 0) | (0, 10, 0) |
| Tx_perp_d2 | (10, 0, 0) | (0, 0, 2) | (0, 20, 0) | I₃ | (0, 20, 0) | (0, 20, 0) |
| Ty_perp | (10, 0, 0) | (0, 1, 0) | (0, 0, -10) | I₃ | (0, 0, -10) | (0, 0, -10) |
| Tz_parallel | (0, 0, 10) | (0, 0, 1) | (0, 0, 0) | I₃ | (0, 0, 0) | (0, 0, 0) |
| Multi_axis | (0, 0, 5) | (1, 0, 0) | (0, -5, 0) | I₃ | (0, -5, 0) | (0, -5, 0) |
| Spinup_3Nm | ConstantTorque = (0,3,0) | — | (0, 3, 0) | I₃ | (0, 3, 0) | (0, 3, 0) |

`Tz_parallel` (r and F both along Z) confirms τ = 0 because sinθ = sin(0°) = 0 — the force cannot produce rotation around a perpendicular axis when it points toward the centre of mass. `Tx_perp_d2` has twice the lever arm of `Tx_perp_d1` and produces exactly twice the torque, confirming the linear distance relationship.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - τ = r × F encodes both the magnitude (|r||F|sinθ) and the axis of rotation in a single vector. The zero-torque case for a parallel force (θ = 0°, sinθ = 0) falls out naturally from the cross product formula without needing a special branch — the algebra handles it automatically.

- *Did you make any mistakes?*
    - Initially `torqueAccum` was not cleared at the end of `Integrate()`, causing torque to accumulate unboundedly across frames. The fix was to clear it to zero after the angular acceleration step and introduce `constantTorque` as the persistent per-frame drive, keeping the accumulator pattern clean.

- *In what way has your knowledge improved?*
    - Understanding the accumulator pattern — summing all torques for the frame, applying them once to produce acceleration, then clearing — decouples the *source* of a torque (constant drive, impulse, collision response) from the integration logic. The same pattern was already in use for linear forces (`forceAccum`), so adding torque was architecturally consistent.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q2 Add inertia for a sphere (Summative - due in lab 26/03/26)
Inertia can be thought of as the resistance of an object to change it's angular momentum.

I = 2/5 mr2

Using this modify your physics object calculation for angular velocity such that 𝜏 = ⅆ𝐿/ⅆ𝑡 and 𝐿 = 𝐼𝜔

Repeat (or rewrite - the tests will now have different outcomes for angular velocity) your previous tests to include inertia. Verify that objects that have more mass require more torque to achieve the same angular velocity.

Once you have added appropriate tests add the functionality to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Using your PhysicsEngine project be able to apply torque producing force to a physics object.
- Verify that your force results in the appropriate torque, angular rotation and angular velocity as expected in your testing project.
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative element)***

### Solution

The moment of inertia of a solid sphere is I = (2/5)mr². Its inverse is I⁻¹ = 5/(2mr²), which scales the identity matrix since a sphere has equal inertia on all axes (isotropic). This is computed once at load time when a `SphereCollider` is encountered and stored in `invInertiaTensor` on the sibling `RigidBody`. The angular acceleration formula α = I⁻¹·τ then becomes α = (5/(2mr²))·τ, so heavier or larger spheres accelerate more slowly under the same torque.

The same `ConstantTorque` mechanism from Q1 drives the demos. The Q1 scenario `simulation_lab6_q1_torque.ini` serves double duty: it uses `SphereCollider` components so the correct inertia is already in effect, making it the Q2 verification scenario as well.

**Snippet 1 — invInertiaTensor field** (`include/components/PhysicsComponents.h`):
```cpp
// Inverse inertia tensor I⁻¹.
// For a uniform solid sphere: I = (2/5)·m·r²·Identity
//                            I⁻¹ = (5/(2·m·r²))·Identity
// Computed at load time by SceneLoader when a SphereCollider is present.
glm::mat3 invInertiaTensor{ glm::mat3(1.0f) };
```

**Snippet 2 — handleSphereCollider inertia computation** (`source/scene/SceneLoader.cpp`):
```cpp
// Solid-sphere formula: I = (2/5)·m·r²·Identity  =>  I⁻¹ = (5/(2·m·r²))·Identity
if (auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(m_currentEntity)) {
    if (!rb->isStatic && rb->mass > 0.0f && sc.radius > 0.0f) {
        const float I = (2.0f / 5.0f) * rb->mass * sc.radius * sc.radius;
        rb->invInertiaTensor = glm::mat3(1.0f / I);
    }
}
```

**Snippet 3 — Angular acceleration using invInertiaTensorWorld** (`source/systems/PhysicsSystem.cpp`):
```cpp
// Q5: refresh world-space inverse inertia tensor each frame.
// I_world⁻¹ = R · I_body⁻¹ · R^T
rb.invInertiaTensorWorld = rb.orientation * rb.invInertiaTensor * glm::transpose(rb.orientation);

// α = I_world⁻¹ · τ_world
const glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;
```

**Snippet 4 — ini entries for all four sphere variants** (`config/simulation_lab6_q1_torque.ini`):
```ini
; Sphere_Base: m=1, r=1 → I=0.4, α_y = 7.5 rad/s²
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 0 3 0
[SphereCollider]
Radius = 1

; Sphere_HeavyMass: m=4, r=1 → I=1.6, α_y = 1.875 rad/s²  (4× mass → ¼ α)
[RigidBody]
Mass = 4
UseGravity = false
ConstantTorque = 0 3 0
[SphereCollider]
Radius = 1

; Sphere_BigRadius: m=1, r=2 → I=1.6, α_y = 1.875 rad/s²  (2× radius → 4× I → ¼ α)
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 0 3 0
[SphereCollider]
Radius = 2
```

The key observable is that `Sphere_HeavyMass` (4× mass) and `Sphere_BigRadius` (2× radius) spin up at exactly the same rate (α = 1.875 rad/s²) despite different causes — both produce I = 1.6 N·m·s², which is ¼ of the reference sphere's α = 7.5 rad/s². `Ref_NoTorque` (no `ConstantTorque` key) remains stationary throughout.

**Test data:**

τ = (0, 3, 0) N·m constant torque applied each frame. α_y = 3 / ((2/5)·m·r²). After t = 1 s: ω_y = α_y × 1. Tolerance ε = 1×10⁻⁴.

| Test ID | Mass m | Radius r | I = (2/5)mr² | I⁻¹ (scalar) | α_y (rad/s²) | ω_y at t=1 s |
|---------|--------|----------|--------------|--------------|--------------|--------------|
| Ref_NoTorque | 1 | 1 | 0.4 | 2.5 | 0 (τ=0) | 0 |
| Sphere_Base | 1 | 1 | 0.4 | 2.5 | 7.5 | 7.5 |
| Sphere_4xMass | 4 | 1 | 1.6 | 0.625 | 1.875 | 1.875 |
| Sphere_2xRadius | 1 | 2 | 1.6 | 0.625 | 1.875 | 1.875 |
| Sphere_2xMass_2xRadius | 2 | 2 | 3.2 | 0.3125 | 0.9375 | 0.9375 |

`Sphere_4xMass` and `Sphere_2xRadius` have identical α (1.875 rad/s²) because both arrive at I = 1.6 — one via linear mass scaling, the other via quadratic radius scaling.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The sphere inertia formula I = (2/5)mr² shows that radius has a larger effect than mass: doubling the radius increases I by 4×, while doubling the mass increases I by only 2×. This is because rotational inertia depends on how far mass is distributed from the axis — even on a solid sphere, the outer layers contribute disproportionately.

- *Did you make any mistakes?*
    - None. The isotropic scalar approach worked cleanly because the sphere formula gives I⁻¹ = scalar × I₃ — the tensor is a scaled identity, so the direction of the torque vector is preserved exactly. There was no need for a full 3×3 matrix computation at this stage.

- *In what way has your knowledge improved?*
    - Separating `invInertiaTensor` (computed once at load time from the collider shape) from `invInertiaTensorWorld` (refreshed every frame from orientation) clarifies the two distinct roles. The body-space tensor never changes for a rigid body; only its projection into world space does.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q3 Add inertia tensor for a cylinder (Summative - due in lab 26/03/26)
In the case of a solid sphere the rotational symmetry means that we can treat inertia with a single value. More generally the moment of inertia will be different along different axis.

Modify your solution to include an inertia tensor with it's axis aligned with the z direction. For a cylinder the inertia tensor is as follows:

![Cylinder Inertia Tensor Formulae](../markdown-resources/Simulation-Lab/6/SimulationLab6_Image1.png)

Remember to account for the local orientation of the cylinder - initially you should do this by using the orientation matrix to convert your torque into inertial (object) space.

Create tests to check that your program works as expected. Initially use objects with the identity matrix for their orientation. Then create more complex scenarios, like using known rotations (like 90 degrees around each cardinal axis) to compare results. Once you have added appropriate tests add the functionality to your physics engine to pass the tests. When the tests pass add a scenario to your sandbox so that you can observe the simulation.

You should be able to demonstrate the following things:

- Using your PhysicsEngine project be able to apply torque producing force to a physics object.
- Verify that your force results in the appropriate torque, angular rotation and angular velocity (calculated using the change in angular momentum using the inertia tensor) as expected in your testing project.
- Document your approach, your tests and your reflection in markdown.
- Demonstrate this functionality by adding appropriate scenarios to your sandbox project ***(Formative element)***

### Solution

A solid cylinder has two distinct moments of inertia. The spin axis (local Y in this engine, along the cylinder's central axis) has the lowest inertia Iy = ½mr², because all mass is within radius r of that axis. The two transverse axes (local X and Z) have higher inertia Ixz = (1/12)m(3r²+h²), because mass at the top and bottom of the cylinder is far from those axes. This non-isotropic tensor is stored as a diagonal `glm::mat3` on `RigidBody::invInertiaTensor`.

For an upright cylinder (identity orientation), torque along local Y = world Y yields fast spin (α = τ/Iy); torque along local X or Z yields slow spin (α = τ/Ixz). For a tilted cylinder, the world-space torque must be correctly mapped through the orientation before applying the tensor. This is done via the world-space tensor `invInertiaTensorWorld = R · I⁻¹_body · R^T` (see Q5), but the initial object-space approach — converting torque to body space: `τ_body = R^T · τ_world`, computing `α_body = I⁻¹_body · τ_body`, then converting back `α_world = R · α_body` — is algebraically equivalent.

**Snippet 1 — handleCylinderCollider inertia computation** (`source/scene/SceneLoader.cpp`):
```cpp
// Cylinder spin axis = local Y (matches the procedural mesh generator).
// Iy  = (1/2)·m·r²              (spin axis — low inertia, fast rotation)
// Ixz = (1/12)·m·(3r² + h²)    (transverse axes — higher inertia, slow rotation)
const float r   = cc.radius;
const float h   = cc.height;
const float Iy  = 0.5f           * rb->mass * r * r;
const float Ixz = (1.0f / 12.0f) * rb->mass * (3.0f * r * r + h * h);
rb->invInertiaTensor = glm::mat3(
    1.0f / Ixz, 0.0f,      0.0f,
    0.0f,       1.0f / Iy, 0.0f,
    0.0f,       0.0f,      1.0f / Ixz
);
```

**Snippet 2 — invInertiaTensorWorld refresh and angular acceleration** (`source/systems/PhysicsSystem.cpp`):
```cpp
// I_world⁻¹ = R · I_body⁻¹ · R^T  (congruence transform — maps body-space tensor to world)
rb.invInertiaTensorWorld = rb.orientation * rb.invInertiaTensor * glm::transpose(rb.orientation);

// α_world = I_world⁻¹ · τ_world
const glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;
```

**Snippet 3 — ini entries for spin-axis vs transverse-axis comparison** (`config/simulation_lab6_q3_cylinder.ini`):
```ini
; Cyl_SpinAxis: τ along spin axis Y → α_y = 5 / 0.125 = 40 rad/s²
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 0 5 0
[CylinderCollider]
Radius = 0.5
Height = 2

; Cyl_TransAxis: τ along transverse X → α_x = 5 / 0.396 ≈ 12.6 rad/s²
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 5 0 0
[CylinderCollider]
Radius = 0.5
Height = 2

; Cyl_Tilted45: pre-rotated 45° around X; world-Y torque decomposes across both body axes
[RigidBody]
Mass = 1
UseGravity = false
InitialRotation = 45 0 0
ConstantTorque = 0 5 0
[CylinderCollider]
Radius = 0.5
Height = 2
```

**Snippet 4 — Worked inertia values** (r=0.5, h=2.0, m=1):

Iy  = ½ × 1 × 0.5² = **0.125 N·m·s²** → I⁻¹_y = 8.0 → α_y = 5 × 8.0 = **40.0 rad/s²**

Ixz = (1/12) × 1 × (3 × 0.25 + 4.0) = (1/12) × 4.75 = **0.3958 N·m·s²** → I⁻¹_xz = 2.527 → α_x = 5 × 2.527 = **12.6 rad/s²**

`Cyl_SpinAxis` spins **~3.2× faster** than `Cyl_TransAxis` under the same 5 N·m torque — the principal observable for Q3. `Cyl_Tilted45` demonstrates correct orientation handling: the world-Y torque decomposes into both body-Y (spin, fast) and body-Z (transverse, slow) components, producing a combined precession.

**Test data:**

Cylinder: r=0.5, h=2.0, m=1. Iy = 0.125 N·m·s², Ixz = 0.3958 N·m·s². Identity orientation unless noted. α = I_axis⁻¹ × τ_axis. Tolerance ε = 1×10⁻⁴.

| Test ID | Orientation | Torque τ | Active axis inertia | I (N·m·s²) | I⁻¹ | α (rad/s²) |
|---------|-------------|----------|---------------------|-----------|-----|------------|
| Cyl_SpinY_fast | identity | (0, 5, 0) | Iy = ½mr² | 0.125 | 8.0 | 40.0 |
| Cyl_TransX_slow | identity | (5, 0, 0) | Ixz | 0.3958 | 2.527 | 12.63 |
| Cyl_TransZ_slow | identity | (0, 0, 5) | Ixz | 0.3958 | 2.527 | 12.63 |
| Cyl_SpinY_τ10 | identity | (0, 10, 0) | Iy | 0.125 | 8.0 | 80.0 |
| Cyl_Tilt90X_transverse | Rx(90°) | (0, 5, 0) | Ixz (local Y now = world Z) | 0.3958 | 2.527 | 12.63 |
| Cyl_Tilt90X_spinNow | Rx(90°) | (0, 0, 5) | Iy (local Z now = world -Y) | 0.125 | 8.0 | 40.0 |

`Cyl_SpinY_fast` spins ~3.17× faster than `Cyl_TransX_slow` with the same torque magnitude. After Rx(90°), the spin axis (local Y) now points along world Z — so world-Z torque gives the fast spin response and world-Y torque gives the slow transverse response; the roles are swapped.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The inertia tensor is diagonal for axis-aligned bodies because there is no cross-coupling between the principal axes. Off-diagonal terms would arise only if the mass distribution were asymmetric relative to the chosen axes. For a cylinder, the rotational symmetry about its length axis guarantees Ixy = Ixz = 0, making the body-space tensor purely diagonal with two unique values.

- *Did you make any mistakes?*
    - Initially the world-space torque was applied directly to the body-space tensor, producing correct results only when the orientation was identity. For `Cyl_Tilted45`, the angular acceleration direction was visually wrong. The fix was computing `invInertiaTensorWorld = R · I⁻¹_body · R^T` each frame, which correctly routes any world-space torque through the body's principal axes regardless of current orientation.

- *In what way has your knowledge improved?*
    - Understanding that "converting torque to object space, applying the tensor, converting back" (`α_world = R · I⁻¹_body · R^T · τ_world`) is algebraically identical to pre-computing the world-space tensor made the Q5 optimisation obvious — it is the same computation, just done once and reused.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q4 Reflect on the Impact of Adding Inertia when Simulating Many Objects (Summative - due in lab 26/03/26)
What is the impact of performance over a single frame. Which calculations are quick and which are slow? What results are being calculated very often?

### Solution

For each body in the simulation, the angular dynamics section of `Integrate()` performs the following operations every frame:

**1. World-space inertia tensor refresh** — `invInertiaTensorWorld = R · I⁻¹_body · R^T`

This requires two 3×3 matrix multiplications. Each mat3×mat3 multiply costs 27 multiplications and 18 additions, so the two multiplications together cost ~54 multiplications and ~36 additions. This is the most expensive operation in the angular integration path and is executed unconditionally for every body every frame.

**2. Angular acceleration** — `α = invInertiaTensorWorld × τ`

A mat3×vec3 multiply: 9 multiplications + 6 additions. Cheap compared to the tensor refresh.

**3. Orientation integration** — `R_new = R + dt · Skew(ω) · R`

Two mat3 operations (Skew construction + multiply). Executed only when `|ω|² > 1e-12` (stationary bodies skip this).

**4. Re-orthogonalisation** — `Orthogonalise(R)` (Gram-Schmidt)

3 normalizations, 2 dot products, 1 cross product. Also conditional on `|ω|² > 1e-12`.

**What scales badly with many bodies:**

The world inertia tensor refresh (step 1) is computed unconditionally for every body every frame, including those that are stationary or have no torque. With N bodies, this is O(N) mat3 multiplications — expensive constant factor even though the complexity class is linear. For isotropic bodies (spheres), `R · (s×I₃) · R^T = s × I₃` is always identity-equivalent and the multiply is wasted.

`Orthogonalise()` applies Gram-Schmidt every tick even for bodies with zero angular velocity, adding unnecessary work.

**Optimisations possible:**

- Introduce a **sleeping threshold**: if `|ω|² < ε` and `|τ|² < ε`, skip steps 1–4 entirely for that body.
- For **isotropic tensors** (sphere colliders, scalar × I₃), skip the tensor refresh since `I_world = I_body`.
- If the fixed timestep is constant, pre-multiply `dt × invInertiaTensorWorld` once per body per frame rather than recomputing `angularAccel * dt` separately.

**Test data:**

N/A — Q4 is a reflective analysis of computational cost, not a numerical physics test.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The dominant cost is the world-space tensor refresh: two full mat3 multiplications per body per frame. With thousands of rigid bodies this dominates the physics budget, yet it is executed unconditionally — even for bodies that are resting or have no angular motion. Sleeping/islands logic (skipping dormant bodies) is the standard engine response to this.

- *Did you make any mistakes?*
    - None.

- *In what way has your knowledge improved?*
    - Separating "computed once at load" (body-space tensor, determined by shape) from "computed every frame" (world-space tensor, dependent on orientation) makes the cost profile clear. The body-space tensor is immutable for a rigid body — it only needs to be recomputed if the shape itself changes, which never happens in a rigid-body simulation.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q5 Inverse Inertia Tensor in World Coordinates (Formative)
Once you are convinced that your calculations are correct you can optimize your engine by storing the inverse inertia tensor in your physics object. Once that is working you can also store the inverse inertia tensor in world coordinates and use that to apply your torque (again in world coordinates - instead of converting your torques to object space).

### Solution

The world-space inverse inertia tensor is computed via the congruence transform: `I_world⁻¹ = R · I_body⁻¹ · R^T`, where R is the orientation matrix. This matrix rotates the body's principal axes (stored column-by-column in R) into world space, applies the per-axis inverse inertia scalings, then rotates back. Once cached as `invInertiaTensorWorld` on the `RigidBody`, torque can be applied directly in world space without any manual coordinate conversion: `α_world = invInertiaTensorWorld · τ_world`.

The tensor is refreshed at the start of the angular integration step each frame — before the torque is applied — so it always reflects the body's current orientation. For isotropic bodies (spheres), the refresh has no effect (the tensor remains a scaled identity) but is still executed for simplicity.

**Snippet 1 — invInertiaTensorWorld field** (`include/components/PhysicsComponents.h`):
```cpp
// Lab 6 Q5: world-space inverse inertia tensor, refreshed each frame.
// Caches  R · I_body⁻¹ · R^T  so torque can be applied directly in world space.
// Correct for non-isotropic bodies (cylinders, cuboids) unlike the body-space tensor.
glm::mat3 invInertiaTensorWorld{ glm::mat3(1.0f) };
```

**Snippet 2 — Per-frame refresh in PhysicsSystem** (`source/systems/PhysicsSystem.cpp`):
```cpp
// Q5: refresh world-space inverse inertia tensor each frame.
// I_world⁻¹ = R · I_body⁻¹ · R^T
rb.invInertiaTensorWorld = rb.orientation * rb.invInertiaTensor * glm::transpose(rb.orientation);
```

**Snippet 3 — World-space angular acceleration** (`source/systems/PhysicsSystem.cpp`):
```cpp
// α_world = I_world⁻¹ · τ_world  (no coordinate conversion needed)
const glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;
```

**Snippet 4 — ini entries showing orientation-dependent behaviour** (`config/simulation_lab6_q5_worldspace.ini`):
```ini
; Same τ = (0, 5, 0) for all three cylinders — only InitialRotation differs.
; Cyl_Upright: identity → world Y = spin axis → fast (α_y ≈ 40 rad/s²)
; Cyl_Tilt45:  45° about X → world Y decomposes across Iy and Ixz → intermediate
; Cyl_Tilt90:  90° about X → world Y = transverse axis → slow (α_y ≈ 12.6 rad/s²)
[RigidBody]
Mass = 1
UseGravity = false
InitialRotation = 90 0 0       ; change this per entity (0, 45, or 90)
ConstantTorque = 0 5 0
[CylinderCollider]
Radius = 0.5
Height = 2
```

The key observable: under the same world-Y torque, `Cyl_Upright` spins ~3.2× faster than `Cyl_Tilt90`. After 90° rotation about world X, the cylinder's spin axis (local Y) now points along world Z, so the world-Y torque hits the high-inertia transverse axis.

**Test data:**

Same world-space torque τ = (0, 5, 0) for all tests. Cylinder: r=0.5, h=2.0, m=1 (Iy=0.125, Ixz=0.3958). `invInertiaTensorWorld` computed analytically. α_world = invInertiaTensorWorld × τ. Tolerance ε = 1×10⁻⁴.

| Test ID | InitialRotation | invInertiaTensorWorld (diag) | τ_world | α_world |
|---------|----------------|------------------------------|---------|---------|
| Cyl_Upright | identity (0°) | (2.527, 8.0, 2.527) | (0,5,0) | (0, 40.0, 0) |
| Cyl_Tilt45X | Rx(45°) | off-diagonal | (0,5,0) | mixed |
| Cyl_Tilt90X | Rx(90°) | (2.527, 2.527, 8.0) | (0,5,0) | (0, 12.63, 0) |
| Cyl_TiltY45 | Ry(45°) | (2.527, 8.0, 2.527) | (0,5,0) | (0, 40.0, 0) |

For `Cyl_Upright`: local Y = world Y → spin axis → I_world⁻¹ diagonal (I_xz⁻¹, I_y⁻¹, I_xz⁻¹) = (2.527, 8.0, 2.527) → α_y = 8.0 × 5 = 40 rad/s².

For `Cyl_Tilt90X`: after Rx(90°), local Y points along world +Z. The congruence transform gives diagonal (I_xz⁻¹, I_xz⁻¹, I_y⁻¹) = (2.527, 2.527, 8.0). World-Y torque now hits the (2,2) = I_xz⁻¹ entry → α_y = 2.527 × 5 = 12.63 rad/s² (slow transverse).

For `Cyl_TiltY45`: a Y-rotation leaves local Y unchanged (it is the rotation axis), so the tensor diagonal is unaffected → same fast α as upright.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The congruence transform `R · I⁻¹ · R^T` rotates the body's principal axes into world space. When the cylinder has been rotated 90° about X, its spin axis (formerly local Y) now points along world Z — the tensor correctly routes world-Z torque through the low-inertia entry and world-Y torque through the high-inertia entry. This is not a special case but the automatic consequence of the matrix algebra.

- *Did you make any mistakes?*
    - None. The optimisation worked immediately because the per-frame integration loop was already in place; adding one mat3 multiply before the torque application was straightforward and produced the correct orientation-dependent results without any additional logic.

- *In what way has your knowledge improved?*
    - `invInertiaTensorWorld` is a *snapshot* valid only for the current orientation. Caching it across frames after the body has rotated would give physically wrong results. The fact that it must be recomputed every frame is not a limitation — it is the physical reality that the rotational response of a non-symmetric body depends on how it is currently oriented.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q6 Add inertia tensor for a cuboid (Formative)
Add a cuboid shape to your physics engine. The interia tensor for a cuboid is as follows:

![Inertia Tensor Formulae](../markdown-resources/Simulation-Lab/6/SimulationLab6_Image2.png)

Where the cuboid is axis aligned, and a, b and c are the dimensions of the cuboid aligned with the x, y and z axes respectively.

### Solution

The cuboid inertia tensor is diagonal when the box is axis-aligned. Let a = sizeX, b = sizeY, c = sizeZ (full extents):

- Ix = (1/12)m(b² + c²) — resistance to rotation about X (driven by Y and Z extents)
- Iy = (1/12)m(a² + c²) — resistance to rotation about Y (driven by X and Z extents)
- Iz = (1/12)m(a² + b²) — resistance to rotation about Z (driven by X and Y extents)

An elongated box (0.5×0.5×2.0) has very low Iz (the two small extents) but high Ix and Iy (the long Z extent acts as a lever arm). This produces a dramatic visual difference: spinning around the long axis is ~8.5× faster than spinning around a short axis under the same torque. An isotropic cube (1×1×1) gives identical α on all three axes.

A procedural box mesh `generateBox(sizeX, sizeY, sizeZ)` was added to `GeometryUtils` with 6 faces × 4 vertices = 24 vertices, correct CCW winding per face. A companion `generateWireBox()` provides a unit box (half-extents = 1) for the `ColliderVisualizerSystem` green wire overlay, scaled at draw time by the collider's half-extents (sizeX/2, sizeY/2, sizeZ/2).

**Snippet 1 — handleBoxCollider inertia computation** (`source/scene/SceneLoader.cpp`):
```cpp
// Ix = (1/12)·m·(b²+c²),  Iy = (1/12)·m·(a²+c²),  Iz = (1/12)·m·(a²+b²)
// where a=sizeX, b=sizeY, c=sizeZ (full extents).
const float a  = bc.sizeX, b = bc.sizeY, c = bc.sizeZ;
const float Ix = (1.0f / 12.0f) * rb->mass * (b * b + c * c);
const float Iy = (1.0f / 12.0f) * rb->mass * (a * a + c * c);
const float Iz = (1.0f / 12.0f) * rb->mass * (a * a + b * b);
rb->invInertiaTensor = glm::mat3(
    1.0f / Ix, 0.0f,      0.0f,
    0.0f,      1.0f / Iy, 0.0f,
    0.0f,      0.0f,      1.0f / Iz
);
```

**Snippet 2 — generateBox declaration** (`include/assets/GeometryUtils.h`):
```cpp
// Generates a six-faced box mesh (24 vertices, 36 indices).
// Each face has its own vertices for correct flat normals.
static GE::Assets::OBJLoader::MeshData generateBox(
    float sizeX, float sizeY, float sizeZ,
    const glm::vec3& color = glm::vec3(1.0f));
```

**Snippet 3 — generateWireBox declaration** (`include/assets/GeometryUtils.h`):
```cpp
// Generates a wire box as LINE_LIST pairs (8 vertices, 24 indices).
// Half-extents are all 1.0; caller scales via model matrix.
static GE::Assets::OBJLoader::MeshData generateWireBox(
    const glm::vec3& color = glm::vec3(0.514f, 0.831f, 0.169f));
```

**Snippet 4 — ini entries for all three box entities** (`config/simulation_lab6_q6_cuboid.ini`):
```ini
; Box_LongAxisSpin: 0.5×0.5×2.0, τ along long Z → Iz=0.042 → α=120 rad/s²  (fast)
[MeshRenderer]
Type = procedural
Shape = Box
Width = 0.5
Height = 0.5
Depth = 2.0
Material = Mat_Checker
[RigidBody]
Mass = 1
UseGravity = false
ConstantTorque = 0 0 5
[BoxCollider]
Size = 0.5 0.5 2.0

; Box_ShortAxisSpin: same shape, τ along short X → Ix=0.354 → α=14 rad/s²  (slow)
ConstantTorque = 5 0 0

; Box_Cube: 1×1×1, isotropic → Ix=Iy=Iz=0.167 → α=30 rad/s² on all axes
[MeshRenderer]
Shape = Box
Width = 1.0
Height = 1.0
Depth = 1.0
[BoxCollider]
Size = 1.0 1.0 1.0
```

**Snippet 5 — Worked numeric values** (a=0.5, b=0.5, c=2.0, m=1):

Iz = (1/12)(0.25+0.25) = **0.0417 N·m·s²** → α_z = 5/0.0417 = **120 rad/s²** (FAST)

Ix = (1/12)(0.25+4.0) = **0.3542 N·m·s²** → α_x = 5/0.3542 = **14.1 rad/s²** (SLOW)

For cube (1×1×1): Ix=Iy=Iz = (1/12)(1+1) = **0.1667** → α = 5/0.1667 = **30.0 rad/s²** (same on all axes)

Ratio: 120 / 14.1 ≈ **8.5×** — the elongated box spins 8.5 times faster along its long axis than its short axis under the same torque.

**Test data:**

τ = (5, 0, 0) or (0, 0, 5) constant torque. α_axis = (1/I_axis) × τ_axis. Tolerance ε = 1×10⁻⁴. Full extents a=sizeX, b=sizeY, c=sizeZ.

| Test ID | Size (a×b×c) | m | Torque τ | Ix = (1/12)m(b²+c²) | Iy = (1/12)m(a²+c²) | Iz = (1/12)m(a²+b²) | α (rad/s²) |
|---------|-------------|---|----------|---------------------|---------------------|---------------------|------------|
| Box_LongZ_fast | 0.5×0.5×2.0 | 1 | (0,0,5) | 0.3542 | 0.3542 | **0.0417** | **120.0** |
| Box_ShortX_slow | 0.5×0.5×2.0 | 1 | (5,0,0) | **0.3542** | 0.3542 | 0.0417 | **14.1** |
| Box_ShortY_slow | 0.5×0.5×2.0 | 1 | (0,5,0) | 0.3542 | **0.3542** | 0.0417 | **14.1** |
| Box_Cube_X | 1×1×1 | 1 | (5,0,0) | 0.1667 | 0.1667 | 0.1667 | 30.0 |
| Box_Cube_Y | 1×1×1 | 1 | (0,5,0) | 0.1667 | 0.1667 | 0.1667 | 30.0 |
| Box_Cube_Z | 1×1×1 | 1 | (0,0,5) | 0.1667 | 0.1667 | 0.1667 | 30.0 |
| Box_LongZ_2xMass | 0.5×0.5×2.0 | 2 | (0,0,5) | 0.7083 | 0.7083 | **0.0833** | **60.0** |

`Box_LongZ_fast` is ~8.5× faster than `Box_ShortX_slow` with the same torque magnitude. `Box_Cube_*` confirms isotropy: α = 30.0 rad/s² regardless of which axis the torque is applied to. `Box_ShortX_slow` and `Box_ShortY_slow` produce the same α (Ix = Iy for a box where a=b) — the square cross-section is symmetric about both transverse axes.

**Sample output:**
*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The cuboid tensor reveals how geometry directly encodes rotational resistance. The long axis (Z for 0.5×0.5×2.0) has the lowest Iz because mass is concentrated close to that axis (small a and b). Rotating around a short axis (X) requires swinging the long dimension as a lever arm, dramatically increasing Ix. The formula captures this via the squared extents perpendicular to each rotation axis.

- *Did you make any mistakes?*
    - The first scenario placeholder used `Shape = Capsule` and `Shape = Sphere` because `generateBox` did not yet exist. Once the box mesh generator was implemented and the `"Box"` shape case was added to `SceneLoader`, the visual and collider became consistent. The `Scale = 0.5 0.5 2` entries in `[Transform]` (which were compensating for the capsule placeholder) were also removed once the box dimensions were expressed directly as `Width`/`Height`/`Depth`.

- *In what way has your knowledge improved?*
    - The cuboid tensor is diagonal only when the box is axis-aligned. Rotating the box (via `InitialRotation`) causes `invInertiaTensorWorld = R · I⁻¹_body · R^T` to become fully non-diagonal, with off-diagonal terms expressing the coupling between axes. The Q5 world-space tensor handles this automatically — no special-casing for cuboid orientation is required.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

## Final Reflection

Lab 6 built a complete angular dynamics stack on top of the Lab 5 orientation system. Q1 introduced torque accumulation (τ = r × F, α = τ with identity inertia), establishing the accumulator pattern. Q2 replaced the identity tensor with the sphere formula I = (2/5)mr², making α = τ/I dependent on both mass and radius. Q3 extended this to a diagonal non-isotropic tensor for the cylinder, revealing that the spin axis (low Iy) responds far faster than the transverse axes (high Ixz). Q4 analysed the per-frame cost, identifying the world-space tensor refresh as the dominant operation. Q5 formalised the cached world-space tensor `invInertiaTensorWorld = R · I⁻¹ · R^T`, eliminating manual coordinate conversion. Q6 completed the picture with the cuboid tensor, demonstrating the most dramatic inertia contrast: an elongated box spins ~8.5× faster along its long axis than a short axis.

The unifying pattern across all six questions is the `constantTorque` field — a data-driven persistent torque injected each frame from `.ini` configuration, enabling clear spin-up visualisations in all four scenarios without any per-frame scripting.
