#pragma once

#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"

/* parasoft-begin-suppress ALL */
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
/* parasoft-end-suppress ALL */

namespace GE::Systems {

enum class FlockSpatialMode : uint8_t {
    BruteForce  = 0,
    UniformGrid = 1,
    Octree      = 2
};

class FlockingSystem : public GE::ECS::ICpuSystem {
public:
    FlockingSystem();
    ~FlockingSystem() override = default;

    void       OnUpdate(float dt) override;
    ERROR_CODE Shutdown()         override { return ERROR_CODE::OK; }

    // Runtime-configurable (ImGui)
    FlockSpatialMode m_spatialMode  { FlockSpatialMode::BruteForce };
    float            m_gridCellSize { 6.0f }; // should match max neighbourhood radius

    // Performance counters (reset each frame, read by ImGui)
    uint64_t m_neighbourChecksLastFrame { 0 };
    float    m_lastUpdateMs             { 0.0f };

    // Freeze toggle: when true, zero all velocities and skip steering
    bool m_frozen { false };

    // Spawn configuration — set by FlatBuffersScenario after scene load.
    // Used by Restart() to scatter agents back to the original spawn sphere.
    glm::vec3 m_spawnOrigin { 0.0f };
    float     m_spawnRadius { 5.0f };

    // Reset all agents to random positions inside the spawn sphere with small
    // random velocities, then unfreeze. Lets settings take effect from a clean state.
    void Restart(GE::ECS::EntityManager* em);

private:
    // ---- Agent cache (rebuilt each frame) ----
    struct AgentEntry {
        uint32_t  idx;   // index into FlockingComponent array
        glm::vec3 pos;
        glm::vec3 vel;
        float     mass;
        uint8_t   group;
    };
    std::vector<AgentEntry> m_agents;

    // ---- Steering force computation ----
    glm::vec3 computeSteering(const AgentEntry& self,
                               const std::vector<uint32_t>& neighbourIndices,
                               GE::ECS::EntityManager& em) const;

    // ---- Spatial partitioning: Uniform Grid ----
    struct GridKey {
        int x, y, z;
        bool operator==(const GridKey& o) const { return x == o.x && y == o.y && z == o.z; }
    };
    struct GridKeyHash {
        std::size_t operator()(const GridKey& k) const {
            return std::hash<int>()(k.x)
                ^ (std::hash<int>()(k.y) << 11)
                ^ (std::hash<int>()(k.z) << 22);
        }
    };
    std::unordered_map<GridKey, std::vector<uint32_t>, GridKeyHash> m_grid;

    void buildUniformGrid();
    std::vector<uint32_t> queryUniformGrid(const glm::vec3& pos, float radius);

    // ---- Spatial partitioning: Octree ----
    struct OctreeNode {
        glm::vec3 center   { 0.0f };
        glm::vec3 halfSize { 50.0f };
        std::vector<uint32_t> agentIndices;
        std::unique_ptr<OctreeNode> children[8];
        bool isLeaf() const {
            return children[0] == nullptr;
        }
    };
    std::unique_ptr<OctreeNode> m_octreeRoot;

    void buildOctree();
    void insertOctree(OctreeNode& node, uint32_t agentIdx, int depth);
    void queryOctree(const OctreeNode& node, const glm::vec3& pos, float radius,
                     std::vector<uint32_t>& out);
};

} // namespace GE::Systems
