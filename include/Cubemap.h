#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
/* parasoft-end-suppress ALL */

#include "VulkanContext.h"
#include "ServiceLocator.h"

namespace GE::Graphics {

/**
 * @class Cubemap
 * @brief Manages a 6-face GPU Cubemap texture typically used for Skyboxes or environmental mapping.
 * * This class handles the allocation, initialization, and lifecycle of the six-layered
 * image and its associated hardware sampler.
 */
class Cubemap final {
public:
    // --- Named Constants ---
    static constexpr uint32_t FACE_COUNT = 6U;       /**< Standard cubemap faces (X+, X-, Y+, Y-, Z+, Z-). */
    static constexpr uint32_t BYTES_PER_PIXEL = 4U;  /**< Assuming RGBA8 format. */
    static constexpr uint32_t MIP_LEVEL_ONE = 1U;    /**< Base level for skybox textures. */

    // --- Lifecycle ---

    /**
     * @brief Constructor: Loads six texture files and assembles them into a GPU Cubemap.
     * Implementation is located in the .cpp to satisfy OPT.18.
     */
    Cubemap(const std::vector<std::string>& filePaths);

    /** @brief Destructor: Safely releases all Vulkan image and sampler resources. */
    ~Cubemap();

    // Prevent copying to maintain strict RAII ownership of GPU memory.
    Cubemap(const Cubemap&) = delete;
    Cubemap& operator=(const Cubemap&) = delete;

    // --- Accessors ---

    /** @brief Returns the hardware image view for descriptor binding. */
    VkImageView getImageView() const { return imageView; }

    /** @brief Returns the texture sampler used for cubemap lookups. */
    VkSampler   getSampler() const { return sampler; }

private:
    // --- Internal State & GPU Handles ---
    VkImage image;               /**< The 6-layered Vulkan image handle. */
    VkImageView imageView;       /**< Access point for the shader to read the image. */
    VkDeviceMemory memory;       /**< Backing VRAM allocation for the image. */
    VkSampler sampler;           /**< Hardware configuration for texture filtering. */
};

} // namespace GE::Graphics