#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace GE::Components {

struct ClothParticle {
    glm::vec3 position     { 0.0f };
    glm::vec3 prevPosition { 0.0f }; // Verlet: stores last-frame position
    glm::vec3 force        { 0.0f };
    bool      pinned       { false };
};

struct ClothComponent {
    // Grid dimensions
    int   rows { 10 }, cols { 10 };
    float cellSize     { 0.2f };
    float particleMass { 0.1f };

    // Spring constants
    float springK  { 100.0f }; // structural
    float shearK   {  50.0f }; // diagonal
    float flexionK {  25.0f }; // skip-one
    float damping  {   0.1f };

    // Physics state (owned by physics thread, read by main thread for rendering)
    std::vector<ClothParticle> particles;  // rows * cols entries

    // Rendering: host-visible persistent-mapped combined vertex+index buffer.
    // Layout: [0, indexOffset) = Vertex data; [indexOffset, total) = uint32_t indices.
    // Populated at load time; vertex region updated each frame from particle positions.
    VkBuffer       vertexBuffer   { VK_NULL_HANDLE }; // combined vertex+index buffer
    VkDeviceMemory vertexMemory   { VK_NULL_HANDLE }; // backing memory
    void*          mappedVertices { nullptr };         // persistent map (full buffer)
    uint32_t       vertexCount    { 0 };
    VkBuffer       indexBuffer    { VK_NULL_HANDLE };  // unused (same as vertexBuffer)
    VkDeviceMemory indexMemory    { VK_NULL_HANDLE };  // unused
    uint32_t       indexCount     { 0 };
    VkDeviceSize   indexOffset    { 0 };               // byte offset of index region in vertexBuffer

    // Color baked at load time from owner color palette
    glm::vec3 color { 0.8f, 0.8f, 0.8f };

    // Runtime ImGui tweakables (benign race between main thread write and physics thread read)
    float windX { 0.0f }, windZ { 0.0f };
};

} // namespace GE::Components
