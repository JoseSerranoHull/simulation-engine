/* parasoft-begin-suppress ALL */
#include <fstream>
#include <string>
/* parasoft-end-suppress ALL */

#include "scene/fb/FBSceneAdapter.h"
#include "Scene_generated.h"        // flatc-generated from flatbuffers/Scene.fbs

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
    if (m_scene->cameras()   != nullptr) { adaptCameras(ctx);   }
    if (m_scene->materials() != nullptr) { adaptMaterials(ctx); }
    if (m_scene->objects()   != nullptr) { adaptObjects(ctx);   }
    // Spawners: deferred to a future iteration
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
// SECTION 5: adaptObjects()
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
        rb.isStatic   = false;
        rb.useGravity = true;

        if (sim != nullptr && sim->initial_state() != nullptr) {
            const auto* ps = sim->initial_state();
            rb.velocity         = toVec3(ps->linear_velocity());
            rb.angularVelocity  = glm::radians(toVec3(ps->angular_velocity()));
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

} // namespace GE::Scene::FB
