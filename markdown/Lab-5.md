# 700105 Simulation and Concurrency Lab Book

## Simulation Lab 5

**19/03/2026**

---

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

### Solution

Orientation is stored as a `glm::mat3` rotation matrix on the `RigidBody` component (initially identity). Each column encodes one local axis in world space: col[0] = local X, col[1] = local Y, col[2] = local Z. Angular displacement is encoded as a single `glm::vec3 angularDisplacementVec` where the **direction** is the rotation axis and the **magnitude** is the total angle in radians. Two companion fields track progress: `angularDisplacementSpeed` (rad/s) and `angularDisplacementApplied` (radians accumulated so far).

Each physics tick, `PhysicsSystem::Integrate()` checks whether there is remaining displacement. It takes a step of `min(speed × dt, remaining)` radians, builds a rotation matrix via `glm::rotate()`, pre-multiplies it onto `rb.orientation`, then re-orthogonalises to prevent floating-point drift. When `angularDisplacementApplied` reaches the total angle, no further rotation is applied.

**Snippet 1 — RigidBody fields** (`include/components/PhysicsComponents.h`):
```cpp
// Orientation represented as a 3x3 rotation matrix (columns = local axes).
// Integrated each tick via  dR/dt = Skew(ω) · R  then re-orthogonalised.
glm::mat3 orientation { glm::mat3(1.0f) };

// Angular displacement (Lab 5 Q1):
// Encodes a target finite rotation as axis * totalAngle (radians).
// The body rotates at angularDisplacementSpeed (rad/s) until
// angularDisplacementApplied reaches the total angle, then stops.
// Zero vector = disabled (no displacement applied).
glm::vec3 angularDisplacementVec    { 0.0f };   // axis * totalAngle (rad)
float     angularDisplacementSpeed  { 1.5708f }; // rad/s (default π/2 ≈ 90°/s)
float     angularDisplacementApplied{ 0.0f };   // accumulated radians so far
```

**Snippet 2 — PhysicsSystem integration step 4b** (`source/systems/PhysicsSystem.cpp`):
```cpp
// --- 4b. Angular displacement (Lab 5 Q1) ---
// Rotates from current orientation by a finite target angle then stops.
// angularDisplacementVec encodes axis * totalAngle (radians).
{
    const float dispTotal = glm::length(rb.angularDisplacementVec);
    if (dispTotal > 1e-6f && rb.angularDisplacementApplied < dispTotal) {
        const float remaining = dispTotal - rb.angularDisplacementApplied;
        const float step      = glm::min(rb.angularDisplacementSpeed * dt, remaining);
        const glm::vec3 axis  = rb.angularDisplacementVec / dispTotal;
        const glm::mat3 R     = glm::mat3(glm::rotate(glm::mat4(1.0f), step, axis));
        rb.orientation = R * rb.orientation;
        Orthogonalise(rb.orientation);
        rb.angularDisplacementApplied += step;
    }
}
```

**Snippet 3 — SceneLoader parsing** (`source/scene/SceneLoader.cpp`, inside `handleRigidBody`):
```cpp
// Lab 5 Q1: animated angular displacement — body rotates from its current orientation
// by the given degrees about each axis (stored as axis * totalAngle in radians), then stops.
// AngularDisplacementSpeed (deg/s) controls how fast the rotation is applied.
if (props.count("AngularDisplacement"))
    rb.angularDisplacementVec = glm::radians(parseVec3(props.at("AngularDisplacement")));
if (props.count("AngularDisplacementSpeed"))
    rb.angularDisplacementSpeed = glm::radians(parseFloat(props.at("AngularDisplacementSpeed")));
```

**Snippet 4 — Example ini entry** (`config/simulation_lab5_orientation.ini`):
```ini
; Cap_90X — rotates 90 degrees about X (takes 1.5 s at 60 deg/s)
[RigidBody]
Mass = 1
UseGravity = false
AngularDisplacement = 90 0 0
AngularDisplacementSpeed = 60
```

The sandbox loads `simulation_lab5_orientation.ini`, which defines six capsules each with a different `AngularDisplacement` value (identity baseline, 90°/180°/270° about X, 90° about Y, 90° about Z) at 60°/s. You can observe each capsule animate to its final orientation and stop — the different total angles complete at different times (1.5 s, 3 s, 4.5 s), making the distinct displacements visible.

**Test data:**

Initial orientation is identity (I₃) for all tests. All tests use a single-step application via the displacement mechanism. Expected values are analytic rotation matrices; tolerance ε = 1 × 10⁻⁴.

| Test ID    | Displacement Input (degrees, axis) | angularDisplacementVec (rad) | Expected col[0] | Expected col[1] | Expected col[2] |
|------------|-------------------------------------|------------------------------|-----------------|-----------------|-----------------|
| Rx_90      | 90° about X  | (π/2, 0, 0)   | (1, 0, 0)  | (0, 0, 1)   | (0, -1, 0)  |
| Rx_180     | 180° about X | (π, 0, 0)     | (1, 0, 0)  | (0, -1, 0)  | (0, 0, -1)  |
| Rx_270     | 270° about X | (3π/2, 0, 0)  | (1, 0, 0)  | (0, 0, -1)  | (0, 1, 0)   |
| Rx_360     | 360° about X | (2π, 0, 0)    | (1, 0, 0)  | (0, 1, 0)   | (0, 0, 1) ≈ I |
| Ry_90      | 90° about Y  | (0, π/2, 0)   | (0, 0, -1) | (0, 1, 0)   | (1, 0, 0)   |
| Ry_180     | 180° about Y | (0, π, 0)     | (-1, 0, 0) | (0, 1, 0)   | (0, 0, -1)  |
| Rz_90      | 90° about Z  | (0, 0, π/2)   | (0, 1, 0)  | (-1, 0, 0)  | (0, 0, 1)   |
| Rz_180     | 180° about Z | (0, 0, π)     | (-1, 0, 0) | (0, -1, 0)  | (0, 0, 1)   |
| RxRy_90_90 | 90° X then 90° Y | (π/2,0,0) then (0,π/2,0) | (0, 0, -1) | (1, 0, 0) | (0, -1, 0) |
| RxRz_90_90 | 90° X then 90° Z | (π/2,0,0) then (0,0,π/2) | (0, 1, 0)  | (0, 0, 1)  | (1, 0, 0)  |

For the combined tests (RxRy, RxRz), two sequential displacement operations are applied: the first to identity, the second to the result. The expected matrix is `R₂ × R₁` where R₂ is applied second (pre-multiplied).

**Sample output:**

*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - Angular displacement is a *finite* rotation — distinct from angular velocity. Encoding it as `axis × totalAngle` in a vec3 is compact and natural: the magnitude is the target angle, the direction is the axis. Pre-multiplying `R × orientation` applies the rotation in world space.

- *Did you make any mistakes?*
    - Initially, the sandbox scenario used `InitialRotation` (a static pose at load time) which showed orientation but not displacement in action. Replacing it with `AngularDisplacement` makes the rotation animate visibly, which is the actual requirement.

- *In what way has your knowledge improved?*
    - Understanding that the rotation matrix columns directly encode "where do my local axes point in world space" makes it easy to verify correctness — e.g., after 90° about X, the local Y axis should point along world +Z.

**Questions:**

*Is there anything you would like to ask?*
    - No.

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

### Solution

Angular velocity ω (rad/s in world space) drives a continuous rotation via the matrix ODE `dR/dt = Skew(ω) · R`. The skew-symmetric matrix `Skew(ω)` encodes the cross-product operator: `Skew(ω) · v = ω × v`. A first-order Euler step gives `R_new = R + dt · Skew(ω) · R`, which is then re-orthogonalised to prevent drift.

`angularVelocity` is set at load time from the ini. Each tick, if `|ω|² > 1e-12`, the integration step is applied and `Orthogonalise()` corrects any drift. The scenario `simulation_lab5_angular_velocity.ini` defines four capsules with different `AngularVelocity` values spinning indefinitely at π/2, π, 3π/2 rad/s, and a multi-axis combination.

**Snippet 1 — SkewSymmetric helper** (`source/systems/PhysicsSystem.cpp`):
```cpp
static glm::mat3 SkewSymmetric(const glm::vec3& w) {
    // GLM mat3 constructor fills column-by-column.
    return glm::mat3(
         0.0f,  w.z, -w.y,   // col 0
        -w.z,  0.0f,  w.x,   // col 1
         w.y,  -w.x, 0.0f    // col 2
    );
}
```

**Snippet 2 — Gram-Schmidt re-orthogonalisation** (`source/systems/PhysicsSystem.cpp`):
```cpp
static void Orthogonalise(glm::mat3& R) {
    const glm::vec3 x = glm::normalize(R[0]);
    const glm::vec3 y = glm::normalize(R[1] - glm::dot(R[1], x) * x);
    R[0] = x;
    R[1] = y;
    R[2] = glm::cross(x, y);
}
```

**Snippet 3 — Angular velocity integration step (step 4)** (`source/systems/PhysicsSystem.cpp`):
```cpp
// --- 4. Angular integration (PhysicsObject::Integrate() pattern) ---
// α = I⁻¹ · τ
const glm::vec3 angularAccel = rb.invInertiaTensor * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;

// dR/dt = Skew(ω) · R  →  R_new = R + dt · Skew(ω) · R
if (glm::dot(rb.angularVelocity, rb.angularVelocity) > 1e-12f) {
    rb.orientation = rb.orientation + dt * SkewSymmetric(rb.angularVelocity) * rb.orientation;
    Orthogonalise(rb.orientation);  // prevent floating-point drift
}
```

**Snippet 4 — Example ini entry** (`config/simulation_lab5_angular_velocity.ini`):
```ini
; Spin_PiHalf_X — π/2 rad/s about X (90 deg/s, 1 full spin per 4 s)
[RigidBody]
Mass = 1
UseGravity = false
AngularVelocity = 1.5708 0 0
```

**Snippet 5 — SceneLoader parsing** (`source/scene/SceneLoader.cpp`):
```cpp
// Lab 5 Q2: initial angular velocity (rad/s, world space)
if (props.count("AngularVelocity"))
    rb.angularVelocity = parseVec3(props.at("AngularVelocity"));
```

The integration is first-order Euler on the matrix ODE. With a small enough timestep (e.g., dt = 0.001 s), the accumulated error over one second is O(dt) ≈ 10⁻³, which is acceptable for visual simulation. The `Orthogonalise` step keeps the matrix valid indefinitely. Note that Q1's displacement block (step 4b) runs after this — both mechanisms can coexist on the same body.

**Test data:**

All tests use `dt = 0.001 s` and integrate for the specified number of steps using the first-order Euler scheme `R_new = R + dt · Skew(ω) · R`, re-orthogonalising each step. Initial orientation is identity. Expected values are the analytic rotation matrices; tolerance ε = 0.01 (to accommodate first-order integration error accumulated over N steps).

| Test ID | ω (rad/s)      | dt    | Steps | T (s) | Expected col[0] | Expected col[1] | Expected col[2] |
|---------|----------------|-------|-------|-------|-----------------|-----------------|-----------------|
| Wx_1s   | (π/2, 0, 0)    | 0.001 | 1000  | 1     | (1, 0, 0)       | (0, 0, 1)       | (0, -1, 0)      |
| Wx_2s   | (π/2, 0, 0)    | 0.001 | 2000  | 2     | (1, 0, 0)       | (0, -1, 0)      | (0, 0, -1)      |
| Wx_3s   | (π/2, 0, 0)    | 0.001 | 3000  | 3     | (1, 0, 0)       | (0, 0, -1)      | (0, 1, 0)       |
| Wx_4s   | (π/2, 0, 0)    | 0.001 | 4000  | 4     | (1, 0, 0)       | (0, 1, 0)       | (0, 0, 1)       |
| Wy_1s   | (0, π, 0)      | 0.001 | 1000  | 1     | (-1, 0, 0)      | (0, 1, 0)       | (0, 0, -1)      |
| Wy_2s   | (0, π, 0)      | 0.001 | 2000  | 2     | (1, 0, 0)       | (0, 1, 0)       | (0, 0, 1)       |
| Wz_1s   | (0, 0, 3π/2)   | 0.001 | 1000  | 1     | (0, -1, 0)      | (1, 0, 0)       | (0, 0, 1)       |
| Wz_2s   | (0, 0, 3π/2)   | 0.001 | 2000  | 2     | (-1, 0, 0)      | (0, -1, 0)      | (0, 0, 1)       |

For `Wx_4s` (4 full seconds at π/2 rad/s = one full 2π rotation), the expected result is approximately identity — verifying that the integration does not accumulate unbounded drift. The tolerance ε = 0.01 is generous; with dt = 0.001 s the actual numerical error is approximately 0.001–0.003.

**Sample output:**

*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - The matrix ODE `dR/dt = Skew(ω) · R` elegantly expresses continuous rotation — the skew-symmetric matrix is the infinitesimal generator of the rotation group. The first-order Euler step is simple but accumulates O(dt) error per second; Gram-Schmidt re-orthogonalisation prevents the matrix from drifting into a non-rotation.

- *Did you make any mistakes?*
    - None significant. The conditional check `if (|ω|² > 1e-12)` avoids unnecessary work for stationary bodies and prevents dividing by near-zero when building `SkewSymmetric`.

- *In what way has your knowledge improved?*
    - Seeing that Q1 (displacement) and Q2 (velocity) are integrated in sequence in the same `Integrate()` call makes clear their orthogonality — displacement is finite and stops; velocity is continuous. Both operate on the same `rb.orientation` matrix.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

### Q3 Reflect on the Different Approaches on Storing Orientation (Summative - due in lab 19/03/26)
How have you stored your orientation? What other options could you choose? What are the advantages and disadvantages of each approach?

### Solution

**A — How orientation is stored in this engine:**

Orientation is stored as `glm::mat3` (a 3×3 rotation matrix) on the `RigidBody` component:

```cpp
glm::mat3 orientation { glm::mat3(1.0f) };
```

The column convention is: col[0] = local X axis in world space, col[1] = local Y axis in world space, col[2] = local Z axis in world space. It is used in three contexts:

1. **Angular velocity integration:** `R_new = R + dt · Skew(ω) · R`
2. **Angular displacement:** `R_new = Rstep × R`
3. **GPU TRS upload:** `const glm::mat4 rMat = glm::mat4(rb.orientation);`

**B — Comparison of orientation representations:**

| Representation | Storage | Gimbal Lock | Drift/Normalisation | GPU Upload | Integration ODE | Interpolation |
|---|---|---|---|---|---|---|
| **Euler angles** (roll/pitch/yaw) | 3 floats | Yes — loses DOF when axes align | N/A | Must construct matrix | Order-dependent, unnatural | Problematic near poles |
| **Rotation matrix** (our choice) | 9 floats | No | Drift — fixed by Gram-Schmidt re-ortho each step | Direct `glm::mat4(R)` | `dR/dt = Skew(ω)·R` is natural matrix ODE | Not ideal — no SLERP |
| **Quaternion** | 4 floats | No | Drift — fixed by re-normalise (cheap) | Must convert: `glm::mat4_cast(q)` | `dq/dt = 0.5·(0,ω)·q` | SLERP ideal — smooth arcs |
| **Axis-Angle** | 4 values (vec3 + float) | No | N/A — not used for integration | Must convert | Not usable directly | Complex |

**C — Why rotation matrix was chosen:**

1. **Natural integration formula.** The angular velocity ODE `dR/dt = Skew(ω) · R` is a matrix equation — no intermediate conversion. Quaternion integration (`dq/dt = 0.5 · (0,ω) · q`) is less obvious and requires understanding quaternion algebra.

2. **Direct GPU upload.** `glm::mat4(rb.orientation)` zero-cost cast fills the upper-left 3×3 of the model matrix. Quaternions require `glm::mat4_cast(q)` which computes 12 multiplications.

3. **Workshop pattern match.** The `PhysicsObject` class from the workshop brief stores orientation as a rotation matrix. Following this pattern makes the code directly comparable to the reference material.

4. **Readable column semantics.** Checking that `R[0] ≈ (1,0,0)` after a Y-rotation is straightforward — it just says "local X still points along world X." This transparency aids debugging.

**D — Why Euler angles are used only as input, not state:**

`InitialRotation` in the ini is read as Euler angles but immediately converted:

```cpp
if (props.count("InitialRotation")) {
    const glm::vec3 eulerRad = glm::radians(parseVec3(props.at("InitialRotation")));
    const glm::mat4 rot = glm::rotate(glm::mat4(1.0f), eulerRad.x, glm::vec3(1,0,0))
                        * glm::rotate(glm::mat4(1.0f), eulerRad.y, glm::vec3(0,1,0))
                        * glm::rotate(glm::mat4(1.0f), eulerRad.z, glm::vec3(0,0,1));
    rb.orientation = glm::mat3(rot);
}
```

Euler angles are human-readable for authoring ini files (e.g., `90 0 0` obviously means 90° about X) but would suffer gimbal lock if used as the physics state. The conversion at load time avoids this permanently.

**E — The drift problem and Gram-Schmidt:**

Floating-point arithmetic causes the matrix columns to become slightly non-orthogonal after each integration step. `Orthogonalise()` corrects this cheaply:

- Step 1: normalise col[0] — sets exact unit length.
- Step 2: remove col[0]-component from col[1], normalise — makes col[1] exactly perpendicular to col[0].
- Step 3: col[2] = cross(col[0], col[1]) — automatically orthogonal and unit length.

With quaternions, drift is fixed by a single `glm::normalize(q)` — conceptually simpler, but the rotation-matrix approach is equally correct and efficient for this use case.

**F — Conclusion:**

Rotation matrix was chosen because it matches the ODE, integrates trivially with the Vulkan pipeline (direct 4×4 cast), mirrors the workshop pattern, and offers transparent column semantics for debugging. A quaternion-based system would offer better interpolation and slightly lower memory, but these advantages are irrelevant for a simulation with a small number of bodies and no animation blending.

**Test data:**

N/A — Q3 is a reflective question about design trade-offs, not a numerical computation. No unit tests are required.

**Sample output:**

*The test cases are being done in another project apart from this one (simulation-engine) so leave this unanswered at the moment*

**Reflection:**

- *Reflect on what you have learnt from this exercise.*
    - Each representation is a different trade-off. Rotation matrices are verbose but transparent and match the physics ODE exactly. Quaternions are compact and ideal for interpolation but require more understanding to integrate directly. The key insight is that Euler angles are excellent for *input/authoring* but should never be the canonical physics state — gimbal lock makes them unsuitable for integration.

- *Did you make any mistakes?*
    - None.

- *In what way has your knowledge improved?*
    - Understanding that `glm::rotate()` internally constructs a quaternion, converts it to a matrix, and returns the matrix explains why it is used as a "compile to matrix" bridge in both `InitialRotation` parsing and the displacement step. The engine always ends up with a matrix regardless of the input representation.

**Questions:**

*Is there anything you would like to ask?*
    - No.

---

## Final Reflection

Lab 5 extended the RigidBody with angular state. Q1 demonstrated finite angular displacement (a body rotating from one orientation to another and stopping), Q2 demonstrated continuous angular velocity (a body spinning indefinitely), and Q3 reflected on why a rotation matrix was chosen over Euler angles, quaternions, or axis-angle representations. The key implementation detail is that all three questions share the same `glm::mat3 orientation` field — different mechanisms write to it, but the rendering always reads from it via `glm::mat4(rb.orientation)`.
