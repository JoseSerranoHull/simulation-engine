/* parasoft-begin-suppress ALL */
#include <fstream>
#include <random>
#include <string>
/* parasoft-end-suppress ALL */

#include "scene/fb/FBSceneAdapter.h"
#include "Scene_generated.h"        // flatc-generated from flatbuffers/Scene.fbs

/* parasoft-begin-suppress ALL */
#include <glm/gtc/constants.hpp>   // glm::pi<float>()
/* parasoft-end-suppress ALL */

// Engine ECS & component headers
#include "ecs/EntityManager.h"
#include "components/Components.h"
#include "components/Transform.h"
#include "components/Tag.h"
#include "components/PhysicsComponents.h"
#include "components/AnimationComponents.h"
#include "scene/Scene.h"
#include "assets/AssetManager.h"
#include "assets/GeometryUtils.h"
#include "assets/Model.h"
#include "assets/Material.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/GpuUploadContext.h"
#include "core/Logger.h"

// Flat-color pipeline index within m_pipelines (appended after createMaterialPipelines())
static constexpr std::size_t FLATCOLOR_PIPELINE_INDEX = 8U;

namespace GE::Scene::FB {

// ===========================================================================
// Internal helpers
// ===========================================================================

static glm::vec3 toVec3(const Simulation::Vec3& v) {
    return { v.x(), v.y(), v.z() };
}

// ===========================================================================
// SECTION 1: load()
// ===========================================================================

bool FBSceneAdapter::load(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        GE_LOG_ERROR("FBSceneAdapter: Cannot open file: " + path);
        return false;
    }

    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    m_buffer.resize(static_cast<std::size_t>(size));
    if (!file.read(reinterpret_cast<char*>(m_buffer.data()), size)) {
        GE_LOG_ERROR("FBSceneAdapter: Failed to read file: " + path);
        return false;
    }

    // Verify the buffer before accessing it
    flatbuffers::Verifier verifier(m_buffer.data(), m_buffer.size());
    if (!Simulation::VerifySceneBuffer(verifier)) {
        GE_LOG_ERROR("FBSceneAdapter: FlatBuffers verification failed for: " + path);
        return false;
    }

    // Zero-copy pointer into owned buffer
    m_scene = Simulation::GetScene(m_buffer.data());
    m_sceneName = (m_scene->name() != nullptr) ? m_scene->name()->str() : "Unnamed";

    return true;
}

// ===========================================================================
// SECTION 2: adaptToECS() — single pass, no intermediate copies
// ===========================================================================

void FBSceneAdapter::adaptToECS(FBSceneContext& ctx) {
    if (m_scene == nullptr) { return; }

    // Order matters: materials must be available when adaptObjects() looks them up
    if (m_scene->cameras()      != nullptr) { adaptCameras(ctx);      }
    if (m_scene->materials()    != nullptr) { adaptMaterials(ctx);    }
    if (m_scene->interactions() != nullptr) { adaptInteractions(ctx); }
    if (m_scene->objects()      != nullptr) { adaptObjects(ctx);      }
    if (m_scene->spawners()     != nullptr &&
        m_scene->spawners_type() != nullptr) { adaptSpawners(ctx);    }
}

// ===========================================================================
// SECTION 3: adaptCameras()
// ===========================================================================

void FBSceneAdapter::adaptCameras(FBSceneContext& ctx) const {
    for (const auto* cam : *m_scene->cameras()) {
        if (cam != nullptr) { adaptCamera(cam, ctx); }
    }
}

void FBSceneAdapter::adaptCamera(const Simulation::Camera* cam, FBSceneContext& ctx) const {
    FBCameraRecord rec;
    rec.name = (cam->name() != nullptr) ? cam->name()->str() : "Camera";

    // Extract position from transform
    if (cam->transform() != nullptr) {
        const auto* t = cam->transform();
        rec.position = toVec3(t->position());

        // Build direction vector from yaw/pitch in the FlatBuffers RotationEuler
        const float yawRad   = glm::radians(t->orientation().yaw());
        const float pitchRad = glm::radians(t->orientation().pitch());
        rec.direction = glm::normalize(glm::vec3{
            std::cos(static_cast<double>(yawRad)) * std::cos(static_cast<double>(pitchRad)),
            std::sin(static_cast<double>(pitchRad)),
            std::sin(static_cast<double>(yawRad)) * std::cos(static_cast<double>(pitchRad))
        });
    }

    // Projection parameters
    if (cam->camera_type_type() == Simulation::CameraType::PerspectiveCamera) {
        const auto* pc = cam->camera_type_as_PerspectiveCamera();
        rec.isOrtho   = false;
        rec.fov       = (pc != nullptr && pc->fov()  > 0.0f) ? pc->fov()  : 45.0f;
        rec.nearPlane = (pc != nullptr && pc->near() > 0.0f) ? pc->near() : 0.1f;
        rec.farPlane  = (pc != nullptr && pc->far()  > 0.0f) ? pc->far()  : 200.0f;
    }
    else if (cam->camera_type_type() == Simulation::CameraType::OrthographicCamera) {
        const auto* oc = cam->camera_type_as_OrthographicCamera();
        rec.isOrtho   = true;
        rec.orthoSize = (oc != nullptr && oc->size() > 0.0f) ? oc->size() : 5.0f;
        rec.nearPlane = (oc != nullptr && oc->near() > 0.0f) ? oc->near() : 0.1f;
        rec.farPlane  = (oc != nullptr && oc->far()  > 0.0f) ? oc->far()  : 200.0f;
    }

    ctx.cameras.push_back(rec);
}

// ===========================================================================
// SECTION 4: adaptMaterials()
// ===========================================================================

void FBSceneAdapter::adaptMaterials(FBSceneContext& ctx) const {
    for (const auto* mat : *m_scene->materials()) {
        if (mat != nullptr) { adaptMaterial(mat, ctx); }
    }
}

void FBSceneAdapter::adaptMaterial(const Simulation::Material* mat, FBSceneContext& ctx) const {
    PhysicsMaterialRecord rec;
    rec.name    = (mat->name() != nullptr) ? mat->name()->str() : "material";
    rec.density = mat->density();
    ctx.physicsMaterials.push_back(rec);
}

// ===========================================================================
// SECTION 5: adaptInteractions()
// ===========================================================================

void FBSceneAdapter::adaptInteractions(FBSceneContext& ctx) const {
    for (const auto* mi : *m_scene->interactions()) {
        if (mi != nullptr) { adaptInteraction(mi, ctx); }
    }
}

void FBSceneAdapter::adaptInteraction(const Simulation::MaterialInteraction* mi,
                                       FBSceneContext& ctx) const
{
    MaterialInteractionRecord rec;
    rec.materialA     = (mi->material_a() != nullptr) ? mi->material_a()->str() : "";
    rec.materialB     = (mi->material_b() != nullptr) ? mi->material_b()->str() : "";
    rec.restitution   = mi->restitution();
    rec.staticFriction  = mi->static_friction();
    rec.dynamicFriction = mi->dynamic_friction();
    ctx.interactions.push_back(rec);
}

// ===========================================================================
// SECTION 6: adaptObjects()
// ===========================================================================

void FBSceneAdapter::adaptObjects(FBSceneContext& ctx) const {
    for (const auto* obj : *m_scene->objects()) {
        if (obj != nullptr) { adaptObject(obj, ctx); }
    }
}

void FBSceneAdapter::adaptObject(const Simulation::Object* obj, FBSceneContext& ctx) const {
    const GE::ECS::EntityID id = ctx.em->CreateEntity();
    const glm::vec3 color = resolveColor(obj, ctx);

    // --- Transform ---
    GE::Components::Transform transform;
    if (obj->transform() != nullptr) {
        const auto* t = obj->transform();
        transform.m_position = toVec3(t->position());
        transform.m_rotation = glm::vec3{ t->orientation().yaw(),
                                          t->orientation().pitch(),
                                          t->orientation().roll() };
        transform.m_scale    = toVec3(t->scale());
    }
    ctx.em->AddComponent(id, transform);

    // --- Tag & Scene registry ---
    std::string name = (obj->name() != nullptr)
        ? obj->name()->str()
        : "fb_object_" + std::to_string(id);
    ctx.em->AddComponent(id, GE::Components::Tag{ name });
    ctx.scene->addEntity(name, id);

    // --- PhysicsMaterialTag ---
    if (obj->material() != nullptr) {
        const std::string matName = obj->material()->str();
        for (const auto& rec : ctx.physicsMaterials) {
            if (rec.name == matName) {
                ctx.em->AddComponent(id, GE::Components::PhysicsMaterialTag{ rec.name, rec.density });
                break;
            }
        }
    }

    // --- Shape → Mesh + Collider ---
    const bool isContainer = (obj->collision_type() == Simulation::CollisionType::CONTAINER);
    adaptShape(obj, id, color, isContainer, ctx);

    // --- Behaviour → RigidBody / AnimatedObjectComponent ---
    adaptBehaviour(obj, id, ctx);
}

// ===========================================================================
// SECTION 6: adaptShape()
// ===========================================================================

void FBSceneAdapter::adaptShape(const Simulation::Object* obj, GE::ECS::EntityID id,
                                const glm::vec3& color, bool /*isContainer*/,
                                FBSceneContext& ctx) const
{
    if (ctx.pipelines == nullptr || ctx.pipelines->size() <= FLATCOLOR_PIPELINE_INDEX) {
        GE_LOG_ERROR("FBSceneAdapter: Flat-color pipeline (index 8) not available.");
        return;
    }

    GE::Graphics::GraphicsPipeline* const flatColorPipeline =
        (*ctx.pipelines)[FLATCOLOR_PIPELINE_INDEX].get();

    // Build a no-texture material that uses the flat-color pipeline (no Set 1).
    // Shadow casting disabled: the shadow pipeline expects Set 1 (material descriptor),
    // which this material does not have. Flat-color objects don't sample a shadow map anyway.
    auto flatMat = std::make_shared<GE::Assets::Material>(VK_NULL_HANDLE, flatColorPipeline);
    flatMat->SetCastsShadows(false);

    using namespace GE::Assets;
    OBJLoader::MeshData meshData;

    switch (obj->shape_type()) {
    case Simulation::Shape::Sphere: {
        const auto* s = obj->shape_as_Sphere();
        const float radius = (s != nullptr) ? s->radius() : 0.5f;
        meshData = GeometryUtils::generateSphere(32, radius, -radius, color);
        ctx.em->AddComponent(id, GE::Components::SphereCollider{ radius });
        break;
    }
    case Simulation::Shape::Plane: {
        const auto* p = obj->shape_as_Plane();
        meshData = GeometryUtils::generatePlane(10.0f, 10.0f);
        // Bake the vertex color into plane mesh (generatePlane doesn't accept color; tint via material)
        for (auto& v : meshData.vertices) { v.color = color; }
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        if (p != nullptr && p->normal() != nullptr) { normal = toVec3(*p->normal()); }
        ctx.em->AddComponent(id, GE::Components::PlaneCollider{ normal, 0.0f });
        break;
    }
    case Simulation::Shape::Cylinder: {
        const auto* c = obj->shape_as_Cylinder();
        const float radius = (c != nullptr) ? c->radius() : 0.5f;
        const float height = (c != nullptr) ? c->height() : 1.0f;
        meshData = GeometryUtils::generateCylinder(32, radius, radius, height, color, true, true);
        ctx.em->AddComponent(id, GE::Components::CylinderCollider{ radius, height });
        break;
    }
    case Simulation::Shape::Capsule: {
        const auto* c = obj->shape_as_Capsule();
        const float radius = (c != nullptr) ? c->radius() : 0.5f;
        const float height = (c != nullptr) ? c->height() : 1.0f;
        meshData = GeometryUtils::generateCapsule(radius, height, 32, 16);
        for (auto& v : meshData.vertices) { v.color = color; }
        ctx.em->AddComponent(id, GE::Components::CapsuleCollider{ radius, height });
        break;
    }
    case Simulation::Shape::Cuboid: {
        const auto* c = obj->shape_as_Cuboid();
        const glm::vec3 sz = (c != nullptr && c->size() != nullptr)
            ? toVec3(*c->size()) : glm::vec3{ 1.0f };
        meshData = GeometryUtils::generateBox(sz.x, sz.y, sz.z, color);
        ctx.em->AddComponent(id, GE::Components::BoxCollider{ sz.x, sz.y, sz.z });
        break;
    }
    default:
        GE_LOG_ERROR("FBSceneAdapter: Unknown shape type on object, skipping mesh.");
        return;
    }

    // Upload to GPU and attach MeshRenderer
    auto meshPtr = ctx.am->processMeshData(
        meshData, flatMat,
        ctx.uploadCtx->cmd,
        ctx.uploadCtx->stagingBuffers,
        ctx.uploadCtx->stagingMemories);

    if (meshPtr) {
        GE::Assets::Mesh* const rawPtr = meshPtr.get();
        auto dummyModel = std::make_unique<GE::Assets::Model>();
        dummyModel->addMesh(std::move(meshPtr));

        GE::Components::MeshRenderer mr;
        mr.subMeshes.push_back({ rawPtr, flatMat.get() });
        ctx.em->AddComponent(id, mr);
        ctx.ownedModels->push_back(std::move(dummyModel));
    }
    else {
        GE_LOG_ERROR("FBSceneAdapter: processMeshData failed for object.");
    }
}

// ===========================================================================
// SECTION 7: adaptBehaviour()
// ===========================================================================

void FBSceneAdapter::adaptBehaviour(const Simulation::Object* obj, GE::ECS::EntityID id,
                                    FBSceneContext& ctx) const
{
    switch (obj->behaviour_type()) {
    case Simulation::Behaviour::StaticObject: {
        GE::Components::RigidBody rb;
        rb.isStatic    = true;
        rb.useGravity  = false;
        rb.inverseMass = 0.0f;
        rb.mass        = 0.0f;
        ctx.em->AddComponent(id, rb);
        break;
    }
    case Simulation::Behaviour::SimulatedObject: {
        const auto* sim = obj->behaviour_as_SimulatedObject();
        GE::Components::RigidBody rb;

        // --- Density lookup ---
        float density = 1.0f;
        if (obj->material() != nullptr) {
            const std::string matName = obj->material()->str();
            for (const auto& rec : ctx.physicsMaterials) {
                if (rec.name == matName) { density = rec.density; break; }
            }
        }

        // --- Mass & inertia tensor from shape geometry ---
        // Planes are always treated as static (infinite mass).
        const bool isPlane = (obj->shape_type() == Simulation::Shape::Plane);
        if (isPlane) {
            rb.isStatic    = true;
            rb.useGravity  = false;
            rb.mass        = 0.0f;
            rb.inverseMass = 0.0f;
            rb.invInertiaTensor = glm::mat3(0.0f);
        } else {
            rb.isStatic   = false;
            rb.useGravity = true;
            float mass = 1.0f;
            glm::mat3 invI = glm::mat3(1.0f);

            const float PI = glm::pi<float>();

            switch (obj->shape_type()) {
            case Simulation::Shape::Sphere: {
                const auto* s = obj->shape_as_Sphere();
                const float r = (s != nullptr) ? s->radius() : 0.5f;
                const float vol = (4.0f / 3.0f) * PI * r * r * r;
                mass = density * vol;
                // I = (2/5) m r²  ·  Identity  →  I⁻¹ = 5/(2mr²) · Identity
                const float invIxx = (mass > 0.0f) ? (5.0f / (2.0f * mass * r * r)) : 0.0f;
                invI = glm::mat3(invIxx);
                break;
            }
            case Simulation::Shape::Cuboid: {
                const auto* c = obj->shape_as_Cuboid();
                const glm::vec3 sz = (c != nullptr && c->size() != nullptr)
                    ? toVec3(*c->size()) : glm::vec3{ 1.0f };
                const float w = sz.x, h = sz.y, d = sz.z;
                const float vol = w * h * d;
                mass = density * vol;
                // Ix = m(h²+d²)/12, Iy = m(w²+d²)/12, Iz = m(w²+h²)/12
                if (mass > 0.0f) {
                    invI[0][0] = 12.0f / (mass * (h*h + d*d));
                    invI[1][1] = 12.0f / (mass * (w*w + d*d));
                    invI[2][2] = 12.0f / (mass * (w*w + h*h));
                }
                break;
            }
            case Simulation::Shape::Cylinder: {
                const auto* c = obj->shape_as_Cylinder();
                const float r = (c != nullptr) ? c->radius() : 0.5f;
                const float ht = (c != nullptr) ? c->height() : 1.0f;
                const float vol = PI * r * r * ht;
                mass = density * vol;
                // Ixx = Izz = m(3r² + h²)/12, Iyy = mr²/2
                if (mass > 0.0f) {
                    const float invIxz = 12.0f / (mass * (3.0f*r*r + ht*ht));
                    const float invIy  = 2.0f  / (mass * r * r);
                    invI[0][0] = invIxz;
                    invI[1][1] = invIy;
                    invI[2][2] = invIxz;
                }
                break;
            }
            case Simulation::Shape::Capsule: {
                const auto* c = obj->shape_as_Capsule();
                const float r  = (c != nullptr) ? c->radius() : 0.5f;
                const float ht = (c != nullptr) ? c->height() : 1.0f;
                // Approximate: cylinder body + full sphere (two hemispheres), additive tensors
                const float mc = density * PI * r * r * ht;              // cylinder mass
                const float ms = density * (4.0f / 3.0f) * PI * r * r * r;  // sphere mass
                mass = mc + ms;
                if (mass > 0.0f) {
                    // Cylinder: Ixx=Izz = mc(3r²+h²)/12, Iyy = mc·r²/2
                    // Sphere:   Iall    = (2/5)·ms·r²
                    const float Is    = (2.0f / 5.0f) * ms * r * r;
                    const float Icxz  = mc * (3.0f * r * r + ht * ht) / 12.0f;
                    const float Icy   = mc * r * r * 0.5f;
                    invI[0][0] = 1.0f / (Icxz + Is);
                    invI[1][1] = 1.0f / (Icy  + Is);
                    invI[2][2] = 1.0f / (Icxz + Is);
                }
                break;
            }
            default:
                mass = density;  // Fallback: unit volume
                break;
            }

            rb.mass        = mass;
            rb.inverseMass = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
            rb.invInertiaTensor = invI;
        }

        if (sim != nullptr && sim->initial_state() != nullptr) {
            const auto* ps = sim->initial_state();
            rb.velocity        = toVec3(ps->linear_velocity());
            rb.angularVelocity = glm::radians(toVec3(ps->angular_velocity()));
        }

        ctx.em->AddComponent(id, rb);

        // Owner tag for color coding
        GE::Components::OwnerComponent oc;
        if (sim != nullptr) {
            oc.owner = static_cast<GE::Components::OwnerType>(
                static_cast<int8_t>(sim->owner()));
        }
        ctx.em->AddComponent(id, oc);
        break;
    }
    case Simulation::Behaviour::AnimatedObject: {
        const auto* anim = obj->behaviour_as_AnimatedObject();
        if (anim == nullptr) { break; }

        GE::Components::AnimatedObjectComponent ac;
        ac.totalDuration = anim->total_duration();
        ac.easing        = static_cast<GE::Components::EasingType>(
                               static_cast<uint8_t>(anim->easing()));
        ac.pathMode      = static_cast<GE::Components::PathMode>(
                               static_cast<uint8_t>(anim->path_mode()));

        if (anim->waypoints() != nullptr) {
            for (const auto* wp : *anim->waypoints()) {
                if (wp == nullptr) { continue; }
                GE::Components::FBWaypoint waypoint;
                waypoint.position = (wp->position() != nullptr) ? toVec3(*wp->position()) : glm::vec3{ 0.0f };
                if (wp->rotation() != nullptr) {
                    waypoint.rotDeg = glm::vec3{ wp->rotation()->yaw(),
                                                  wp->rotation()->pitch(),
                                                  wp->rotation()->roll() };
                }
                waypoint.time     = wp->time();
                ac.waypoints.push_back(waypoint);
            }
        }

        // Place entity at waypoint[0] position
        if (!ac.waypoints.empty()) {
            GE::Components::Transform* transform =
                ctx.em->GetTIComponent<GE::Components::Transform>(id);
            if (transform != nullptr) {
                transform->m_position = ac.waypoints[0].position;
            }
        }

        ctx.em->AddComponent(id, ac);
        break;
    }
    default:
        break;
    }
}

// ===========================================================================
// SECTION 8: resolveColor()
// ===========================================================================

glm::vec3 FBSceneAdapter::resolveColor(const Simulation::Object* obj,
                                       const FBSceneContext& ctx) const
{
    if (!ctx.useOwnerColors) {
        return FBSceneContext::defaultColor;
    }

    // Only SimulatedObject has an owner field
    if (obj->behaviour_type() == Simulation::Behaviour::SimulatedObject) {
        const auto* sim = obj->behaviour_as_SimulatedObject();
        if (sim != nullptr) {
            const auto ownerIdx = static_cast<std::size_t>(
                static_cast<int8_t>(sim->owner()));
            if (ownerIdx < FBSceneContext::ownerColors.size()) {
                return FBSceneContext::ownerColors[ownerIdx];
            }
        }
    }

    return FBSceneContext::defaultColor;
}

// ===========================================================================
// SECTION 9: adaptSpawners()
// Pre-creates entity pools for each spawner (frozen, hidden) so that no GPU
// uploads occur at runtime. SpawnerSystem activates entities from pendingIds.
// ===========================================================================

void FBSceneAdapter::adaptSpawners(FBSceneContext& ctx) const
{
    const auto* spawnTypesVec = m_scene->spawners_type();
    const auto* spawnersVec   = m_scene->spawners();
    const auto  count         = spawnersVec->size();

    // Seeded once per adaptSpawners call for radius/size randomisation
    std::mt19937 rng(std::random_device{}());
    auto randFloat = [&](float lo, float hi) -> float {
        if (lo >= hi) return lo;
        return lo + std::uniform_real_distribution<float>(0.0f, 1.0f)(rng) * (hi - lo);
    };

    const float PI = glm::pi<float>();

    for (flatbuffers::uoffset_t i = 0; i < count; ++i) {
        const Simulation::BaseSpawner* base  = nullptr;
        float radiusMin = 0.5f, radiusMax = 0.5f;
        float heightMin = 1.0f, heightMax = 1.0f;
        glm::vec3 sizeMin{ 1.0f }, sizeMax{ 1.0f };
        enum class SpawnShape { Sphere, Cylinder, Capsule, Cuboid } spawnShape{};

        switch ((*spawnTypesVec)[i]) {
        case Simulation::SpawnerType::SphereSpawner: {
            const auto* s = spawnersVec->GetAs<Simulation::SphereSpawner>(i);
            if (!s) continue;
            base = s->base();
            if (s->radius_range()) { radiusMin = s->radius_range()->min(); radiusMax = s->radius_range()->max(); }
            spawnShape = SpawnShape::Sphere;
            break;
        }
        case Simulation::SpawnerType::CylinderSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CylinderSpawner>(i);
            if (!c) continue;
            base = c->base();
            if (c->radius_range()) { radiusMin = c->radius_range()->min(); radiusMax = c->radius_range()->max(); }
            if (c->height_range()) { heightMin = c->height_range()->min(); heightMax = c->height_range()->max(); }
            spawnShape = SpawnShape::Cylinder;
            break;
        }
        case Simulation::SpawnerType::CapsuleSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CapsuleSpawner>(i);
            if (!c) continue;
            base = c->base();
            if (c->radius_range()) { radiusMin = c->radius_range()->min(); radiusMax = c->radius_range()->max(); }
            if (c->height_range()) { heightMin = c->height_range()->min(); heightMax = c->height_range()->max(); }
            spawnShape = SpawnShape::Capsule;
            break;
        }
        case Simulation::SpawnerType::CuboidSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CuboidSpawner>(i);
            if (!c) continue;
            base = c->base();
            if (c->size_range()) {
                sizeMin = toVec3(c->size_range()->min());
                sizeMax = toVec3(c->size_range()->max());
            }
            spawnShape = SpawnShape::Cuboid;
            break;
        }
        default:
            continue;
        }

        if (base == nullptr) continue;

        SpawnerRecord rec;
        rec.name      = (base->name() != nullptr) ? base->name()->str() : "spawner";
        rec.startTime = base->start_time();

        // --- SpawnType ---
        if (base->spawn_type_type() == Simulation::SpawnType::SingleBurstSpawn) {
            const auto* bs = base->spawn_type_as_SingleBurstSpawn();
            rec.isBurst  = true;
            rec.maxCount = (bs != nullptr) ? bs->count() : 1u;
        } else if (base->spawn_type_type() == Simulation::SpawnType::RepeatingSpawn) {
            const auto* rs = base->spawn_type_as_RepeatingSpawn();
            rec.isBurst  = false;
            rec.interval = (rs != nullptr) ? rs->interval() : 1.0f;
            rec.maxCount = (rs != nullptr) ? rs->max_count() : 1u;
        }

        // --- SpawnLocation ---
        if (base->location_type() == Simulation::SpawnLocation::FixedLocation) {
            rec.locationType = GE::Components::SpawnLocType::FIXED;
            const auto* fl = base->location_as_FixedLocation();
            if (fl && fl->transform()) {
                rec.fixedPos = toVec3(fl->transform()->position());
            }
        } else if (base->location_type() == Simulation::SpawnLocation::RandomBox) {
            rec.locationType = GE::Components::SpawnLocType::RANDOM_BOX;
            const auto* rb = base->location_as_RandomBox();
            if (rb) {
                if (rb->min()) rec.boxMin = toVec3(*rb->min());
                if (rb->max()) rec.boxMax = toVec3(*rb->max());
            }
        } else if (base->location_type() == Simulation::SpawnLocation::RandomSphere) {
            rec.locationType = GE::Components::SpawnLocType::RANDOM_SPHERE;
            const auto* rs = base->location_as_RandomSphere();
            if (rs) {
                if (rs->center()) rec.sphereCenter = toVec3(*rs->center());
                rec.sphereRadius = rs->radius();
            }
        }

        // --- Velocity ranges ---
        if (base->linear_velocity()) {
            rec.linVelMin = toVec3(base->linear_velocity()->min());
            rec.linVelMax = toVec3(base->linear_velocity()->max());
        }
        if (base->angular_velocity()) {
            rec.angVelMin = toVec3(base->angular_velocity()->min());
            rec.angVelMax = toVec3(base->angular_velocity()->max());
        }

        // --- Density lookup for mass pre-computation ---
        float density = 1.0f;
        if (base->material() != nullptr) {
            const std::string matName = base->material()->str();
            for (const auto& pmr : ctx.physicsMaterials) {
                if (pmr.name == matName) { density = pmr.density; break; }
            }
        }

        // --- Owner cycling ---
        const Simulation::SpawnerOwnerType ownerType = base->owner();
        const bool isSequential = (ownerType == Simulation::SpawnerOwnerType::SEQUENTIAL);

        // --- Pre-create entity pool ---
        if (ctx.pipelines == nullptr || ctx.pipelines->size() <= FLATCOLOR_PIPELINE_INDEX) {
            GE_LOG_ERROR("FBSceneAdapter::adaptSpawners: flat-color pipeline not available, skipping spawner.");
            continue;
        }
        GE::Graphics::GraphicsPipeline* const flatColorPipeline =
            (*ctx.pipelines)[FLATCOLOR_PIPELINE_INDEX].get();

        for (uint32_t e = 0; e < rec.maxCount; ++e) {
            // Determine owner and color
            GE::Components::OwnerType owner;
            if (isSequential) {
                owner = static_cast<GE::Components::OwnerType>(e % 4);
            } else {
                owner = static_cast<GE::Components::OwnerType>(
                    static_cast<int8_t>(ownerType));
            }
            const auto ownerIdx = static_cast<std::size_t>(static_cast<int>(owner));
            const glm::vec3 color = (ctx.useOwnerColors && ownerIdx < FBSceneContext::ownerColors.size())
                ? FBSceneContext::ownerColors[ownerIdx]
                : FBSceneContext::defaultColor;

            // Sample shape parameters
            const float r  = randFloat(radiusMin, radiusMax);
            const float ht = randFloat(heightMin, heightMax);
            const glm::vec3 sz{
                randFloat(sizeMin.x, sizeMax.x),
                randFloat(sizeMin.y, sizeMax.y),
                randFloat(sizeMin.z, sizeMax.z)
            };

            const GE::ECS::EntityID id = ctx.em->CreateEntity();

            // Transform — hidden far below world until activated
            GE::Components::Transform tr;
            tr.m_position = glm::vec3{ 0.0f, -1.0e6f, 0.0f };
            tr.m_scale    = glm::vec3{ 1.0f };
            ctx.em->AddComponent(id, tr);

            // Tag
            const std::string entName = rec.name + "_pool_" + std::to_string(e);
            ctx.em->AddComponent(id, GE::Components::Tag{ entName });
            ctx.scene->addEntity(entName, id);

            // PhysicsMaterialTag
            if (base->material() != nullptr) {
                const std::string matName = base->material()->str();
                for (const auto& pmr : ctx.physicsMaterials) {
                    if (pmr.name == matName) {
                        ctx.em->AddComponent(id, GE::Components::PhysicsMaterialTag{ pmr.name, pmr.density });
                        break;
                    }
                }
            }

            // Mesh + Collider + RigidBody (shape-dependent)
            using namespace GE::Assets;
            OBJLoader::MeshData meshData;
            float mass = 0.0f;
            glm::mat3 invI = glm::mat3(0.0f);

            switch (spawnShape) {
            case SpawnShape::Sphere: {
                meshData = GeometryUtils::generateSphere(32, r, -r, color);
                ctx.em->AddComponent(id, GE::Components::SphereCollider{ r });
                mass = density * (4.0f / 3.0f) * PI * r * r * r;
                if (mass > 0.0f) invI = glm::mat3(5.0f / (2.0f * mass * r * r));
                break;
            }
            case SpawnShape::Cylinder: {
                meshData = GeometryUtils::generateCylinder(32, r, r, ht, color, true, true);
                ctx.em->AddComponent(id, GE::Components::CylinderCollider{ r, ht });
                mass = density * PI * r * r * ht;
                if (mass > 0.0f) {
                    invI[0][0] = 12.0f / (mass * (3.0f * r * r + ht * ht));
                    invI[1][1] = 2.0f  / (mass * r * r);
                    invI[2][2] = invI[0][0];
                }
                break;
            }
            case SpawnShape::Capsule: {
                meshData = GeometryUtils::generateCapsule(r, ht, 32, 16);
                for (auto& v : meshData.vertices) { v.color = color; }
                ctx.em->AddComponent(id, GE::Components::CapsuleCollider{ r, ht });
                const float mc = density * PI * r * r * ht;
                const float ms = density * (4.0f / 3.0f) * PI * r * r * r;
                mass = mc + ms;
                if (mass > 0.0f) {
                    const float Is   = (2.0f / 5.0f) * ms * r * r;
                    const float Icxz = mc * (3.0f * r * r + ht * ht) / 12.0f;
                    const float Icy  = mc * r * r * 0.5f;
                    invI[0][0] = 1.0f / (Icxz + Is);
                    invI[1][1] = 1.0f / (Icy  + Is);
                    invI[2][2] = invI[0][0];
                }
                break;
            }
            case SpawnShape::Cuboid: {
                meshData = GeometryUtils::generateBox(sz.x, sz.y, sz.z, color);
                ctx.em->AddComponent(id, GE::Components::BoxCollider{ sz.x, sz.y, sz.z });
                mass = density * sz.x * sz.y * sz.z;
                if (mass > 0.0f) {
                    invI[0][0] = 12.0f / (mass * (sz.y * sz.y + sz.z * sz.z));
                    invI[1][1] = 12.0f / (mass * (sz.x * sz.x + sz.z * sz.z));
                    invI[2][2] = 12.0f / (mass * (sz.x * sz.x + sz.y * sz.y));
                }
                break;
            }
            }

            // Upload mesh to GPU
            auto flatMat = std::make_shared<GE::Assets::Material>(VK_NULL_HANDLE, flatColorPipeline);
            flatMat->SetCastsShadows(false);
            auto meshPtr = ctx.am->processMeshData(
                meshData, flatMat,
                ctx.uploadCtx->cmd,
                ctx.uploadCtx->stagingBuffers,
                ctx.uploadCtx->stagingMemories);
            if (meshPtr) {
                GE::Assets::Mesh* const rawPtr = meshPtr.get();
                auto dummyModel = std::make_unique<GE::Assets::Model>();
                dummyModel->addMesh(std::move(meshPtr));
                GE::Components::MeshRenderer mr;
                mr.subMeshes.push_back({ rawPtr, flatMat.get() });
                ctx.em->AddComponent(id, mr);
                ctx.ownedModels->push_back(std::move(dummyModel));
            } else {
                GE_LOG_ERROR("FBSceneAdapter::adaptSpawners: processMeshData failed for pool entity.");
            }

            // RigidBody — frozen until SpawnerSystem activates it
            GE::Components::RigidBody rb;
            rb.isStatic         = true;
            rb.useGravity       = false;
            rb.mass             = mass;
            rb.inverseMass      = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
            rb.invInertiaTensor = invI;
            ctx.em->AddComponent(id, rb);

            // OwnerComponent
            ctx.em->AddComponent(id, GE::Components::OwnerComponent{ owner });

            rec.entityIds.push_back(id);
        }

        if (!rec.entityIds.empty()) {
            ctx.spawners.push_back(std::move(rec));
        }
    }
}

} // namespace GE::Scene::FB
