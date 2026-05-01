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
    float     heat         { 0.0f }; // 0 = cold, 1.0 = fully burned
    bool      burned       { false };// true = freed by burning, falls freely
};

// Explicit spring between two cloth particles.
// kSpring is baked at load time; runtime ImGui slider changes affect new cloth loads only.
struct ClothSpring {
    uint16_t a       { 0 };       // index into ClothComponent::particles
    uint16_t b       { 0 };
    float    restLen { 0.0f };    // original rest length (set at load time)
    float    kSpring { 100.0f };  // spring constant (structural / shear / flexion)
    bool     active  { true };    // false = spring has torn; permanently skip
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

    // Tearing
    std::vector<ClothSpring> springs;            // explicit spring list (populated at load time)
    float tearThreshold { 3.0f };                // spring tears when length > threshold * restLen

    // Burning (Runtime ImGui tweakables — benign race on POD scalars/vec3)
    glm::vec3 burnCenter { 0.0f, -1000.0f, 0.0f }; // default far off-scene = inactive
    float     burnRadius { 0.8f };
    float     burnRate   { 1.5f };               // heat units per second within burnRadius
    bool      burnActive { false };

    // Runtime ImGui tweakables (benign race between main thread write and physics thread read)
    bool  windEnabled { false };
    float windX { 0.0f }, windZ { 0.0f };
};

} // namespace GE::Components
