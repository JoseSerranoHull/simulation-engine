#pragma once

/* parasoft-begin-suppress ALL */
// External headers are suppressed to isolate analysis to local logic.
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan_core.h>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include <array>

// Standard Interleaved Vertex Format. 
struct Vertex {
    // --- SANITIZATION: Named Constants ---
    static constexpr uint32_t BINDING_ZERO = 0U;
    static constexpr uint32_t ATTRIBUTE_COUNT = 4U;
    static constexpr uint32_t LOC_POSITION = 0U;
    static constexpr uint32_t LOC_COLOR = 1U;
    static constexpr uint32_t LOC_TEXCOORD = 2U;
    static constexpr uint32_t LOC_NORMAL = 3U;
    static constexpr uint32_t HASH_SHIFT = 1U;

    // Explicit initialization of all members
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    glm::vec2 texcoord{ 0.0f, 0.0f };
    glm::vec3 normal{ 0.0f, 0.0f, 1.0f };

    // Tells Vulkan how to read the vertex buffer.
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};

        bindingDescription.binding = BINDING_ZERO;
        bindingDescription.stride = static_cast<uint32_t>(sizeof(Vertex));
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    // Maps member variables to Shader input locations.
    static std::array<VkVertexInputAttributeDescription, ATTRIBUTE_COUNT> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, ATTRIBUTE_COUNT> attributeDescriptions{};

        // Position: Location 0
        attributeDescriptions[LOC_POSITION].binding = BINDING_ZERO;
        attributeDescriptions[LOC_POSITION].location = LOC_POSITION;
        attributeDescriptions[LOC_POSITION].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_POSITION].offset = static_cast<uint32_t>(offsetof(Vertex, position));

        // Color: Location 1
        attributeDescriptions[LOC_COLOR].binding = BINDING_ZERO;
        attributeDescriptions[LOC_COLOR].location = LOC_COLOR;
        attributeDescriptions[LOC_COLOR].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_COLOR].offset = static_cast<uint32_t>(offsetof(Vertex, color));

        // TexCoord: Location 2
        attributeDescriptions[LOC_TEXCOORD].binding = BINDING_ZERO;
        attributeDescriptions[LOC_TEXCOORD].location = LOC_TEXCOORD;
        attributeDescriptions[LOC_TEXCOORD].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[LOC_TEXCOORD].offset = static_cast<uint32_t>(offsetof(Vertex, texcoord));

        // Normal: Location 3
        attributeDescriptions[LOC_NORMAL].binding = BINDING_ZERO;
        attributeDescriptions[LOC_NORMAL].location = LOC_NORMAL;
        attributeDescriptions[LOC_NORMAL].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[LOC_NORMAL].offset = static_cast<uint32_t>(offsetof(Vertex, normal));

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return (position == other.position) && (color == other.color) &&
            (texcoord == other.texcoord) && (normal == other.normal);
    }
};

struct VertexHasher {
    size_t operator()(const Vertex& vertex) const {
        // Use std::hash with GLM types (provided by glm/gtx/hash.hpp)
        return ((std::hash<glm::vec3>()(vertex.position) ^
            (std::hash<glm::vec3>()(vertex.color) << Vertex::HASH_SHIFT)) >> Vertex::HASH_SHIFT) ^
            (std::hash<glm::vec2>()(vertex.texcoord) << Vertex::HASH_SHIFT);
    }
};