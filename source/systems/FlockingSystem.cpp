/* parasoft-begin-suppress ALL */
#include <chrono>
#include <cmath>
/* parasoft-end-suppress ALL */

#include "systems/FlockingSystem.h"
#include "components/FlockingComponent.h"
#include "components/PhysicsComponents.h"
#include "components/Transform.h"
#include "ecs/ComponentArray.h"
#include "core/ServiceLocator.h"
#include "core/Common.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

static constexpr int   OCTREE_MAX_DEPTH     = 6;
static constexpr int   OCTREE_MAX_LEAF_AGENTS = 8;
static constexpr float MIN_DIST             = 1e-6f;
static constexpr float OCTREE_HALF_WORLD    = 60.0f; // half-size of root node (world units)

// ============================================================================
// Constructor
// ============================================================================

FlockingSystem::FlockingSystem() {
    m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<FlockingSystem>();
    m_stage  = ECS::ESystemStage::GameLogic;
    m_state  = SystemState::Running;
}

// ============================================================================
// OnUpdate — master loop
// ============================================================================

void FlockingSystem::OnUpdate(float dt) {
    if (dt <= 0.0f) { return; }

    const auto t0 = std::chrono::high_resolution_clock::now();

    GE::ECS::EntityManager* em = ServiceLocator::GetEntityManager();
    if (em == nullptr) { return; }

    auto& fkArr = em->GetCompArr<GE::Components::FlockingComponent>();
    auto& rbArr = em->GetCompArr<GE::Components::RigidBody>();
    auto& trArr = em->GetCompArr<GE::Components::Transform>();

    const uint32_t agentCount = fkArr.GetCount();
    if (agentCount == 0U) { return; }

    // Freeze mode: zero all velocities and skip steering
    if (m_frozen) {
        for (uint32_t i = 0U; i < agentCount; ++i) {
            const GE::ECS::EntityID eid = fkArr.Index()[i];
            auto* rb = em->TryGetTIComponent<GE::Components::RigidBody>(eid);
            if (rb != nullptr) { rb->velocity = glm::vec3(0.0f); }
        }
        m_neighbourChecksLastFrame = 0;
        const auto t1 = std::chrono::high_resolution_clock::now();
        m_lastUpdateMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
        return;
    }

    // --- 1. Build agent cache ---
    m_agents.clear();
    m_agents.reserve(agentCount);
    for (uint32_t i = 0U; i < agentCount; ++i) {
        const GE::ECS::EntityID eid = fkArr.Index()[i];
        const GE::Components::FlockingComponent& fk = fkArr.Data()[i];

        const GE::Components::Transform* tr = em->TryGetTIComponent<GE::Components::Transform>(eid);
        const GE::Components::RigidBody*  rb = em->TryGetTIComponent<GE::Components::RigidBody>(eid);
        if (tr == nullptr || rb == nullptr) { continue; }

        AgentEntry entry;
        entry.idx   = i;
        entry.pos   = tr->m_position;
        entry.vel   = rb->velocity;
        entry.mass  = rb->mass;
        entry.group = fk.groupId;
        m_agents.push_back(entry);
    }

    if (m_agents.empty()) { return; }

    m_neighbourChecksLastFrame = 0U;

    // --- 2. Build spatial structure ---
    switch (m_spatialMode) {
    case FlockSpatialMode::UniformGrid:
        buildUniformGrid();
        break;
    case FlockSpatialMode::Octree:
        buildOctree();
        break;
    default:
        break;
    }

    // --- 3. For each agent, compute steering and apply ---
    for (const AgentEntry& agent : m_agents) {
        const GE::Components::FlockingComponent& fk = fkArr.Data()[agent.idx];

        // Maximum query radius: largest neighbourhood radius
        const float maxRadius = std::max({ fk.separationRadius, fk.alignmentRadius, fk.cohesionRadius });

        // Get neighbours
        std::vector<uint32_t> neighbourIdx;
        switch (m_spatialMode) {
        case FlockSpatialMode::UniformGrid:
            neighbourIdx = queryUniformGrid(agent.pos, maxRadius);
            break;
        case FlockSpatialMode::Octree:
            if (m_octreeRoot != nullptr) {
                queryOctree(*m_octreeRoot, agent.pos, maxRadius, neighbourIdx);
            }
            break;
        default: {
            // Brute force: check all other agents
            for (std::size_t j = 0; j < m_agents.size(); ++j) {
                ++m_neighbourChecksLastFrame;
                const float d = glm::length(m_agents[j].pos - agent.pos);
                if (d < maxRadius) {
                    neighbourIdx.push_back(static_cast<uint32_t>(j));
                }
            }
            break;
        }
        }

        // Compute steering force
        const glm::vec3 steering = computeSteering(agent, neighbourIdx, *em);

        // Apply steering force to RigidBody
        const GE::ECS::EntityID eid = fkArr.Index()[agent.idx];
        GE::Components::RigidBody* rb = em->TryGetTIComponent<GE::Components::RigidBody>(eid);
        if (rb == nullptr) { continue; }

        rb->forceAccum += steering;

        // Clamp velocity to maxSpeed
        const float speed = glm::length(rb->velocity);
        if (speed > fk.maxSpeed && speed > MIN_DIST) {
            rb->velocity = (rb->velocity / speed) * fk.maxSpeed;
        }
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    m_lastUpdateMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
}

// ============================================================================
// computeSteering — Weighted Truncated Sum (Buckland Ch.3 pattern)
// ============================================================================

glm::vec3 FlockingSystem::computeSteering(const AgentEntry& self,
                                           const std::vector<uint32_t>& neighbourIndices,
                                           GE::ECS::EntityManager& em) const
{
    auto& fkArr = em.GetCompArr<GE::Components::FlockingComponent>();
    const GE::Components::FlockingComponent& fk = fkArr.Data()[self.idx];

    // --- Separation ---
    glm::vec3 separation{ 0.0f };
    int sepCount = 0;
    for (uint32_t ni : neighbourIndices) {
        if (ni >= static_cast<uint32_t>(m_agents.size())) { continue; }
        const AgentEntry& n = m_agents[ni];
        if (&n == &self) { continue; }
        // Group filter
        if (fk.groupId != 0 && n.group != fk.groupId) { continue; }

        const glm::vec3 diff = self.pos - n.pos;
        const float dist = glm::length(diff);
        if (dist < fk.separationRadius && dist > MIN_DIST) {
            separation += glm::normalize(diff) / dist; // weight by proximity
            ++sepCount;
        }
    }
    if (sepCount > 0 && glm::length(separation) > MIN_DIST) {
        separation = glm::normalize(separation) * fk.maxForce;
    }

    // --- Alignment ---
    glm::vec3 avgVel{ 0.0f };
    int alignCount = 0;
    for (uint32_t ni : neighbourIndices) {
        if (ni >= static_cast<uint32_t>(m_agents.size())) { continue; }
        const AgentEntry& n = m_agents[ni];
        if (&n == &self) { continue; }
        if (fk.groupId != 0 && n.group != fk.groupId) { continue; }

        const float dist = glm::length(n.pos - self.pos);
        if (dist < fk.alignmentRadius) {
            avgVel += n.vel;
            ++alignCount;
        }
    }
    glm::vec3 alignment{ 0.0f };
    if (alignCount > 0) {
        const glm::vec3 desired = avgVel / static_cast<float>(alignCount);
        const glm::vec3 steer   = desired - self.vel;
        const float mag = glm::length(steer);
        if (mag > MIN_DIST) {
            alignment = (glm::normalize(steer)) * fk.maxForce;
        }
    }

    // --- Cohesion (Seek toward centre of mass) ---
    glm::vec3 centreOfMass{ 0.0f };
    int cohCount = 0;
    for (uint32_t ni : neighbourIndices) {
        if (ni >= static_cast<uint32_t>(m_agents.size())) { continue; }
        const AgentEntry& n = m_agents[ni];
        if (&n == &self) { continue; }
        if (fk.groupId != 0 && n.group != fk.groupId) { continue; }

        const float dist = glm::length(n.pos - self.pos);
        if (dist < fk.cohesionRadius) {
            centreOfMass += n.pos;
            ++cohCount;
        }
    }
    glm::vec3 cohesion{ 0.0f };
    if (cohCount > 0) {
        const glm::vec3 target = centreOfMass / static_cast<float>(cohCount);
        const glm::vec3 toTarget = target - self.pos;
        const float mag = glm::length(toTarget);
        if (mag > MIN_DIST) {
            cohesion = glm::normalize(toTarget) * fk.maxForce;
        }
    }

    // --- Avoidance (push away from static sphere obstacles) ---
    glm::vec3 avoidance{ 0.0f };
    {
        auto& sphereArr = em.GetCompArr<GE::Components::SphereCollider>();
        auto& rbArr     = em.GetCompArr<GE::Components::RigidBody>();
        const uint32_t sc = sphereArr.GetCount();
        for (uint32_t si = 0U; si < sc; ++si) {
            const GE::ECS::EntityID seid = sphereArr.Index()[si];
            const GE::Components::RigidBody* srb = em.TryGetTIComponent<GE::Components::RigidBody>(seid);
            if (srb == nullptr || !srb->isStatic) { continue; }

            // Skip if this entity also has a FlockingComponent (it's a flock agent)
            if (em.TryGetTIComponent<GE::Components::FlockingComponent>(seid) != nullptr) { continue; }

            const GE::Components::Transform* str = em.TryGetTIComponent<GE::Components::Transform>(seid);
            if (str == nullptr) { continue; }

            const float obstacleR   = sphereArr.Data()[si].radius;
            const float avoidRadius = obstacleR + fk.separationRadius * 1.5f;
            const glm::vec3 away    = self.pos - str->m_position;
            const float dist        = glm::length(away);
            if (dist < avoidRadius && dist > MIN_DIST) {
                avoidance += glm::normalize(away) * fk.maxForce;
            }
        }
    }

    // --- Weighted Truncated Sum (Buckland pattern) ---
    glm::vec3 totalForce{ 0.0f };
    float remaining = fk.maxForce;

    auto addForce = [&](const glm::vec3& f, float w) -> bool {
        const glm::vec3 wf  = f * w;
        const float     mag = glm::length(wf);
        if (mag < MIN_DIST) { return true; }
        if (mag > remaining) {
            totalForce += glm::normalize(wf) * remaining;
            remaining = 0.0f;
            return false;
        }
        totalForce += wf;
        remaining  -= mag;
        return true;
    };

    if (!addForce(separation, fk.wSeparation)) { return totalForce; }
    if (!addForce(avoidance,  fk.wAvoidance))  { return totalForce; }
    if (!addForce(alignment,  fk.wAlignment))  { return totalForce; }
    addForce(cohesion, fk.wCohesion);

    return totalForce;
}

// ============================================================================
// Uniform Grid
// ============================================================================

void FlockingSystem::buildUniformGrid() {
    m_grid.clear();
    const float cell = m_gridCellSize;
    for (std::size_t i = 0; i < m_agents.size(); ++i) {
        const glm::vec3& p = m_agents[i].pos;
        GridKey key{
            static_cast<int>(std::floor(p.x / cell)),
            static_cast<int>(std::floor(p.y / cell)),
            static_cast<int>(std::floor(p.z / cell))
        };
        m_grid[key].push_back(static_cast<uint32_t>(i));
    }
}

std::vector<uint32_t> FlockingSystem::queryUniformGrid(const glm::vec3& pos, float radius) {
    std::vector<uint32_t> result;
    const float cell = m_gridCellSize;
    const int minX = static_cast<int>(std::floor((pos.x - radius) / cell));
    const int maxX = static_cast<int>(std::ceil ((pos.x + radius) / cell));
    const int minY = static_cast<int>(std::floor((pos.y - radius) / cell));
    const int maxY = static_cast<int>(std::ceil ((pos.y + radius) / cell));
    const int minZ = static_cast<int>(std::floor((pos.z - radius) / cell));
    const int maxZ = static_cast<int>(std::ceil ((pos.z + radius) / cell));

    for (int cx = minX; cx <= maxX; ++cx) {
        for (int cy = minY; cy <= maxY; ++cy) {
            for (int cz = minZ; cz <= maxZ; ++cz) {
                auto it = m_grid.find(GridKey{ cx, cy, cz });
                if (it == m_grid.end()) { continue; }
                for (uint32_t idx : it->second) {
                    ++m_neighbourChecksLastFrame;
                    const float d = glm::length(m_agents[idx].pos - pos);
                    if (d < radius) {
                        result.push_back(idx);
                    }
                }
            }
        }
    }
    return result;
}

// ============================================================================
// Octree
// ============================================================================

void FlockingSystem::buildOctree() {
    m_octreeRoot = std::make_unique<OctreeNode>();
    m_octreeRoot->center   = glm::vec3{ 0.0f };
    m_octreeRoot->halfSize = glm::vec3{ OCTREE_HALF_WORLD };

    for (std::size_t i = 0; i < m_agents.size(); ++i) {
        insertOctree(*m_octreeRoot, static_cast<uint32_t>(i), 0);
    }
}

void FlockingSystem::insertOctree(OctreeNode& node, uint32_t agentIdx, int depth) {
    // If this is a leaf with room, or we're at max depth, store here
    if (depth >= OCTREE_MAX_DEPTH ||
        (node.isLeaf() && node.agentIndices.size() < static_cast<std::size_t>(OCTREE_MAX_LEAF_AGENTS)))
    {
        node.agentIndices.push_back(agentIdx);
        return;
    }

    // If this leaf is full and not at max depth, subdivide
    if (node.isLeaf()) {
        // Create 8 children
        for (int c = 0; c < 8; ++c) {
            node.children[c] = std::make_unique<OctreeNode>();
            const glm::vec3 half = node.halfSize * 0.5f;
            node.children[c]->halfSize = half;
            node.children[c]->center  = node.center + glm::vec3{
                (c & 1) ? half.x : -half.x,
                (c & 2) ? half.y : -half.y,
                (c & 4) ? half.z : -half.z
            };
        }
        // Re-insert existing agents into children
        for (uint32_t existing : node.agentIndices) {
            const glm::vec3& p = m_agents[existing].pos;
            const int octant = ((p.x >= node.center.x) ? 1 : 0)
                             | ((p.y >= node.center.y) ? 2 : 0)
                             | ((p.z >= node.center.z) ? 4 : 0);
            insertOctree(*node.children[octant], existing, depth + 1);
        }
        node.agentIndices.clear();
    }

    // Insert new agent into correct child
    const glm::vec3& p = m_agents[agentIdx].pos;
    const int octant = ((p.x >= node.center.x) ? 1 : 0)
                     | ((p.y >= node.center.y) ? 2 : 0)
                     | ((p.z >= node.center.z) ? 4 : 0);
    insertOctree(*node.children[octant], agentIdx, depth + 1);
}

// Sphere–AABB overlap test: returns true if sphere (pos, radius) overlaps AABB (center ± halfSize)
static bool sphereAABBOverlap(const glm::vec3& pos, float radius,
                               const glm::vec3& center, const glm::vec3& halfSize)
{
    const glm::vec3 d = glm::max(glm::abs(pos - center) - halfSize, glm::vec3{ 0.0f });
    return glm::dot(d, d) <= radius * radius;
}

void FlockingSystem::queryOctree(const OctreeNode& node, const glm::vec3& pos, float radius,
                                  std::vector<uint32_t>& out)
{
    if (!sphereAABBOverlap(pos, radius, node.center, node.halfSize)) { return; }

    if (node.isLeaf()) {
        for (uint32_t idx : node.agentIndices) {
            ++m_neighbourChecksLastFrame;
            const float d = glm::length(m_agents[idx].pos - pos);
            if (d < radius) {
                out.push_back(idx);
            }
        }
        return;
    }

    for (int c = 0; c < 8; ++c) {
        if (node.children[c] != nullptr) {
            queryOctree(*node.children[c], pos, radius, out);
        }
    }
}

} // namespace GE::Systems
