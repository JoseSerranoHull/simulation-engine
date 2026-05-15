# Chapter 5 — Physics System: Integration Methods, Rigid Bodies & Collision Response

## 5.1 What Is a Physics Simulation?

A physics simulation approximates the real world by applying Newton's laws numerically. The fundamental relationship is:

```
F = ma   →   a = F / m
```

Every frame (at a fixed timestep), for each physics object:
1. Accumulate all forces (gravity, springs, etc.)
2. Compute acceleration: `a = F / mass`
3. Integrate acceleration → velocity → position

The word **"integrate"** means: given the current state and its rate of change, compute the next state. Different numerical integration methods have different accuracy and stability trade-offs.

---

## 5.2 The RigidBody Component

```cpp
// include/components/PhysicsComponents.h
namespace GE::Components {

struct RigidBody {
    // --- Linear motion ---
    float     mass          { 1.0f };
    float     inverseMass   { 1.0f };      // Pre-computed 1/mass. inverseMass=0 → infinite mass (static)
    glm::vec3 velocity      { 0.0f };
    glm::vec3 forceAccum    { 0.0f };      // Forces accumulated during the frame, cleared after integration
    float     linearDamping { 0.0f };      // Velocity multiplied by (1 - damping*dt) each tick

    // --- Angular motion ---
    glm::vec3 angularVelocity  { 0.0f };
    glm::vec3 torqueAccum      { 0.0f };
    glm::mat3 orientation      { 1.0f };           // 3×3 rotation matrix (world space)
    glm::mat3 invInertiaTensor { 1.0f };           // Body-space inverse inertia tensor
    glm::mat3 invInertiaTensorWorld { 1.0f };      // World-space = R · I⁻¹ · Rᵀ (updated each tick)

    // --- Material ---
    float restitution { 0.5f };    // Coefficient of restitution: 0 = clay, 1 = perfect bounce

    // --- Flags ---
    bool  useGravity { true };
    bool  isStatic   { false };    // Immovable; inverseMass treated as 0 in impulse calculations
};

} // namespace GE::Components
```

**Key design: `inverseMass`**

Division is expensive. Storing `inverseMass = 1/mass` means the hot path in integration and impulse response only ever multiplies. A static body has `inverseMass = 0` — any impulse formula using `invMassA + invMassB` naturally gives zero contribution from the static body. No special-casing needed.

---

## 5.3 Three Integration Methods

The engine exposes three integration methods, selectable at runtime from the ImGui menu:

```cpp
// include/systems/PhysicsSystem.h
enum class IntegrationMethod {
    Euler,         // Forward Euler — simplest, gains energy over time
    SemiImplicit,  // Symplectic Euler — energy-conserving, default
    RK4            // Runge-Kutta 4th order — most accurate, 4× costlier
};
```

### Method 1: Forward (Explicit) Euler

The simplest method. Compute velocity from the **old** state, then update position:

```cpp
// source/systems/PhysicsSystem.cpp — Euler branch
glm::vec3 acceleration = forceAccum * rb.inverseMass;

rb.velocity              += acceleration * dt;
tr->m_worldPosition      += rb.velocity * dt;   // writes world-space position
// SyncWorldToLocal() then converts worldPosition → localPosition + rebuilds localMatrix
```

**Problem:** Euler adds energy to conservative systems. A bouncing ball bounces higher each tick. It is numerically unstable for large `dt` values.

### Method 2: Semi-Implicit (Symplectic) Euler ← Default

Update velocity **first**, then use the new velocity for position:

```cpp
// source/systems/PhysicsSystem.cpp — Semi-Implicit branch
glm::vec3 acceleration = forceAccum * rb.inverseMass;

rb.velocity              += acceleration * dt;    // velocity updated first
tr->m_worldPosition      += rb.velocity * dt;    // then use NEW velocity
```

This tiny change makes the integrator **symplectic** — it conserves energy for conservative forces (gravity, spring forces). A bouncing ball bounces to exactly the same height each time. This is the correct choice for game physics simulations.

### Method 3: Runge-Kutta 4th Order (RK4)

RK4 takes four sub-step samples and combines them with a weighted average. It is **exact** for polynomial ODEs (like constant acceleration):

```cpp
// source/systems/PhysicsSystem.cpp — RK4 branch
const glm::vec3 k1p = rb.velocity;
const glm::vec3 k2p = rb.velocity + accel * (dt * 0.5f);
const glm::vec3 k3p = rb.velocity + accel * (dt * 0.5f);
const glm::vec3 k4p = rb.velocity + accel * dt;

tr->m_worldPosition += (dt / 6.0f) * (k1p + 2.0f*k2p + 2.0f*k3p + k4p);
rb.velocity         += accel * dt;
```

For constant gravity, RK4 gives the same result as the analytic solution. It becomes beneficial only for forces that vary with position or velocity (springs, drag). The cost is 4× more force evaluations per tick.

### Comparison

```mermaid
graph LR
    subgraph Integration Method Comparison
    E["Euler\n✓ Simple\n✗ Adds energy\n✗ Unstable for large dt\nUse: demos only"]
    SI["Semi-Implicit Euler\n✓ Energy-conserving\n✓ Stable\n✓ Same cost as Euler\nUse: default for all games"]
    RK["RK4\n✓ Most accurate\n✓ Exact for const acceleration\n✗ 4× cost\nUse: accuracy benchmarks"]
    end
```

---

## 5.4 Sphere-Sphere Collision Detection

Before resolving a collision, we must detect it. Two spheres overlap when:

```
distance(centerA, centerB) < radiusA + radiusB
```

```cpp
// source/systems/PhysicsSystem.cpp — broadphase + narrowphase
for (uint32_t i = 0; i < sphereCount; ++i) {
    for (uint32_t j = i + 1; j < sphereCount; ++j) {
        // Use m_worldPosition — the world-space coordinate computed by TransformSystem
        glm::vec3 posA = transformA->m_worldPosition;
        glm::vec3 posB = transformB->m_worldPosition;
        float     sumR = sphereA.radius + sphereB.radius;

        glm::vec3 delta  = posA - posB;
        float     distSq = glm::dot(delta, delta);

        if (distSq < sumR * sumR && distSq > 0.0f) {
            float dist = glm::sqrt(distSq);
            glm::vec3 n = delta / dist;   // Unit normal A←B
            resolveCollision(idA, idB, n, dist, sumR, ...);
        }
    }
}
```

---

## 5.5 Sphere-Sphere Impulse Response

When two spheres collide, we apply an **impulse** — an instantaneous change in momentum — along the collision normal. The impulse formula for two bodies with arbitrary masses is:

```
j = -(1 + e) * v_rel / (1/mA + 1/mB)
```

Where:
- `e` = coefficient of restitution (minimum of the two bodies' restitution values)
- `v_rel` = relative velocity along the collision normal (negative = approaching)
- `j` = impulse magnitude (scalar)

```cpp
// source/systems/PhysicsSystem.cpp — ResolveCollisions()
glm::vec3 velA = rbA->velocity;
glm::vec3 velB = rbB->velocity;

// Relative approach velocity along collision normal
float vRel = glm::dot(velA - velB, n);

// Only resolve if approaching (vRel < 0)
if (vRel < 0.0f) {
    // Pick the lower restitution (e.g., rubber hitting stone → rubber's bounce)
    float e = std::min(rbA->restitution, rbB->restitution);

    // General impulse formula — works for equal mass, unequal mass, and static bodies
    const float totalInvMass = rbA->inverseMass + rbB->inverseMass;
    if (totalInvMass <= 0.0f) return;  // Both infinitely massive — do nothing

    const float j = -(1.0f + e) * vRel / totalInvMass;

    // Apply inverse-mass-weighted impulse (heavier body changes velocity less)
    rbA->velocity += j * rbA->inverseMass * n;   // Push A away from B
    rbB->velocity -= j * rbB->inverseMass * n;   // Push B away from A
}
```

**For a static body (`inverseMass = 0`):**
- The formula gives `j = -(1+e) * vRel / (invMassA + 0)` = normal impulse
- `velB -= j * 0 * n` = no change (static body never moves)
- This correctly handles the "ball hits wall" case without any special casing

### Positional Correction (Preventing Sinking)

After applying impulse, the spheres may still physically overlap. Without correction, objects sink into each other over many frames. The correction writes to `m_worldPosition`, then calls `SyncWorldToLocal` to keep `m_localPosition` and `m_localMatrix[3]` consistent — essential so TransformSystem computes the correct `m_worldMatrix` for the next frame:

```cpp
// Positional correction
const glm::vec3 correction = (penetration / totalInvMass) * n;
if (!aIsStatic) {
    aTrans->m_worldPosition += correction * invMassA;
    SyncWorldToLocal(*aTrans, em);   // patches m_localPosition and m_localMatrix[3]
}
if (!bIsStatic) {
    bTrans->m_worldPosition -= correction * invMassB;
    SyncWorldToLocal(*bTrans, em);
}
```

**Why `SyncWorldToLocal` must also patch `m_localMatrix[3]`:** TransformSystem sets `m_worldMatrix = m_localMatrix` (for root entities) in pass 2, then extracts `m_worldPosition` from `m_worldMatrix[3]` in pass 3. If `m_localMatrix[3]` were stale (pointing to the pre-correction position), pass 3 would overwrite the corrected `m_worldPosition` — causing the "quicksand sinking" bug where objects slowly drift through floors.

---

## 5.6 Sphere-Plane Collision

A plane is defined by a normal `n` and a signed distance `d` from the origin: `dot(x, n) = d`.

```cpp
// Signed distance from sphere centre to plane
GE::Physics::Sphere sphere(sTrans->m_worldPosition, sCol.radius);
float dist = plane.DistanceToPoint(sphere.GetCenter());

if (dist < sphere.GetRadius()) {
    float penetration = sphere.GetRadius() - dist;

    // Push sphere out of plane (world-space correction)
    sTrans->m_worldPosition += plane.GetNormal() * penetration;
    SyncWorldToLocal(*sTrans, em);  // keep localPosition and localMatrix[3] in sync

    // Reflect velocity
    float vRelN = glm::dot(rb.velocity, plane.GetNormal());
    if (vRelN < 0.0f) {
        rb.velocity -= (1.0f + e) * vRelN * plane.GetNormal();
    }
}
```

---

## 5.7 Angular Dynamics

Beyond translational motion, the engine simulates **rotation** driven by torque.

### Inertia Tensor

The inertia tensor `I` is the rotational analogue of mass — it describes how resistant a body is to angular acceleration about each axis. For a uniform sphere: `I = (2/5) * m * r²` (same on all axes). For a box: different values on each axis.

The engine stores `invInertiaTensor` (the inverse, for cheaper calculation) in **body space** (the object's local axes) and transforms it to world space each tick:

```cpp
// source/systems/PhysicsSystem.cpp — angular dynamics update
// Transform inertia tensor from body space to world space
rb.invInertiaTensorWorld =
    rb.orientation * rb.invInertiaTensor * glm::transpose(rb.orientation);
// (R · I⁻¹ · Rᵀ transforms the body-space tensor by the current rotation)

// Angular acceleration = I⁻¹_world * torque
glm::vec3 angularAccel = rb.invInertiaTensorWorld * rb.torqueAccum;
rb.angularVelocity += angularAccel * dt;
```

### Integrating Rotation

Rotation is stored as a 3×3 matrix (not Euler angles or quaternion — avoids gimbal lock and quaternion normalisation). The derivative of a rotation matrix is:

```
dR/dt = [ω]× · R
```

Where `[ω]×` is the skew-symmetric matrix of angular velocity ω:

```cpp
static glm::mat3 SkewSymmetric(const glm::vec3& w) {
    return glm::mat3(
         0.0f,  w.z, -w.y,   // Column 0
        -w.z,  0.0f,  w.x,   // Column 1
         w.y, -w.x,  0.0f    // Column 2
    );
}

// Integrate orientation
rb.orientation += dt * SkewSymmetric(rb.angularVelocity) * rb.orientation;

// Orthogonalise after integration to prevent numerical drift
// (the matrix should always be a pure rotation — no shear, no scale)
orthogonalise(rb.orientation);   // Gram-Schmidt process
```

---

## 5.8 MaterialInteractionRegistry

Different material pairs can have different bounce coefficients. A rubber ball hitting a glass surface bounces differently than rubber hitting steel:

```cpp
// include/physics/Physics.h
namespace GE::Physics {

struct MaterialInteractionRecord {
    std::string materialA;
    std::string materialB;
    float restitution;   // Override coefficient when these materials meet
};

class MaterialInteractionRegistry {
public:
    void Register(const std::string& a, const std::string& b, float restitution);
    bool Lookup(const std::string& a, const std::string& b,
                MaterialInteractionRecord& out) const;
private:
    std::vector<MaterialInteractionRecord> m_records;
};

} // namespace GE::Physics
```

**Usage in collision resolution:**

```cpp
// In PhysicsSystem::ResolveCollisions():
float e = std::min(rbA->restitution, rbB->restitution);  // Default: per-body minimum

if (m_registry != nullptr) {
    auto* matA = em->GetTIComponent<PhysicsMaterialTag>(idA);
    auto* matB = em->GetTIComponent<PhysicsMaterialTag>(idB);
    if (matA && matB) {
        MaterialInteractionRecord rec;
        if (m_registry->Lookup(matA->name, matB->name, rec)) {
            e = rec.restitution;   // Override with table value
        }
    }
}
```

Materials are registered from the FlatBuffers scene file's `interactions` table:

```json
// config/flatbufferConfig/02_materials_and_physics.json (human-readable)
"interactions": [
    { "material_a": "rubber", "material_b": "steel",  "restitution": 0.85 },
    { "material_a": "rubber", "material_b": "rubber",  "restitution": 0.70 },
    { "material_a": "glass",  "material_b": "concrete","restitution": 0.30 }
]
```

---

## 5.9 Physics System Execution Order

Each physics tick (inside `UpdateCpuStages`):

```mermaid
flowchart TD
    A["TransformSystem (stage 1)\nPass 1: rebuild localMatrix for Dirty entities\nPass 2: worldMatrix = parent * localMatrix\nPass 3: extract worldPosition/Scale/Rotation"]
    B["AnimationSystem (stage 2)\nMove animated objects along waypoints\nWrite m_localPosition/localRotation → Dirty\nStore prevPosition for kinematic velocity"]
    C["PhysicsSystem (stage 3)\nIntegrate: update m_worldPosition\nSyncWorldToLocal: world → localPosition + localMatrix[3]\nResolveCollisions × m_solverIterations"]
    D["FlockingSystem (stage 6)\nReynolds boids steering → forceAccum"]
    E["SpawnerSystem (stage 6)\nEntityFactory::InstantiatePrefab\nPre-computes worldMatrix at creation"]

    A --> B --> C --> C
    C --> D
    C --> E
```

### Multi-Pass Collision Solver

`PhysicsSystem::OnUpdate` runs `ResolveCollisions` N times per tick (default N = 3):

```cpp
void PhysicsSystem::OnUpdate(float dt) {
    Integrate(dt);
    for (int i = 0; i < m_solverIterations; ++i) {
        ResolveCollisions();
    }
}
```

Each pass propagates corrections between adjacent contact pairs. With a single pass, correcting pair A-B doesn't update pair B-C, causing stacked objects to drift. Three passes gives stable 3–4 object stacks at normal physics Hz. `m_solverIterations` is exposed in the ImGui Simulation menu (slider 1–8).

---

## 5.10 Dual Local/World Transform Property System

The engine implements Unity-style dual transform coordinates — every `Transform` carries both a **local** (stored) and a **world** (computed) position:

```cpp
// include/components/Transform.h
struct Transform {
    // Local — authored by scene loaders, AnimationSystem, Inspector
    glm::vec3 m_localPosition { 0.0f };
    glm::vec3 m_localRotation { 0.0f };  // Euler degrees YXZ
    glm::vec3 m_localScale    { 1.0f };

    // World — computed by TransformSystem each frame (read-only outside TransformSystem)
    glm::vec3 m_worldPosition { 0.0f };
    glm::vec3 m_worldRotation { 0.0f };  // Euler extracted from worldMatrix (display only)
    glm::vec3 m_worldScale    { 1.0f };

    glm::mat4 m_localMatrix { 1.0f };
    glm::mat4 m_worldMatrix { 1.0f };
    uint32_t  m_parentEntityID = UINT32_MAX;
    TransformState m_state = TransformState::Dirty;
};
```

### Why Two Positions?

Before this refactor, there was only `m_position`. This worked correctly for root entities (where local = world), but broke when entities had parents: `PhysicsSystem` treated `m_position` as world coordinates, while `TransformSystem` treated it as local — entities with parents would float at the wrong height.

Now:
- **Physics reads and writes `m_worldPosition`** — always world-space, correct regardless of parent
- **TransformSystem computes `m_worldPosition`** from `m_worldMatrix[3]` in pass 3
- **SyncWorldToLocal converts** `m_worldPosition → m_localPosition` (via inverse parent matrix) after each physics correction

### TransformSystem Three Passes

```
Pass 1 (Dirty entities only):
    m_localMatrix = translate(m_localPosition) * rotY * rotX * rotZ * scale(m_localScale)

Pass 2 (all entities):
    if root: m_worldMatrix = m_localMatrix
    else:    m_worldMatrix = parent.m_worldMatrix * m_localMatrix

Pass 3 (all entities):
    m_worldPosition = vec3(m_worldMatrix[3])
    m_worldScale    = { |col0|, |col1|, |col2| }
    m_worldRotation = YXZ Euler extracted from normalised rotation submatrix
```

### SyncWorldToLocal

PhysicsSystem calls this after every position write (integration or collision correction):

```cpp
void PhysicsSystem::SyncWorldToLocal(Transform& trans, EntityManager* em) {
    if (trans.m_parentEntityID == UINT32_MAX) {
        trans.m_localPosition = trans.m_worldPosition;
    } else {
        auto* p = em->TryGetTIComponent<Transform>(trans.m_parentEntityID);
        if (p) {
            trans.m_localPosition = vec3(inverse(p->m_worldMatrix) * vec4(trans.m_worldPosition, 1));
        }
    }
    // Patch translation column of localMatrix so TransformSystem pass 2
    // produces the correct worldMatrix without needing a full rebuild.
    trans.m_localMatrix[3] = glm::vec4(trans.m_localPosition, 1.0f);
}
```

The `m_localMatrix[3]` patch is critical. If only `m_localPosition` were updated, TransformSystem pass 2 would compute `worldMatrix` from the stale (pre-correction) `m_localMatrix`, and pass 3 would then overwrite `m_worldPosition` with the wrong value — causing the object to sink through floors over time.

---

*Next: [Chapter 6 — Cloth Simulation & Flocking Boids](06_Cloth_and_Flocking.md)*

---

## 5.11 Physics Debugging Checklist

When physics behaviour does not match expectations, use this table to diagnose the problem:

| Symptom | Most Likely Cause | Diagnostic Step | Fix |
|---------|------------------|----------------|-----|
| Objects slowly sink through floor | `SyncWorldToLocal` not called after impulse, or `m_localMatrix[3]` not patched | Log `worldPosition.y` after each tick — does it drift down gradually? | Verify `SyncWorldToLocal` runs at the END of `ResolveCollisions`, and that `m_localMatrix[3]` is patched |
| Objects explode outward on first contact | Penetration depth calculated incorrectly (negative value used as positive) | Log penetration depth at first collision | Ensure `depth = radius - distance` is positive only when overlapping |
| Restitution > 1 (objects gain energy on bounce) | `restitution` field > 1.0 or MaterialInteractionRegistry returns > 1 | Log restitution value used in impulse | Clamp to [0, 1] |
| Jitter at rest on flat surface | Micro-velocity not killed | Log velocity magnitude each tick | Kill velocity when `|v| < 0.05f` after integration |
| Physics runs slow at high Hz | Accumulator capped too low (spiral-of-death guard) | Log how many ticks execute per graphics frame | Raise `maxTicksPerFrame` from 4 to 8 |
| Tunneling (fast objects pass through geometry) | Physics Hz too low, or object moving more than its radius per tick | Log `|velocity| * dt` vs `collider radius` | Raise physics Hz; for very fast objects, implement swept sphere test |
| Spinning objects never come to rest | `angularDamping` is 0 | Check `RigidBody::angularDamping` | Set a small damping value (0.05–0.2) |
| Stack of objects collapses | Too few solver iterations | Observe with 3 boxes stacked at 120 Hz | Raise `m_solverIterations` from 3 to 6; also reduce physics Hz if accumulator is backed up |
| Collisions only detected on one side | Plane normal reversed or `isStatic` flag missing | Log `penetration = dot(posA - planeOrigin, planeNormal) - radius` | Flip the normal in the scene JSON; ensure `isStatic = true` for planes |
| Angular velocity never changes | Inertia tensor is identity (not set up) | Log `invInertiaTensorWorld` | Compute the correct analytical `I_body` for the shape (sphere: `2/5 * mass * r²`) |

---

## 5.12 Angular Dynamics: Full Derivation

This section explains WHY the angular integration works the way it does — useful for anyone
reimplementing physics from scratch.

### The Inertia Tensor

For linear motion: `F = ma`, so `a = F / m`. The inverse mass `1/m` is how much one unit of
force accelerates the object.

For rotational motion: the equivalent is the **inertia tensor** `I`. Instead of a scalar,
it is a 3×3 matrix that describes how the object's mass is distributed relative to its rotation
axis:

```
τ = I · α    →    α = I⁻¹ · τ
(torque = I × angular_accel)
```

For a uniform sphere of radius r and mass m:

```
        ⎡ 2/5·m·r²    0         0      ⎤
I_body = ⎢    0      2/5·m·r²   0      ⎥
        ⎣    0         0      2/5·m·r² ⎦
```

The diagonal entries are the same because a sphere has equal resistance to rotation around
all axes. For a box (half-extents hx, hy, hz):

```
        ⎡ 1/3·m·(hy²+hz²)         0               0       ⎤
I_body = ⎢      0          1/3·m·(hx²+hz²)         0       ⎥
        ⎣      0                   0       1/3·m·(hx²+hy²) ⎦
```

The engine stores the **inverse** of this tensor (`invInertiaTensor`) to avoid a matrix
inversion every tick.

### World-Space Inertia

The body-space tensor is defined in the object's local frame. When the object rotates, its
world-space resistance to rotation changes. The world-space inverse inertia tensor must be
recomputed every tick:

```
I_world^{-1} = R · I_body^{-1} · R^T
```

where `R` is the current rotation matrix. In code:

```cpp
// In PhysicsSystem::Integrate(), per RigidBody:
rb.invInertiaTensorWorld =
    glm::mat3(rb.orientation)
    * rb.invInertiaTensor
    * glm::transpose(glm::mat3(rb.orientation));
```

### The Skew-Symmetric Matrix for ω × r

The angular velocity vector `ω` and a point offset `r` combine as a cross product `ω × r`
to give the velocity contribution from rotation. The cross product can be written as a matrix
multiplication using the **skew-symmetric matrix** of ω:

```
         ⎡  0   -ωz   ωy ⎤
Skew(ω) = ⎢  ωz   0   -ωx ⎥
         ⎣ -ωy   ωx    0  ⎦

ω × r = Skew(ω) · r
```

This is useful for computing the angular contribution to velocity at a contact point:
`v_contact = linearVelocity + Skew(ω) · r_contact`.

### Gram-Schmidt Re-Orthogonalisation

After thousands of integration steps, floating-point rounding makes the rotation matrix `R`
slightly non-orthogonal (its columns drift from being exactly perpendicular with unit length).
An un-orthogonal rotation matrix produces distorted shapes and incorrect inertia computations.

Gram-Schmidt fixes this by re-deriving the third column from the first two:

```cpp
// After updating rb.orientation in PhysicsSystem:
glm::vec3 col0 = glm::normalize(glm::vec3(rb.orientation[0]));
glm::vec3 col1 = glm::normalize(
    glm::vec3(rb.orientation[1]) - glm::dot(glm::vec3(rb.orientation[1]), col0) * col0);
glm::vec3 col2 = glm::cross(col0, col1);  // guaranteed orthogonal
rb.orientation[0] = glm::vec4(col0, 0.0f);
rb.orientation[1] = glm::vec4(col1, 0.0f);
rb.orientation[2] = glm::vec4(col2, 0.0f);
```

This should be applied every N ticks (e.g., every 100 ticks) rather than every tick, as it is
relatively expensive for a minor correction.

---

## 5.13 Collision Algorithm Reference — All 10 Passes

This section explains the mathematical primitive behind every collision pass in `PhysicsSystem::ResolveCollisions()`. Each subsection gives: the input data, the detection test, how to compute the contact normal and penetration depth, and the most common implementation mistake. After detection, **all passes feed the same impulse formula** from §5.5.

---

### Pass A — Sphere vs Plane

**Primitive:** Signed distance from a point to a half-space.

A plane is defined by a unit normal `n̂` and a distance `d` from the world origin: any point `p` on the plane satisfies `dot(p, n̂) = d`. The signed distance from an arbitrary point `c` to the plane is `signed_dist = dot(c, n̂) − d`.

```
signed_dist = dot(sphere.center, plane.normal) − plane.d
penetration = sphere.radius − signed_dist

if penetration > 0:
    contact_normal = plane.normal          // points from plane into sphere
    push sphere.center by +contact_normal * penetration
    reflect velocity: v -= (1+e) * dot(v, n̂) * n̂
```

**Common mistake:** Using the unsigned distance. If the sphere is on the back side of the plane, `signed_dist` is negative and `penetration` would be large and positive — the sphere would be pushed *into* the plane instead of out of it. Always test `signed_dist > 0` (sphere is on the front side) before applying the response.

---

### Pass B — Sphere vs Sphere

**Primitive:** Distance between two points vs. sum of radii.

```
delta = centerA − centerB
distSq = dot(delta, delta)
sumR   = radiusA + radiusB

if distSq < sumR² and distSq > 0:
    dist        = sqrt(distSq)
    normal      = delta / dist          // unit vector A←B (points into A)
    penetration = sumR − dist
    → apply two-body impulse (§5.5) along normal
    → apply positional correction (§5.5) along normal
```

**Common mistake:** Not guarding `distSq > 0`. Two coincident spheres give `delta = {0,0,0}`, making `dist = 0` and the normal division undefined (NaN). In practice this happens when a spawner creates two objects at the same position; the guard skips the resolve for that one frame and they separate naturally.

---

### Pass C — Sphere vs Axis-Aligned Box (AABB)

**Primitive:** Clamp sphere centre to the box extents; measure distance to the clamped point.

The box is defined by its world-space centre `boxCenter` and half-extents `halfExtents` (a vec3). The closest point on the box surface to the sphere centre is found by clamping each coordinate independently:

```
closest = clamp(sphere.center, boxCenter − halfExtents, boxCenter + halfExtents)
delta   = sphere.center − closest
distSq  = dot(delta, delta)

if distSq < sphere.radius²:
    if distSq > 0:
        dist    = sqrt(distSq)
        normal  = delta / dist      // sphere centre is outside box
    else:
        // sphere centre is INSIDE the box — find the shallowest escape axis
        find the axis where (sphere.center − boxMin) or (boxMax − sphere.center) is smallest
        normal = outward direction along that axis
        dist   = 0
    penetration = sphere.radius − dist
    → apply impulse + correction (treat box as static if inverseMass = 0)
```

**Common mistake:** Not handling the inside-sphere case (`distSq = 0`). If the sphere centre is fully inside the box (happens after spawning inside geometry), `delta = {0,0,0}` and normalisation fails. The fallback finds the minimum-penetration axis and ejects the sphere along it.

---

### Pass D — Box vs Plane (Support Point)

**Primitive:** The "support point" — the box vertex (corner) that is furthest in the direction opposite the plane normal.

For collision, the deepest penetrating corner of the box is the one that most violates the plane. Iterate all 8 corners of the box (constructed from ±halfExtents rotated by the box's world orientation), compute the signed distance of each to the plane, and take the most negative one:

```
for each of 8 corners c of box:
    signed_dist = dot(c, plane.normal) − plane.d
    if signed_dist < deepest:
        deepest      = signed_dist
        deepest_corner = c

penetration = -deepest   // positive if corner is on wrong side
if penetration > 0:
    normal = plane.normal
    push box by +normal * penetration * inverseMass  (if not static)
    apply impulse at contact_point = deepest_corner
```

**Why only one corner?** For a resting box on a flat floor, two corners touch simultaneously. Using the deepest single corner produces a one-point contact, which can cause rocking. A more accurate solver contacts all penetrating corners; for this engine, the multi-pass solver (3 iterations) corrects the rocking within a few ticks.

**Common mistake:** Forgetting to rotate the corner offsets by the box's world orientation matrix. `boxCenter ± halfExtents` gives the axis-aligned corners — correct only when `orientation = identity`.

---

### Pass E — Capsule vs Plane

**Primitive:** Closest point on a line segment to the plane.

A capsule is a cylinder with hemispherical end-caps: defined by two endpoints `A` and `B` on the central axis and a radius `r`. Its closest point to a plane is one of the two endpoints (whichever has the smaller signed distance):

```
distA = dot(capsule.pointA, plane.normal) − plane.d
distB = dot(capsule.pointB, plane.normal) − plane.d

deepest_point = (distA < distB) ? capsule.pointA : capsule.pointB
deepest_dist  = min(distA, distB)

penetration = capsule.radius − deepest_dist
if penetration > 0:
    normal = plane.normal
    push capsule along +normal * penetration
    apply impulse
```

**Why endpoints only?** The closest point on a line segment to an infinite plane is always one of the endpoints (or both, when the segment is parallel to the plane). The interior of the segment is farther from the plane than both endpoints combined.

**Common mistake:** Computing the closest point on the segment to the *plane's origin point* (a sphere-segment distance test) rather than the closest point to the plane itself. These give different results when the segment is not perpendicular to the plane.

---

### Pass F — Cylinder vs Plane

**Primitive:** Same as Pass E — the cylinder's two axis endpoints plus radius.

A cylinder in this engine is Y-axis aligned in local space and rotated by the entity's world orientation. Its axis runs from `axisBottom` to `axisTop` (world space), each with the cylinder's radius:

```
// Identical to Pass E:
deepest_dist = min(dot(axisBottom, plane.normal), dot(axisTop, plane.normal)) − plane.d
penetration  = cylinder.radius − deepest_dist
```

The only difference from a capsule is that a capsule's end-caps are hemispherical (so the radius adds uniformly to any axis point), while a cylinder's end-caps are flat discs. For plane collision, both reduce to the same endpoint test because the flat disc's lowest edge is at the same signed distance as the axis endpoint plus radius.

---

### Pass G — Capsule vs Box

**Primitive:** Distance from a line segment to a point (box centre), then treat as sphere–box.

This is a compound test:
1. Find the closest point on the capsule's axis segment to the box centre.
2. Treat that closest point as a sphere centre with the capsule's radius.
3. Run Pass C (sphere–AABB) against the box.

```
// Step 1: closest point on segment to box center
t = clamp(dot(boxCenter − segA, segDir) / segLenSq, 0, 1)
closestOnSeg = segA + t * (segB − segA)

// Step 2: treat closestOnSeg as sphere center, run Pass C
// (sphere.center = closestOnSeg, sphere.radius = capsule.radius)
```

**Why this works:** The capsule is effectively a swept sphere along its axis. The closest axis-point to the box gives the worst-case penetration scenario. This is an approximation — it misses cases where the capsule end-cap overlaps a box corner — but is accurate enough for games and avoids the full GJK algorithm.

---

### Pass H — Sphere vs Capsule

**Primitive:** Distance from a point to a line segment.

The closest point on the capsule axis to the sphere centre determines whether they collide:

```
// Parameterised projection of sphere.center onto the segment
t = clamp(dot(sphere.center − segA, segDir) / segLenSq, 0, 1)
closestOnAxis = segA + t * (segB − segA)

delta       = sphere.center − closestOnAxis
distSq      = dot(delta, delta)
contactDist = sphere.radius + capsule.radius

if distSq < contactDist²:
    dist        = sqrt(distSq)
    normal      = (dist > 0) ? delta / dist : UP
    penetration = contactDist − dist
    → apply two-body impulse + correction
```

The `t = clamp(…, 0, 1)` ensures the closest point is on the segment, not its infinite line extension. `t = 0` means the sphere is closest to `segA`; `t = 1` to `segB`; anything in between to a point along the axis.

**Common mistake:** Forgetting the `clamp` — the sphere would detect a collision with the imaginary extension of the capsule's axis beyond its endpoints, producing phantom impulses in empty space.

---

### Pass I — Box vs Box (Separating Axis Theorem, 3-Axis SAT)

**Primitive:** Separating Axis Theorem (SAT) — two convex objects do NOT collide if there exists a separating axis where their projections do not overlap.

For two AABBs (axis-aligned boxes), only three axes need to be tested: world X, Y, and Z. For oriented boxes (OBBs), 15 axes are needed — but this engine approximates box–box with AABB since boxes rarely have both non-trivial orientations active at once:

```
for axis in {world_X, world_Y, world_Z}:
    projA = half_extent_A_along_axis
    projB = half_extent_B_along_axis
    dist  = |center_A_along_axis − center_B_along_axis|
    overlap_along_axis = (projA + projB) − dist

    if overlap_along_axis <= 0:
        return NO_COLLISION     // Separating axis found

// If no separating axis found, find the axis with minimum overlap:
min_overlap = min(overlap_X, overlap_Y, overlap_Z)
contact_normal = axis corresponding to min_overlap (signed toward A)
penetration    = min_overlap
→ apply impulse + correction
```

**Why minimum overlap?** Resolving along the axis of minimum penetration moves the boxes the smallest possible distance to separate them, which minimises positional correction artifacts.

**Common mistake:** Using the center-to-center vector as the normal instead of the minimum-overlap axis. For boxes resting on a floor, the center-to-center vector points diagonally, pushing the box sideways; the minimum-overlap axis correctly points straight up (Y axis).

---

### Pass J — Cylinder vs Sphere / Box

**Primitive:** Closest point on the cylinder's central axis to the other object's centre.

This is the same closest-point-on-segment primitive used in Passes G and H, applied with the cylinder's axis. For cylinder–sphere:

```
t = clamp(dot(sphere.center − cylBottom, cylAxis) / cylLen², 0, 1)
closestOnAxis = cylBottom + t * cylDir * cylLen

radialDist = length(sphere.center − closestOnAxis)
axialDepth = ... (check if sphere is also within the axial extent)

contactDist = cyl.radius + sphere.radius
if radialDist < contactDist:
    normal      = normalize(sphere.center − closestOnAxis)
    penetration = contactDist − radialDist
```

For cylinder–box: use the box-centre as the query point (same as Pass G but with cylinder parameters).

**Limitation:** This approximation treats the cylinder as a capsule for collision purposes (hemispherical end-caps instead of flat discs). For a game engine with axis-aligned cylinders and moderate velocity, this is visually indistinguishable from the correct flat-cap test. The full cylinder–box test requires 5-axis SAT with edge cross-products.

---

### Summary: Which Primitive for Each Shape Pair?

| Shape pair | Primitive | Key function |
|---|---|---|
| Sphere–Plane | Signed distance to plane | `dot(center, normal) − d` |
| Sphere–Sphere | Point–point distance | `length(A − B)` |
| Sphere–Box | Closest point on AABB | `clamp(center, boxMin, boxMax)` |
| Box–Plane | Support point (deepest corner) | iterate 8 corners |
| Capsule–Plane | Closest endpoint to plane | `min(dist(A,plane), dist(B,plane))` |
| Cylinder–Plane | Same as capsule | same |
| Capsule–Box | Closest point on segment to box centre | `closestOnSeg` → sphere–AABB |
| Sphere–Capsule | Closest point on segment to sphere centre | `clamp(t, 0, 1)` projection |
| Box–Box | 3-axis SAT | min-overlap axis |
| Cylinder–Sphere/Box | Closest point on axis | `clamp(t, 0, 1)` projection |

All detections produce a `(contact_normal, penetration)` pair. Everything else — the impulse, the mass weighting, the positional correction — is identical across all pairs (§5.5).
