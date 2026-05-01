#pragma once

#include "scripts/GameScriptComponent.h"
#include "ecs/EntityManager.h"
#include "components/Components.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"
#include "components/Tag.h"
#include "assets/Mesh.h"
#include "assets/Material.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <random>
#include <string>
/* parasoft-end-suppress ALL */

namespace GE::Scripts {

/**
 * @class ClothSphereSpawnerScript
 * @brief Spawns physics spheres on SPACE-press that arc into the cloth panel.
 *        Spawns just above the cloth's initial horizontal y=3.0 with vy=0, so
 *        both sphere and free cloth rows fall at the same gravity rate — the sphere
 *        stays within its radius of the cloth face and always collides.
 *        Destroyed by TTL (10s) OR floor contact (y < sphereRadius).
 */
class ClothSphereSpawnerScript final : public GameScriptComponent {
public:
    float sphereRadius { 0.18f };
    float launchSpeedZ { 9.0f };
    float sphereTTL    { 10.0f };  // seconds until auto-destroy

    ClothSphereSpawnerScript(GE::Assets::Mesh* mesh, GE::Assets::Material* mat)
        : m_mesh(mesh), m_mat(mat) {}

    const char* GetScriptName() const override { return "ClothSphereSpawner"; }

    void Update(float dt) override {
        auto* input = GetInput();
        auto* em    = GetEntityManager();
        if (!input || !em) { return; }

        // One sphere per key-press (edge detect, no repeat while held)
        const bool spaceNow = input->IsKeyDown(GLFW_KEY_SPACE);
        if (spaceNow && !m_spaceWasDown) { SpawnSphere(em); }
        m_spaceWasDown = spaceNow;

        // Destroy spheres that have hit the floor OR exceeded their TTL
        for (auto it = m_spheres.begin(); it != m_spheres.end(); ) {
            it->lifetime -= dt;
            auto* tr = em->TryGetTIComponent<GE::Components::Transform>(it->id);
            const bool expired   = it->lifetime <= 0.0f;
            const bool hitFloor  = (tr != nullptr) && (tr->m_worldPosition.y < sphereRadius);
            const bool gone      = (tr == nullptr);
            if (expired || hitFloor || gone) {
                if (tr) { em->DestroyEntity(it->id); }
                it = m_spheres.erase(it);
            } else {
                ++it;
            }
        }
    }

    void OnDestroy() override {
        auto* em = GetEntityManager();
        if (!em) { return; }
        for (const auto& entry : m_spheres) { em->DestroyEntity(entry.id); }
        m_spheres.clear();
    }

private:
    struct SphereEntry {
        GE::ECS::EntityID id;
        float             lifetime;
    };

    GE::Assets::Mesh*          m_mesh         { nullptr };
    GE::Assets::Material*      m_mat          { nullptr };
    bool                       m_spaceWasDown { false };
    std::vector<SphereEntry>   m_spheres;
    std::mt19937               m_rng          { 42U };

    void SpawnSphere(GE::ECS::EntityManager* em) {
        // Cloth: pinned at y=3.0 (row 0, z=0); initial state = horizontal at y=3.0, z=0..2.175.
        //
        // Free-fall matching: spawn at y = 3.0 + yExtra (just above the cloth's initial y),
        // with vy=0. Both the sphere AND the free cloth rows (which start from rest) fall
        // under the same gravity acceleration. The Y-gap stays constant throughout the Z-
        // approach, ensuring the sphere always stays within its radius of the cloth particles
        // regardless of how far the cloth has settled.
        //
        // With vz=-9, spawnZ=4.5: t=0.5 s to cloth face (z=0). Gravity drop = 1.225 m.
        //   sphere y at z=0: (3.0 + yExtra) - 1.225 ≈ 1.875–1.925  ← cloth centre ✓
        std::uniform_real_distribution<float> xDist( 0.3f,  1.9f);
        std::uniform_real_distribution<float> yExtra(0.10f, 0.15f);
        std::uniform_real_distribution<float> xvDist(-0.4f, 0.4f);

        const glm::vec3 spawnPos { xDist(m_rng), 3.0f + yExtra(m_rng), 4.5f };
        const glm::vec3 velocity { xvDist(m_rng), 0.0f, -launchSpeedZ };

        const GE::ECS::EntityID id = em->CreateEntity();

        GE::Components::Transform tr;
        tr.m_localPosition = spawnPos;
        tr.m_worldPosition = spawnPos;
        const glm::mat4 mMat = glm::translate(glm::mat4(1.0f), spawnPos);
        tr.m_localMatrix = mMat;
        tr.m_worldMatrix = mMat;
        em->AddComponent(id, tr);

        GE::Components::Tag tag;
        tag.m_name = "ClothSphere_" + std::to_string(id);
        em->AddComponent(id, tag);

        // SphereCollider — ClothSystem picks this up automatically; no extra wiring needed
        GE::Components::SphereCollider sc;
        sc.radius    = sphereRadius;
        sc.isTrigger = false;
        em->AddComponent(id, sc);

        GE::Components::RigidBody rb;
        rb.mass           = 0.5f;
        rb.inverseMass    = 2.0f;
        rb.restitution    = 0.4f;
        rb.isStatic       = false;
        rb.useGravity     = true;
        rb.velocity       = velocity;
        const float r2    = sphereRadius * sphereRadius;
        rb.invInertiaTensor = glm::mat3(5.0f / (2.0f * rb.mass * r2));
        em->AddComponent(id, rb);

        if (m_mesh && m_mat) {
            GE::Components::MeshRenderer mr;
            mr.subMeshes.push_back({ m_mesh, m_mat });
            em->AddComponent(id, mr);
        }

        m_spheres.push_back({ id, sphereTTL });
    }
};

} // namespace GE::Scripts
