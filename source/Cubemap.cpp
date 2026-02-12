#include "../include/VulkanUtils.h"
#include "../include/Cubemap.h"
#include "../include/Common.h"

/* parasoft-begin-suppress ALL */
#include <stb_image.h>
#include <stdexcept>
#include <cstring>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: RESOURCE INITIALIZATION
// ========================================================================

/**
 * @brief Constructor: Orchestrates the loading and assembly of a 6-face GPU Cubemap.
 */
Cubemap::Cubemap(const std::vector<std::string>& filePaths)
{
    int32_t texWidth{ 0 };
    int32_t texHeight{ 0 };
    int32_t texChannels{ 0 };

    std::vector<stbi_uc*> pixels(FACE_COUNT);

    // Step 1: Disk I/O - Load all 6 faces from the provided file paths
    for (uint32_t i = 0U; i < FACE_COUNT; ++i) {
        pixels[i] = stbi_load(filePaths[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (pixels[i] == nullptr) {
            throw std::runtime_error("Cubemap: Failed to load face: " + filePaths[i]);
        }
    }

    const VkDeviceSize layerSize = static_cast<VkDeviceSize>(texWidth) * static_cast<VkDeviceSize>(texHeight) * static_cast<VkDeviceSize>(BYTES_PER_PIXEL);
    const VkDeviceSize totalSize = layerSize * static_cast<VkDeviceSize>(FACE_COUNT);

    // Step 2: Create a host-visible staging buffer to store all faces contiguously
    VkBuffer stagingBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory stagingMemory{ VK_NULL_HANDLE };

	VulkanContext* context = ServiceLocator::GetContext();

    VulkanUtils::createBuffer(context->device, context->physicalDevice, totalSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        stagingBuffer, stagingMemory);

    // Step 3: Populate the staging buffer with pixel data and free CPU memory
    void* data{ nullptr };
    static_cast<void>(vkMapMemory(context->device, stagingMemory, 0U, totalSize, 0U, &data));

    for (uint32_t i = 0U; i < FACE_COUNT; ++i) {
        const size_t byteOffset = static_cast<size_t>(layerSize) * static_cast<size_t>(i);
        void* const pixelOffset = static_cast<void*>(static_cast<uint8_t*>(data) + byteOffset);

        static_cast<void>(std::memcpy(pixelOffset, pixels[i], static_cast<size_t>(layerSize)));
        stbi_image_free(pixels[i]);
    }
    vkUnmapMemory(context->device, stagingMemory);

    // Step 4: Create the final GPU image with CUBE_COMPATIBLE bit enabled
    VulkanUtils::createImage(context->device, context->physicalDevice,
        static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
        MIP_LEVEL_ONE, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory, FACE_COUNT, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    // Step 5: Transition layout to receive data transfer
    VulkanUtils::transitionImageLayout(context->device, context->graphicsCommandPool, context->graphicsQueue,
        image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, MIP_LEVEL_ONE, FACE_COUNT);

    // Step 6: Define copy regions for each of the 6 cubemap layers
    const VkCommandBuffer commandBuffer = VulkanUtils::beginSingleTimeCommands(context->device, context->graphicsCommandPool);

    std::vector<VkBufferImageCopy> regions(FACE_COUNT);
    for (uint32_t i = 0U; i < FACE_COUNT; ++i) {
        regions[i].bufferOffset = layerSize * static_cast<VkDeviceSize>(i);
        regions[i].imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0U, i, 1U };
        regions[i].imageExtent = { static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1U };
    }

    // Step 7: Record and execute the transfer to GPU VRAM
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, FACE_COUNT, regions.data());
    VulkanUtils::endSingleTimeCommands(context->device, context->graphicsCommandPool, context->graphicsQueue, commandBuffer);

    // Step 8: Transition to shader-read-only layout for sampling in the skybox pipeline
    VulkanUtils::transitionImageLayout(context->device, context->graphicsCommandPool, context->graphicsQueue,
        image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, MIP_LEVEL_ONE, FACE_COUNT);

    // Step 9: Cleanup staging resources
    vkDestroyBuffer(context->device, stagingBuffer, nullptr);
    vkFreeMemory(context->device, stagingMemory, nullptr);

    // Step 10: Create the specialized Cubemap Image View and Sampler
    imageView = VulkanUtils::createImageView(context->device, image, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT, MIP_LEVEL_ONE, VK_IMAGE_VIEW_TYPE_CUBE, FACE_COUNT);

    VulkanUtils::createTextureSampler(context->device, sampler, MIP_LEVEL_ONE, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

// ========================================================================
// SECTION 2: RESOURCE CLEANUP
// ========================================================================

/**
 * @brief Destructor: Releases all hardware handles associated with the Cubemap.
 */
Cubemap::~Cubemap() {
    VulkanContext* context = ServiceLocator::GetContext();

    if ((context != nullptr) && (context->device != VK_NULL_HANDLE)) {
        vkDestroySampler(context->device, sampler, nullptr);
        vkDestroyImageView(context->device, imageView, nullptr);
        vkDestroyImage(context->device, image, nullptr);
        vkFreeMemory(context->device, memory, nullptr);
    }
}