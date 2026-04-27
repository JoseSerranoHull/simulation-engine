/* parasoft-begin-suppress ALL */
#include <fstream>
#include <random>
#include <string>
#include <unordered_map>
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
#include "components/ClothComponent.h"
#include "components/FlockingComponent.h"
#include "scene/Scene.h"
#include "assets/AssetManager.h"
#include "assets/GeometryUtils.h"
#include "assets/Model.h"
#include "assets/Material.h"
#include "graphics/GraphicsPipeline.h"
#include "graphics/GpuUploadContext.h"
#include "graphics/VulkanUtils.h"
#include "assets/Vertex.h"
#include "core/Logger.h"
#include "components/ScriptComponent.h"
#include "scripts/ScriptFactory.h"

// Pipeline indices within m_pipelines (see Scenario::createMaterialPipelines())
static constexpr std::size_t PHONG_PIPELINE_INDEX     = 0U;   // phong.vert + phong.frag
static constexpr std::size_t FLATCOLOR_PIPELINE_INDEX = 8U;   // flat vertex-color, no descriptor set

namespace GE::Scene::FB {

// ===========================================================================
// Internal helpers
// ===========================================================================

static glm::vec3 toVec3(const Simulation::Vec3& v) {
    return { v.x(), v.y(), v.z() };
}

// Creates a host-visible, host-coherent VkBuffer of the given size and persistently maps it.
// Returns false on failure. Caller owns cleanup (vkUnmapMemory, vkDestroyBuffer, vkFreeMemory).
static bool createHostVisibleBuffer(VkDevice device,
                                    VkPhysicalDevice physDevice,
                                    VkDeviceSize size,
                                    VkBufferUsageFlags usage,
                                    VkBuffer& outBuffer,
                                    VkDeviceMemory& outMemory,
                                    void*& outMapped)
{
    GE::Graphics::VulkanUtils::createBuffer(
        device, physDevice, size, usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        outBuffer, outMemory);

    if (outBuffer == VK_NULL_HANDLE || outMemory == VK_NULL_HANDLE) {
        return false;
    }

    if (vkMapMemory(device, outMemory, 0, size, 0, &outMapped) != VK_SUCCESS) {
        outMapped = nullptr;
        return false;
    }
    return true;
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

    // Order matters: materials before objects/prefabs; prefabs before spawners
    if (m_scene->cameras()      != nullptr) { adaptCameras(ctx);      }
    if (m_scene->materials()    != nullptr) { adaptMaterials(ctx);    }
    if (m_scene->interactions() != nullptr) { adaptInteractions(ctx); }
    if (m_scene->prefabs()      != nullptr) { adaptPrefabs(ctx);      }
    if (m_scene->objects()      != nullptr) { adaptObjects(ctx);      }
    if (m_scene->objects()      != nullptr) { adaptParentLinks(ctx);  }
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
        transform.m_localPosition = toVec3(t->position());
        transform.m_localRotation = glm::vec3{ t->orientation().yaw(),
                                               t->orientation().pitch(),
                                               t->orientation().roll() };
        transform.m_localScale    = toVec3(t->scale());
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
    // ClothObject and FlockAgent build their own geometry; skip the static adaptShape() path.
    // Objects with no shape (NONE) are empty containers (e.g. GameMap) — skip silently.
    const bool hasShape     = (obj->shape_type() != Simulation::Shape::NONE);
    const bool isCloth      = (obj->behaviour_type() == Simulation::Behaviour::ClothObject);
    const bool isFlockAgent = (obj->behaviour_type() == Simulation::Behaviour::FlockAgent);
    const bool isContainer  = (obj->collision_type() == Simulation::CollisionType::CONTAINER);
    if (hasShape && !isCloth && !isFlockAgent) {
        bool usedTexturePath = false;

        if (!ctx.useOwnerColors &&
            obj->texture_path() != nullptr && !obj->texture_path()->str().empty() &&
            ctx.pipelines != nullptr && ctx.pipelines->size() > PHONG_PIPELINE_INDEX)
        {
            auto whiteTex     = ctx.am->loadTexture("textures/white.png");
            auto blackTex     = ctx.am->loadTexture("textures/black.png");
            auto flatNormalTex = ctx.am->loadTexture("textures/flat_normal.png");
            auto albedo       = ctx.am->loadTexture(obj->texture_path()->str());
            if (albedo && whiteTex && blackTex && flatNormalTex) {
                auto normalTex = (obj->normal_map_path() && !obj->normal_map_path()->str().empty())
                                 ? ctx.am->loadTexture(obj->normal_map_path()->str()) : flatNormalTex;
                auto aoTex    = (obj->ao_path()        && !obj->ao_path()->str().empty())
                                ? ctx.am->loadTexture(obj->ao_path()->str())        : whiteTex;
                auto roughTex = (obj->roughness_path() && !obj->roughness_path()->str().empty())
                                ? ctx.am->loadTexture(obj->roughness_path()->str()) : whiteTex;
                auto metalTex = (obj->metallic_path()  && !obj->metallic_path()->str().empty())
                                ? ctx.am->loadTexture(obj->metallic_path()->str())  : blackTex;

                auto phongMat = ctx.am->createMaterial(
                    albedo, normalTex, aoTex, metalTex, roughTex,
                    (*ctx.pipelines)[PHONG_PIPELINE_INDEX].get());

                if (phongMat) {
                    phongMat->SetCastsShadows(true);
                    using namespace GE::Assets;
                    OBJLoader::MeshData meshData;
                    bool shapeBuilt = false;

                    switch (obj->shape_type()) {
                    case Simulation::Shape::Sphere: {
                        const auto* s = obj->shape_as_Sphere();
                        const float r = s ? s->radius() : 0.5f;
                        meshData = GeometryUtils::generateSphere(32, r, -r, glm::vec3(1.0f));
                        ctx.em->AddComponent(id, GE::Components::SphereCollider{ r });
                        shapeBuilt = true;
                        break;
                    }
                    case Simulation::Shape::Plane: {
                        const auto* p = obj->shape_as_Plane();
                        const float planeW = p ? p->width()  : 20.0f;
                        const float planeD = p ? p->depth()  : 20.0f;
                        meshData = GeometryUtils::generatePlane(planeW, planeD);
                        for (auto& v : meshData.vertices) { v.color = glm::vec3(1.0f); }
                        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
                        if (p && p->normal()) { normal = toVec3(*p->normal()); }
                        GE::Components::PlaneCollider pc{ normal, 0.0f };
                        if (glm::abs(normal.y) > 0.9f) { pc.sizeX = planeW; pc.sizeZ = planeD; }
                        ctx.em->AddComponent(id, pc);
                        shapeBuilt = true;
                        break;
                    }
                    case Simulation::Shape::Cylinder: {
                        const auto* c = obj->shape_as_Cylinder();
                        const float r  = c ? c->radius() : 0.5f;
                        const float ht = c ? c->height()  : 1.0f;
                        meshData = GeometryUtils::generateCylinder(32, r, r, ht, glm::vec3(1.0f), true, true);
                        ctx.em->AddComponent(id, GE::Components::CylinderCollider{ r, ht });
                        shapeBuilt = true;
                        break;
                    }
                    case Simulation::Shape::Capsule: {
                        const auto* c = obj->shape_as_Capsule();
                        const float r  = c ? c->radius() : 0.5f;
                        const float ht = c ? c->height()  : 1.0f;
                        meshData = GeometryUtils::generateCapsule(r, ht, 32, 16);
                        for (auto& v : meshData.vertices) { v.color = glm::vec3(1.0f); }
                        ctx.em->AddComponent(id, GE::Components::CapsuleCollider{ r, ht });
                        shapeBuilt = true;
                        break;
                    }
                    case Simulation::Shape::Cuboid: {
                        const auto* c = obj->shape_as_Cuboid();
                        const glm::vec3 sz = (c && c->size()) ? toVec3(*c->size()) : glm::vec3(1.0f);
                        meshData = GeometryUtils::generateBox(sz.x, sz.y, sz.z, glm::vec3(1.0f));
                        ctx.em->AddComponent(id, GE::Components::BoxCollider{ sz.x, sz.y, sz.z });
                        shapeBuilt = true;
                        break;
                    }
                    default:
                        break;
                    }

                    if (shapeBuilt && !meshData.vertices.empty()) {
                        auto meshPtr = ctx.am->processMeshData(
                            meshData, phongMat,
                            ctx.uploadCtx->cmd,
                            ctx.uploadCtx->stagingBuffers,
                            ctx.uploadCtx->stagingMemories);
                        if (meshPtr) {
                            GE::Assets::Mesh* const rawPtr = meshPtr.get();
                            auto dummyModel = std::make_unique<GE::Assets::Model>();
                            dummyModel->addMesh(std::move(meshPtr));
                            GE::Components::MeshRenderer mr;
                            mr.subMeshes.push_back({ rawPtr, phongMat.get() });
                            ctx.em->AddComponent(id, mr);
                            ctx.ownedModels->push_back(std::move(dummyModel));
                            usedTexturePath = true;
                        }
                    }
                }
            }
        }

        if (!usedTexturePath) {
            adaptShape(obj, id, color, isContainer, ctx);
        }
    }

    // --- Behaviour → RigidBody / AnimatedObjectComponent / ClothComponent ---
    adaptBehaviour(obj, id, ctx);

    // --- Script attachment (data-driven via script_type field) ---
    if (obj->script_type() != nullptr) {
        const std::string typeName = obj->script_type()->str();
        auto script = GE::Scripts::CreateScript(typeName);
        if (script != nullptr) {
            script->SetEntityID(id);
            ctx.em->AddComponent(id, GE::Components::ScriptComponent{ std::move(script) });
        }
    }
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
        const float planeW = (p != nullptr) ? p->width() : 20.0f;
        const float planeD = (p != nullptr) ? p->depth() : 20.0f;
        meshData = GeometryUtils::generatePlane(planeW, planeD);
        for (auto& v : meshData.vertices) { v.color = color; }
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        if (p != nullptr && p->normal() != nullptr) { normal = toVec3(*p->normal()); }
        GE::Components::PlaneCollider pc{ normal, 0.0f };
        // Upward/downward-facing planes (floors, ceilings) are bounded by their visual extent.
        // Vertical planes (walls) remain infinite so they stop objects at any height/position.
        if (glm::abs(normal.y) > 0.9f) {
            pc.sizeX = planeW;
            pc.sizeZ = planeD;
        }
        ctx.em->AddComponent(id, pc);
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
                transform->m_localPosition = ac.waypoints[0].position;
            }
        }

        ctx.em->AddComponent(id, ac);
        break;
    }
    case Simulation::Behaviour::ClothObject: {
        const auto* cloth = obj->behaviour_as_ClothObject();

        GE::Components::ClothComponent cc;
        cc.rows         = (cloth != nullptr) ? cloth->rows()          : 10;
        cc.cols         = (cloth != nullptr) ? cloth->cols()          : 10;
        cc.cellSize     = (cloth != nullptr) ? cloth->cell_size()     : 0.2f;
        cc.springK      = (cloth != nullptr) ? cloth->spring_k()      : 100.0f;
        cc.shearK       = (cloth != nullptr) ? cloth->shear_k()       : 50.0f;
        cc.flexionK     = (cloth != nullptr) ? cloth->flexion_k()     : 25.0f;
        cc.damping      = (cloth != nullptr) ? cloth->damping()       : 0.1f;
        cc.particleMass = (cloth != nullptr) ? cloth->particle_mass() : 0.1f;
        const bool pinTop = (cloth != nullptr) ? cloth->pin_top_edge() : true;

        // Get the entity's world position from its Transform
        const GE::Components::Transform* tr =
            ctx.em->GetTIComponent<GE::Components::Transform>(id);
        const glm::vec3 origin = (tr != nullptr) ? tr->m_localPosition : glm::vec3{ 0.0f };

        // Initialise particle flat grid
        cc.particles.resize(static_cast<std::size_t>(cc.rows * cc.cols));
        for (int r = 0; r < cc.rows; ++r) {
            for (int c = 0; c < cc.cols; ++c) {
                const int     idx = r * cc.cols + c;
                const glm::vec3 pos = origin + glm::vec3{
                    c * cc.cellSize, 0.0f, r * cc.cellSize };
                cc.particles[idx].position     = pos;
                cc.particles[idx].prevPosition = pos;
                cc.particles[idx].pinned       = (pinTop && r == 0);
            }
        }

        // Default burn center: lower-middle of the hanging cloth.
        // X/Z = centre of the grid span; Y estimated as bottom of fully-hung cloth.
        cc.burnCenter = {
            origin.x + (cc.cols - 1) * 0.5f * cc.cellSize,
            origin.y - (cc.rows - 1) * cc.cellSize,
            origin.z + (cc.rows - 1) * 0.5f * cc.cellSize
        };

        // Resolve owner color
        cc.color = resolveColor(obj, ctx);

        // Build initial vertex data (flat grid)
        const uint32_t vCount = static_cast<uint32_t>(cc.rows * cc.cols);
        std::vector<GE::Assets::Vertex> verts(vCount);
        for (int r = 0; r < cc.rows; ++r) {
            for (int c = 0; c < cc.cols; ++c) {
                const int idx = r * cc.cols + c;
                GE::Assets::Vertex& v = verts[idx];
                v.position = cc.particles[idx].position;
                v.color    = cc.color;
                v.texcoord = glm::vec2{
                    static_cast<float>(c) / static_cast<float>(cc.cols - 1),
                    static_cast<float>(r) / static_cast<float>(cc.rows - 1) };
                v.normal   = glm::vec3{ 0.0f, 1.0f, 0.0f };
            }
        }

        // Build index data (two triangles per quad)
        std::vector<uint32_t> indices;
        indices.reserve(static_cast<std::size_t>((cc.rows - 1) * (cc.cols - 1) * 6));
        for (int r = 0; r < cc.rows - 1; ++r) {
            for (int c = 0; c < cc.cols - 1; ++c) {
                const uint32_t tl = static_cast<uint32_t>(r * cc.cols + c);
                const uint32_t tr_ = tl + 1U;
                const uint32_t bl = static_cast<uint32_t>((r + 1) * cc.cols + c);
                const uint32_t br = bl + 1U;
                // Triangle 1: tl, bl, tr
                indices.push_back(tl);
                indices.push_back(bl);
                indices.push_back(tr_);
                // Triangle 2: tr, bl, br
                indices.push_back(tr_);
                indices.push_back(bl);
                indices.push_back(br);
            }
        }
        cc.vertexCount = vCount;
        cc.indexCount  = static_cast<uint32_t>(indices.size());

        // Allocate combined host-visible buffer: [vertices][indices]
        const VkDeviceSize vertexBytes = static_cast<VkDeviceSize>(vCount) * sizeof(GE::Assets::Vertex);
        const VkDeviceSize indexBytes  = static_cast<VkDeviceSize>(cc.indexCount) * sizeof(uint32_t);
        cc.indexOffset = vertexBytes;
        const VkDeviceSize totalBytes  = vertexBytes + indexBytes;

        GE::Graphics::VulkanContext* vkCtx = ServiceLocator::GetContext();
        if (vkCtx == nullptr || vkCtx->device == VK_NULL_HANDLE) {
            GE_LOG_ERROR("FBSceneAdapter: ClothObject: VulkanContext unavailable.");
            break;
        }

        if (!createHostVisibleBuffer(
                vkCtx->device, vkCtx->physicalDevice,
                totalBytes,
                VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                cc.vertexBuffer, cc.vertexMemory, cc.mappedVertices))
        {
            GE_LOG_ERROR("FBSceneAdapter: ClothObject: Failed to create host-visible buffer.");
            break;
        }

        // Write initial vertices + indices into mapped buffer
        std::memcpy(cc.mappedVertices, verts.data(), vertexBytes);
        std::memcpy(static_cast<uint8_t*>(cc.mappedVertices) + vertexBytes,
                    indices.data(), indexBytes);

        // Create a Mesh wrapping the cloth buffer — non-owning reference, cloth owns cleanup.
        if (ctx.pipelines == nullptr || ctx.pipelines->size() <= FLATCOLOR_PIPELINE_INDEX) {
            GE_LOG_ERROR("FBSceneAdapter: ClothObject: Flat-color pipeline not available.");
            break;
        }
        GE::Graphics::GraphicsPipeline* const flatColorPipeline =
            (*ctx.pipelines)[FLATCOLOR_PIPELINE_INDEX].get();
        auto flatMat = std::make_shared<GE::Assets::Material>(VK_NULL_HANDLE, flatColorPipeline);
        flatMat->SetCastsShadows(false);

        auto meshPtr = std::make_unique<GE::Assets::Mesh>(
            cc.vertexBuffer,
            cc.indexCount,
            cc.indexOffset,
            flatMat);

        GE::Assets::Mesh* const rawMesh = meshPtr.get();
        auto dummyModel = std::make_unique<GE::Assets::Model>();
        dummyModel->addMesh(std::move(meshPtr));

        GE::Components::MeshRenderer mr;
        mr.subMeshes.push_back({ rawMesh, flatMat.get() });
        ctx.em->AddComponent(id, mr);
        ctx.ownedModels->push_back(std::move(dummyModel));

        // Build explicit spring list used by ClothSystem for tearing support.
        // kSpring baked at load time; runtime slider changes affect new loads only.
        if (cc.rows * cc.cols > 65535) {
            GE_LOG_ERROR("FBSceneAdapter: ClothObject: rows*cols > 65535 — uint16_t indices overflow.");
        } else {
            const float restStruct = cc.cellSize;
            const float restShear  = cc.cellSize * 1.41421356f; // √2
            const float restFlex   = cc.cellSize * 2.0f;
            const int R = cc.rows, C = cc.cols;

            // Structural: horizontal + vertical neighbours
            for (int r = 0; r < R; ++r) {
                for (int c = 0; c < C; ++c) {
                    if (c + 1 < C)
                        cc.springs.push_back({ static_cast<uint16_t>(r*C+c), static_cast<uint16_t>(r*C+c+1), restStruct, cc.springK, true });
                    if (r + 1 < R)
                        cc.springs.push_back({ static_cast<uint16_t>(r*C+c), static_cast<uint16_t>((r+1)*C+c), restStruct, cc.springK, true });
                }
            }
            // Shear: diagonal neighbours
            for (int r = 0; r < R-1; ++r) {
                for (int c = 0; c < C-1; ++c) {
                    cc.springs.push_back({ static_cast<uint16_t>(r*C+c),   static_cast<uint16_t>((r+1)*C+c+1), restShear, cc.shearK, true });
                    cc.springs.push_back({ static_cast<uint16_t>(r*C+c+1), static_cast<uint16_t>((r+1)*C+c),   restShear, cc.shearK, true });
                }
            }
            // Flexion: skip-one horizontal + vertical
            for (int r = 0; r < R; ++r)
                for (int c = 0; c < C-2; ++c)
                    cc.springs.push_back({ static_cast<uint16_t>(r*C+c), static_cast<uint16_t>(r*C+c+2), restFlex, cc.flexionK, true });
            for (int r = 0; r < R-2; ++r)
                for (int c = 0; c < C; ++c)
                    cc.springs.push_back({ static_cast<uint16_t>(r*C+c), static_cast<uint16_t>((r+2)*C+c), restFlex, cc.flexionK, true });
        }

        ctx.em->AddComponent(id, cc);
        break;
    }
    case Simulation::Behaviour::FlockAgent: {
        // FlockAgent spawns N agent entities around this object's transform position.
        // The host object itself is not used as a physics body — it's just the spawn anchor.
        const auto* fa = obj->behaviour_as_FlockAgent();

        const int   agentCount       = (fa != nullptr) ? fa->agent_count()       : 60;
        const float sphereRadius     = (fa != nullptr) ? fa->sphere_radius()     : 0.3f;
        const float separationRadius = (fa != nullptr) ? fa->separation_radius() : 1.5f;
        const float alignmentRadius  = (fa != nullptr) ? fa->alignment_radius()  : 3.0f;
        const float cohesionRadius   = (fa != nullptr) ? fa->cohesion_radius()   : 5.0f;
        const float wSeparation      = (fa != nullptr) ? fa->w_separation()      : 2.0f;
        const float wAlignment       = (fa != nullptr) ? fa->w_alignment()       : 1.0f;
        const float wCohesion        = (fa != nullptr) ? fa->w_cohesion()        : 1.0f;
        const float maxSpeed         = (fa != nullptr) ? fa->max_speed()         : 6.0f;
        const float maxForce         = (fa != nullptr) ? fa->max_force()         : 15.0f;
        const float spawnRadius      = (fa != nullptr) ? fa->spawn_radius()      : 5.0f;

        // Get spawn origin from the anchor object's Transform
        const GE::Components::Transform* anchor =
            ctx.em->GetTIComponent<GE::Components::Transform>(id);
        const glm::vec3 origin = (anchor != nullptr) ? anchor->m_localPosition : glm::vec3{ 0.0f };

        // Store spawn config so FlockingSystem::Restart() can rescatter agents correctly
        ctx.flockSpawnOrigin = origin;
        ctx.flockSpawnRadius = spawnRadius;

        // Prepare flat-color material
        if (ctx.pipelines == nullptr || ctx.pipelines->size() <= FLATCOLOR_PIPELINE_INDEX) {
            GE_LOG_ERROR("FBSceneAdapter: FlockAgent: Flat-color pipeline not available.");
            break;
        }
        GE::Graphics::GraphicsPipeline* const flatColorPipeline =
            (*ctx.pipelines)[FLATCOLOR_PIPELINE_INDEX].get();

        // Seeded RNG for scatter positions
        std::mt19937 rng(0xF10C1337u);
        std::uniform_real_distribution<float> distUnit(-1.0f, 1.0f);

        const float PI = glm::pi<float>();
        // Agent mass from sphere volume with density 1.0 (flock agents are lightweight)
        const float density = 1.0f;
        const float mass = density * (4.0f / 3.0f) * PI * sphereRadius * sphereRadius * sphereRadius;
        const float invI_val = (mass > 0.0f) ? (5.0f / (2.0f * mass * sphereRadius * sphereRadius)) : 0.0f;

        for (int i = 0; i < agentCount; ++i) {
            // Random position inside a sphere via rejection sampling
            glm::vec3 offset{ 0.0f };
            for (int attempt = 0; attempt < 100; ++attempt) {
                offset = glm::vec3{ distUnit(rng), distUnit(rng), distUnit(rng) };
                if (glm::dot(offset, offset) <= 1.0f) { break; }
            }
            const glm::vec3 spawnPos = origin + offset * spawnRadius;

            const GE::ECS::EntityID agentId = ctx.em->CreateEntity();

            // Transform
            GE::Components::Transform agentTr;
            agentTr.m_localPosition = spawnPos;
            agentTr.m_localScale    = glm::vec3{ 1.0f };
            ctx.em->AddComponent(agentId, agentTr);

            // Tag
            const std::string agentName = "flock_agent_" + std::to_string(i);
            ctx.em->AddComponent(agentId, GE::Components::Tag{ agentName });
            ctx.scene->addEntity(agentName, agentId);

            // SphereCollider
            ctx.em->AddComponent(agentId, GE::Components::SphereCollider{ sphereRadius });

            // RigidBody — no gravity, flock forces drive motion
            GE::Components::RigidBody rb;
            rb.isStatic         = false;
            rb.useGravity       = false;
            rb.mass             = mass;
            rb.inverseMass      = (mass > 0.0f) ? (1.0f / mass) : 0.0f;
            rb.invInertiaTensor = glm::mat3(invI_val);
            rb.linearDamping    = 0.95f; // extra damping to prevent indefinite acceleration
            // Random initial velocity (small kick)
            rb.velocity = glm::vec3{ distUnit(rng), distUnit(rng), distUnit(rng) } * 2.0f;
            ctx.em->AddComponent(agentId, rb);

            // FlockingComponent
            GE::Components::FlockingComponent fk;
            fk.separationRadius = separationRadius;
            fk.alignmentRadius  = alignmentRadius;
            fk.cohesionRadius   = cohesionRadius;
            fk.wSeparation      = wSeparation;
            fk.wAlignment       = wAlignment;
            fk.wCohesion        = wCohesion;
            fk.maxSpeed         = maxSpeed;
            fk.maxForce         = maxForce;
            ctx.em->AddComponent(agentId, fk);

            // Mesh (sphere)
            const glm::vec3 agentColor = GE::Scene::FB::FBSceneContext::defaultColor;
            auto agentMat = std::make_shared<GE::Assets::Material>(VK_NULL_HANDLE, flatColorPipeline);
            agentMat->SetCastsShadows(false);
            GE::Assets::OBJLoader::MeshData meshData =
                GE::Assets::GeometryUtils::generateSphere(16, sphereRadius, -sphereRadius, agentColor);
            auto meshPtr = ctx.am->processMeshData(
                meshData, agentMat,
                ctx.uploadCtx->cmd,
                ctx.uploadCtx->stagingBuffers,
                ctx.uploadCtx->stagingMemories);
            if (meshPtr) {
                GE::Assets::Mesh* const rawPtr = meshPtr.get();
                auto dummyModel = std::make_unique<GE::Assets::Model>();
                dummyModel->addMesh(std::move(meshPtr));
                GE::Components::MeshRenderer mr;
                mr.subMeshes.push_back({ rawPtr, agentMat.get() });
                ctx.em->AddComponent(agentId, mr);
                ctx.ownedModels->push_back(std::move(dummyModel));
            }
        }
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
// SECTION 9: adaptPrefabs()
// Uploads GPU meshes per prefab definition and stores PrefabTemplates in ctx.prefabRegistry.
// useOwnerColors=true  → 4 owner-colored meshes (red/green/blue/yellow) per prefab.
// useOwnerColors=false → 1 material-appearance mesh (textured Phong or tinted flat-color).
// ===========================================================================

void FBSceneAdapter::adaptPrefabs(FBSceneContext& ctx) const
{
    const auto* prefabsVec = m_scene->prefabs();
    if (prefabsVec == nullptr) { return; }

    if (ctx.pipelines == nullptr || ctx.pipelines->size() <= FLATCOLOR_PIPELINE_INDEX) {
        GE_LOG_ERROR("FBSceneAdapter::adaptPrefabs: flat-color pipeline not available.");
        return;
    }
    GE::Graphics::GraphicsPipeline* const flatColorPipeline =
        (*ctx.pipelines)[FLATCOLOR_PIPELINE_INDEX].get();

    const float PI = glm::pi<float>();

    for (flatbuffers::uoffset_t i = 0; i < prefabsVec->size(); ++i) {
        const Simulation::Prefab* pb = (*prefabsVec)[i];
        if (pb == nullptr || pb->name() == nullptr) { continue; }

        const std::string prefabName = pb->name()->str();

        PrefabTemplate tmpl;
        tmpl.name = prefabName;

        // --- Material density lookup ---
        float density = 1.0f;
        float restitution = 0.6f;
        if (pb->material() != nullptr) {
            const std::string matName = pb->material()->str();
            for (const auto& pmr : ctx.physicsMaterials) {
                if (pmr.name == matName) { density = pmr.density; break; }
            }
        }
        tmpl.density     = density;
        tmpl.restitution = restitution;

        // --- Script type ---
        if (pb->script_type() != nullptr) {
            tmpl.scriptType = pb->script_type()->str();
        }

        // --- Shape parsing + mesh generation ---
        using namespace GE::Assets;
        OBJLoader::MeshData meshData;
        glm::mat3 invI = glm::mat3(0.0f);
        float mass = 0.0f;
        const glm::vec3 color = FBSceneContext::defaultColor;

        switch (pb->shape_type()) {
        case Simulation::Shape::Sphere: {
            const auto* s = pb->shape_as_Sphere();
            const float r = s ? s->radius() : 0.5f;
            tmpl.shapeKind = PrefabShapeKind::Sphere;
            tmpl.radius    = r;
            meshData = GeometryUtils::generateSphere(24, r, -r, color);
            mass = density * (4.0f / 3.0f) * PI * r * r * r;
            if (mass > 0.0f) invI = glm::mat3(5.0f / (2.0f * mass * r * r));
            break;
        }
        case Simulation::Shape::Capsule: {
            const auto* c = pb->shape_as_Capsule();
            const float r  = c ? c->radius() : 0.3f;
            const float ht = c ? c->height()  : 1.0f;
            tmpl.shapeKind = PrefabShapeKind::Capsule;
            tmpl.radius    = r;
            tmpl.height    = ht;
            meshData = GeometryUtils::generateCapsule(r, ht, 24, 12);
            for (auto& v : meshData.vertices) { v.color = color; }
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
        case Simulation::Shape::Cylinder: {
            const auto* c = pb->shape_as_Cylinder();
            const float r  = c ? c->radius() : 0.3f;
            const float ht = c ? c->height()  : 1.0f;
            tmpl.shapeKind = PrefabShapeKind::Cylinder;
            tmpl.radius    = r;
            tmpl.height    = ht;
            meshData = GeometryUtils::generateCylinder(24, r, r, ht, color, true, true);
            mass = density * PI * r * r * ht;
            if (mass > 0.0f) {
                invI[0][0] = 12.0f / (mass * (3.0f * r * r + ht * ht));
                invI[1][1] = 2.0f  / (mass * r * r);
                invI[2][2] = invI[0][0];
            }
            break;
        }
        case Simulation::Shape::Cuboid: {
            const auto* c = pb->shape_as_Cuboid();
            const glm::vec3 sz = (c && c->size()) ? toVec3(*c->size()) : glm::vec3(0.5f);
            tmpl.shapeKind = PrefabShapeKind::Cuboid;
            tmpl.size      = sz;
            meshData = GeometryUtils::generateBox(sz.x, sz.y, sz.z, color);
            mass = density * sz.x * sz.y * sz.z;
            if (mass > 0.0f) {
                invI[0][0] = 12.0f / (mass * (sz.y * sz.y + sz.z * sz.z));
                invI[1][1] = 12.0f / (mass * (sz.x * sz.x + sz.z * sz.z));
                invI[2][2] = 12.0f / (mass * (sz.x * sz.x + sz.y * sz.y));
            }
            break;
        }
        default:
            GE_LOG_WARN("FBSceneAdapter::adaptPrefabs: unsupported shape in prefab '" + prefabName + "', skipping.");
            continue;
        }

        tmpl.mass        = mass;
        tmpl.invInertia  = invI;

        // Helper: upload a mesh variant and push the Model into ownedModels.
        auto uploadVariant = [&](OBJLoader::MeshData data,
                                 std::shared_ptr<GE::Assets::Material> mat) -> GE::Assets::Mesh* {
            auto meshPtr = ctx.am->processMeshData(
                data, mat,
                ctx.uploadCtx->cmd,
                ctx.uploadCtx->stagingBuffers,
                ctx.uploadCtx->stagingMemories);
            if (!meshPtr) { return nullptr; }
            GE::Assets::Mesh* raw = meshPtr.get();
            auto mdl = std::make_unique<GE::Assets::Model>();
            mdl->addMesh(std::move(meshPtr));
            ctx.ownedModels->push_back(std::move(mdl));
            return raw;
        };

        // Shared flat-color material (no descriptor set, no textures — just the pipeline)
        auto flatMat = std::make_shared<GE::Assets::Material>(VK_NULL_HANDLE, flatColorPipeline);
        flatMat->SetCastsShadows(false);

        if (ctx.useOwnerColors) {
            // --- Owner-color mode: upload 4 meshes, one per peer color ---
            bool anyFailed = false;
            for (std::size_t ownerIdx = 0; ownerIdx < 4; ++ownerIdx) {
                OBJLoader::MeshData coloredData = meshData;
                const glm::vec3 c = FBSceneContext::ownerColors[ownerIdx];
                for (auto& v : coloredData.vertices) { v.color = c; }
                GE::Assets::Mesh* m = uploadVariant(coloredData, flatMat);
                if (m == nullptr) { anyFailed = true; break; }
                tmpl.ownerMeshes[ownerIdx] = m;
            }
            if (anyFailed) {
                GE_LOG_ERROR("FBSceneAdapter::adaptPrefabs: owner mesh upload failed for '" + prefabName + "'.");
                continue;
            }
        } else {
            // --- Material-color mode: one mesh reflecting the material's appearance ---

            // Helper: upload the material-tinted flat-color fallback mesh.
            auto uploadMaterialTint = [&]() -> bool {
                static const std::unordered_map<std::string, glm::vec3> s_tints = {
                    { "rubber",   { 0.45f, 0.22f, 0.08f } },
                    { "steel",    { 0.70f, 0.70f, 0.78f } },
                    { "concrete", { 0.52f, 0.50f, 0.47f } },
                    { "plastic",  { 0.25f, 0.55f, 0.80f } },
                };
                const std::string matName = pb->material() ? pb->material()->str() : "";
                const auto it  = s_tints.find(matName);
                const glm::vec3 tint = (it != s_tints.end()) ? it->second : FBSceneContext::defaultColor;
                OBJLoader::MeshData tintedData = meshData;
                for (auto& v : tintedData.vertices) { v.color = tint; }
                GE::Assets::Mesh* m = uploadVariant(tintedData, flatMat);
                if (!m) { return false; }
                tmpl.materialMesh = m;
                return true;
            };

            // Textured Phong path (only when texture_path is present and loadable).
            bool textureSucceeded = false;
            if (pb->texture_path() != nullptr) {
                const std::string texPath = pb->texture_path()->str();
                auto albedo        = ctx.am->loadTexture(texPath);
                auto whiteTex      = ctx.am->loadTexture("textures/white.png");
                auto blackTex      = ctx.am->loadTexture("textures/black.png");
                auto flatNormalTex = ctx.am->loadTexture("textures/flat_normal.png");
                if (albedo && whiteTex && blackTex && flatNormalTex) {
                    auto normalTex = (pb->normal_map_path() && !pb->normal_map_path()->str().empty())
                                     ? ctx.am->loadTexture(pb->normal_map_path()->str()) : flatNormalTex;
                    auto aoTex    = (pb->ao_path()        && !pb->ao_path()->str().empty())
                                    ? ctx.am->loadTexture(pb->ao_path()->str())        : whiteTex;
                    auto roughTex = (pb->roughness_path() && !pb->roughness_path()->str().empty())
                                    ? ctx.am->loadTexture(pb->roughness_path()->str()) : whiteTex;
                    auto metalTex = (pb->metallic_path()  && !pb->metallic_path()->str().empty())
                                    ? ctx.am->loadTexture(pb->metallic_path()->str())  : blackTex;
                    auto phongMat = ctx.am->createMaterial(
                        albedo, normalTex, aoTex, metalTex, roughTex,
                        (*ctx.pipelines)[PHONG_PIPELINE_INDEX].get());
                    phongMat->SetCastsShadows(false);
                    OBJLoader::MeshData neutralData = meshData;
                    for (auto& v : neutralData.vertices) { v.color = glm::vec3(1.0f); }
                    GE::Assets::Mesh* m = uploadVariant(neutralData, phongMat);
                    if (m) {
                        tmpl.materialMesh   = m;
                        tmpl.materialMatPtr = phongMat;
                        textureSucceeded    = true;
                    } else {
                        GE_LOG_WARN("FBSceneAdapter::adaptPrefabs: textured upload failed for '" + prefabName + "', using tint fallback.");
                    }
                } else {
                    GE_LOG_WARN("FBSceneAdapter::adaptPrefabs: texture files missing for '" + prefabName + "', using tint fallback.");
                }
            }

            if (!textureSucceeded) {
                if (!uploadMaterialTint()) {
                    GE_LOG_ERROR("FBSceneAdapter::adaptPrefabs: material mesh upload failed for '" + prefabName + "'.");
                    continue;
                }
            }
        }

        ctx.prefabRegistry[prefabName] = std::move(tmpl);
        GE_LOG_INFO("FBSceneAdapter::adaptPrefabs: registered prefab '" + prefabName + "'.");
    }
}

// ===========================================================================
// SECTION 10: adaptSpawners()
// Reads spawner config and stores SpawnerRecords with prefab_ref names.
// No entity pre-creation — SpawnerSystem calls EntityFactory at runtime.
// ===========================================================================

void FBSceneAdapter::adaptSpawners(FBSceneContext& ctx) const
{
    const auto* spawnTypesVec = m_scene->spawners_type();
    const auto* spawnersVec   = m_scene->spawners();
    const auto  count         = spawnersVec->size();

    for (flatbuffers::uoffset_t i = 0; i < count; ++i) {
        const Simulation::BaseSpawner* base = nullptr;

        switch ((*spawnTypesVec)[i]) {
        case Simulation::SpawnerType::SphereSpawner: {
            const auto* s = spawnersVec->GetAs<Simulation::SphereSpawner>(i);
            if (s) base = s->base();
            break;
        }
        case Simulation::SpawnerType::CylinderSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CylinderSpawner>(i);
            if (c) base = c->base();
            break;
        }
        case Simulation::SpawnerType::CapsuleSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CapsuleSpawner>(i);
            if (c) base = c->base();
            break;
        }
        case Simulation::SpawnerType::CuboidSpawner: {
            const auto* c = spawnersVec->GetAs<Simulation::CuboidSpawner>(i);
            if (c) base = c->base();
            break;
        }
        default: continue;
        }

        if (base == nullptr) { continue; }

        SpawnerRecord rec;
        rec.name      = (base->name() != nullptr) ? base->name()->str() : "spawner";
        rec.startTime = base->start_time();
        rec.prefabRef = (base->prefab_ref() != nullptr) ? base->prefab_ref()->str() : "";

        // Warn early if prefab_ref is missing or not in registry
        if (rec.prefabRef.empty()) {
            GE_LOG_WARN("FBSceneAdapter::adaptSpawners: spawner '" + rec.name + "' has no prefab_ref — will be inert.");
        } else if (ctx.prefabRegistry.find(rec.prefabRef) == ctx.prefabRegistry.end()) {
            GE_LOG_WARN("FBSceneAdapter::adaptSpawners: spawner '" + rec.name + "' references unknown prefab '" + rec.prefabRef + "'.");
        }

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
            if (fl && fl->transform()) { rec.fixedPos = toVec3(fl->transform()->position()); }
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

        // --- Owner ---
        const Simulation::SpawnerOwnerType ownerType = base->owner();
        const bool isSequential = (ownerType == Simulation::SpawnerOwnerType::SEQUENTIAL);
        rec.ownerPeerId   = isSequential
            ? uint8_t(1)
            : static_cast<uint8_t>(static_cast<int8_t>(ownerType) + 1);
        rec.isSequential  = isSequential;

        ctx.spawners.push_back(std::move(rec));
    }
}

// ===========================================================================
// SECTION 10: adaptParentLinks() — wires Transform::m_parentEntityID by name
// ===========================================================================

void FBSceneAdapter::adaptParentLinks(FBSceneContext& ctx) const {
    if (m_scene->objects() == nullptr) { return; }

    for (const auto* obj : *m_scene->objects()) {
        if (obj == nullptr || obj->parent() == nullptr) { continue; }
        if (obj->name()   == nullptr) { continue; }

        const std::string childName  = obj->name()->str();
        const std::string parentName = obj->parent()->str();

        if (!ctx.scene->hasEntity(childName) || !ctx.scene->hasEntity(parentName)) {
            GE_LOG_WARN("FBSceneAdapter: parent link '" + childName
                        + "' -> '" + parentName + "' — entity not found, skipped.");
            continue;
        }

        const GE::ECS::EntityID childId  = ctx.scene->getEntityID(childName);
        const GE::ECS::EntityID parentId = ctx.scene->getEntityID(parentName);

        auto* childTr = ctx.em->TryGetTIComponent<GE::Components::Transform>(childId);
        if (childTr == nullptr) { continue; }

        childTr->m_parentEntityID = parentId;
        childTr->m_state = GE::Components::Transform::TransformState::Dirty;
    }
}

} // namespace GE::Scene::FB
