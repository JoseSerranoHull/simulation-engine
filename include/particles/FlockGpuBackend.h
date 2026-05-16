#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <string>
#include "core/libs.h"
/* parasoft-end-suppress ALL */

#include "graphics/VulkanUtils.h"
#include "graphics/ShaderModule.h"
#include "core/Common.h"
#include "graphics/VulkanContext.h"
#include "core/ServiceLocator.h"

namespace GE::Particles {

/**
 * @struct GpuBoid
 * @brief GPU-side boid state stored in the ping-pong SSBOs.
 * Layout matches the GLSL BoidState struct exactly (std140, two vec4s = 32 bytes).
 */
struct alignas(16) GpuBoid {
    glm::vec4 posGroup; ///< xyz = world position, w = groupId (cast to float)
    glm::vec4 vel;      ///< xyz = velocity,        w = 0 (unused)
};

/**
 * @struct FlockUBO
 * @brief Compute shader parameters uploaded each dispatch.
 * 48 bytes, std140 compliant (all fields are 4-byte scalars in three 16-byte rows).
 */
struct alignas(16) FlockUBO {
    float    deltaTime;
    uint32_t boidCount;
    uint32_t pingPong;   ///< 0 = read A / write B; 1 = read B / write A
    float    pad0;

    float separationRadius;
    float alignmentRadius;
    float cohesionRadius;
    float wSeparation;

    float wAlignment;
    float wCohesion;
    float maxSpeed;
    float maxForce;

    // Containment: soft spring pulling boids back when they leave the spawn sphere.
    // xyz = spawn centre, w = spawn radius.
    glm::vec4 spawnCenterRadius;
    float     containmentK;  // spring constant (force per unit overshoot)
    float     pad1, pad2, pad3;
};

/**
 * @class FlockGpuBackend
 * @brief Manages GPU-based boid steering (Compute) and point-sprite rendering (Graphics).
 *
 * Mirrors the GpuParticleBackend pattern exactly:
 *  - Two device-local SSBOs (ping-pong) — each also bound as a vertex buffer.
 *  - One host-visible UBO for simulation parameters.
 *  - Separate compute pipeline (flock_brute_comp.spv) and graphics pipeline
 *    (flock_vert.spv / flock_frag.spv).
 *
 * Demonstrates the "Extended Concurrency — Compute Shaders" requirement from
 * the Simulation and Concurrency final lab: the O(N²) BruteForce steering loop
 * runs entirely on the GPU, one thread per boid, in parallel.
 */
class FlockGpuBackend final {
public:
    // --- Lifecycle ---

    /**
     * @brief Constructs and initialises all GPU resources.
     * @param transparentRenderPass  The render pass used for the transparent geometry pass.
     * @param inGlobalSetLayout      Set-0 descriptor layout (view/proj/lights).
     */
    explicit FlockGpuBackend(VkRenderPass transparentRenderPass,
                             VkDescriptorSetLayout inGlobalSetLayout);

    ~FlockGpuBackend();

    FlockGpuBackend(const FlockGpuBackend&) = delete;
    FlockGpuBackend& operator=(const FlockGpuBackend&) = delete;

    // --- Core Execution ---

    /**
     * @brief Uploads initial boid positions/velocities from the ECS and seeds SSBO A.
     * Call once after construction, before the first Dispatch.
     */
    void Init(std::vector<GpuBoid> initialBoids);

    /**
     * @brief Records the compute dispatch and post-dispatch barrier.
     * Must be called OUTSIDE an active render pass (before recordTransparentPass).
     */
    void Dispatch(VkCommandBuffer cb, const FlockUBO& params);

    /**
     * @brief Records point-sprite draw commands for the current output SSBO.
     * Must be called INSIDE the transparent render pass.
     */
    void Draw(VkCommandBuffer cb, VkDescriptorSet globalDescriptorSet) const;

    /**
     * @brief Re-seeds all boids to random positions inside the spawn sphere.
     * Safe to call from the main/graphics thread during OnGUI (after vkWaitForFences
     * has confirmed the previous frame completed). Resets ping-pong to 0.
     */
    void Restart(glm::vec3 spawnCenter, float spawnRadius);

    uint32_t GetBoidCount() const { return m_boidCount; }

private:
    // --- Named Constants ---
    static constexpr uint32_t COMPUTE_WORKGROUP_SIZE = 256U;
    static constexpr uint32_t BINDING_UBO            = 0U;
    static constexpr uint32_t BINDING_SSBO_A         = 1U;
    static constexpr uint32_t BINDING_SSBO_B         = 2U;
    static constexpr uint32_t DESCRIPTOR_COUNT_ONE   = 1U;
    static constexpr uint32_t SET_INDEX_GLOBAL        = 0U;

    // --- Configuration ---
    uint32_t              m_boidCount    { 0 };
    uint32_t              m_pingPong     { 0 }; ///< Toggled after every Dispatch
    VkDescriptorSetLayout m_globalSetLayout { VK_NULL_HANDLE };
    VkSampleCountFlagBits m_msaaSamples;

    // --- Ping-pong SSBO pair (DEVICE_LOCAL | STORAGE | VERTEX) ---
    VkBuffer       m_ssboA { VK_NULL_HANDLE };
    VkDeviceMemory m_memA  { VK_NULL_HANDLE };
    VkBuffer       m_ssboB { VK_NULL_HANDLE };
    VkDeviceMemory m_memB  { VK_NULL_HANDLE };

    // --- UBO (HOST_VISIBLE | HOST_COHERENT, persistent-mapped) ---
    VkBuffer       m_ubo       { VK_NULL_HANDLE };
    VkDeviceMemory m_uboMem    { VK_NULL_HANDLE };
    void*          m_uboMapped { nullptr };

    // --- Compute Pipeline State ---
    VkDescriptorSetLayout m_computeSetLayout   { VK_NULL_HANDLE };
    VkPipelineLayout      m_computePipeLayout  { VK_NULL_HANDLE };
    VkPipeline            m_computePipeline    { VK_NULL_HANDLE };
    VkDescriptorPool      m_descriptorPool     { VK_NULL_HANDLE };
    VkDescriptorSet       m_descriptorSet      { VK_NULL_HANDLE };

    // --- Graphics Pipeline State ---
    VkPipelineLayout m_gfxPipeLayout { VK_NULL_HANDLE };
    VkPipeline       m_gfxPipeline   { VK_NULL_HANDLE };

    // --- Initialisation Helpers ---
    void createBuffers(uint32_t boidCount);
    void createComputeDescriptors();
    void createComputePipeline();
    void createGraphicsPipeline(VkRenderPass renderPass);
};

} // namespace GE::Particles
