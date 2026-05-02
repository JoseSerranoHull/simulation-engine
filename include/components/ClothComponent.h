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
    bool      burned       { false }; // true = freed by burning, falls freely
    bool      wasCurled    { false }; // true = vertex-curl already applied at burn moment
};

// Explicit spring between two cloth particles.
// kSpring is baked at load time; runtime ImGui slider changes affect new cloth loads only.
enum class SpringType : uint8_t { Structural = 0, Shear = 1, Flexion = 2 };

struct ClothSpring {
    uint16_t   a           { 0 };                     // index into ClothComponent::particles
    uint16_t   b           { 0 };
    float      restLen     { 0.0f };                  // original rest length (set at load time)
    float      kSpring     { 100.0f };                // spring constant (structural / shear / flexion)
    bool       active      { true };                  // false = spring has torn; permanently skip
    float      stressAccum { 0.0f };                  // accumulated stress from torn neighbours
    SpringType type        { SpringType::Structural }; // for debug colour coding
};

// An independent ignition point on the cloth.
struct BurnSource {
    glm::vec3 center { 0.0f, -1000.0f, 0.0f }; // world-space position (far off-scene = inactive by default)
    float     radius { 0.8f };
    bool      active { false };
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
    std::vector<ClothSpring> springs;              // explicit spring list (populated at load time)
    float tearThreshold     { 3.0f };             // spring tears when length > threshold * restLen
    float tearRoughness     { 0.04f };            // random position jitter applied to particles at tear boundary
    float stressTransferRate{ 0.4f };             // fraction of excess tension transferred to neighbour springs on tear

    // Burning (Runtime ImGui tweakables — benign race on POD scalars/vec3)
    float                   burnRate    { 1.5f }; // heat units per second (applies to all sources)
    float                   curlAmount  { 0.05f };// position warp magnitude applied once when a particle burns
    std::vector<BurnSource> burnSources;          // active ignition points (populated by FBSceneAdapter + ImGui)

    // Jakobsen constraint solver
    int   constraintIters   { 2 };     // constraint relaxation passes per physics tick (1-8)

    // Heat diffusion (Dayong 2011 Laplacian conduction along spring graph)
    float heatConductivity  { 0.4f };  // heat flux per unit temperature difference per second
    float shrinkScale       { 0.35f }; // rest-length reduction factor per unit heat (creates wrinkling)

    // Rendering: true when cloth is rendered via Phong + texture pipeline.
    // Cold particles write white (1,1,1) so albedo × fragVertexColor leaves texture unmodified.
    bool  useTextureMode    { false };

    // Runtime ImGui tweakables (benign race between main thread write and physics thread read)
    bool  windEnabled   { false };
    float windX         { 0.0f }, windZ { 0.0f };
    float dragCoeff     { 1.2f };  // aerodynamic drag coefficient (Cd), tunable 0.1-4.0
    float gustAmplitude { 0.3f };  // gust amplitude: wind scales by (1 ± gustAmplitude)
    float gustFrequency { 0.8f };  // gust oscillation rate in Hz
};

} // namespace GE::Components
