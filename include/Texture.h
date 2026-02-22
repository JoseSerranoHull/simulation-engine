#pragma once

/* parasoft-begin-suppress ALL */
#include "../include/libs.h"
#include "../include/VulkanUtils.h"
#include "../include/VulkanContext.h"
#include "../include/ServiceLocator.h"
#include <stb_image.h>
#include <cmath>
#include <algorithm>
#include <string>
/* parasoft-end-suppress ALL */

namespace GE::Graphics {

/**
 * @class Texture
 * @brief Manages a GPU-side image resource, including its view, sampler, and mipmap chain.
 * This class supports both loading textures from disk and wrapping existing GPU handles
 * for refraction/post-processing snapshots.
 */
class Texture final {
public:
    static constexpr uint32_t BYTES_PER_PIXEL = 4U; // Standard RGBA8 format
    static constexpr uint32_t MIP_LEVEL_ONE = 1U;

private:
    VkImage image{ VK_NULL_HANDLE };
    VkImageView imageView{ VK_NULL_HANDLE };
    VkSampler sampler{ VK_NULL_HANDLE };
    VkDeviceMemory memory{ VK_NULL_HANDLE };

    uint32_t width{ 0U };
    uint32_t height{ 0U };
    uint32_t mipLevels{ MIP_LEVEL_ONE };
    bool ownsResources{ true };

public:
    /**
     * @brief Loads an image from disk and uploads it to Device Local memory.
     * Performs automatic mipmap generation for improved texture filtering at distance.
     */
    Texture(const std::string& path)
        : ownsResources(true)
    {
        int32_t texWidth{ 0 };
        int32_t texHeight{ 0 };
        int32_t texChannels{ 0 };

        // Force RGBA8 format (4 channels) for Vulkan compatibility
        stbi_uc* const pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

        if (pixels == nullptr) {
            throw std::runtime_error("Texture: Failed to load image from path: " + path);
        }

        // 1. Calculate Mip Levels: Based on the largest dimension of the image
        width = static_cast<uint32_t>(texWidth);
        height = static_cast<uint32_t>(texHeight);

        const float maxDim = static_cast<float>(std::max(width, height));
        // Formula: mipLevels = floor(log2(max(w, h))) + 1
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(maxDim))) + MIP_LEVEL_ONE;

        const VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * static_cast<VkDeviceSize>(BYTES_PER_PIXEL);

        // 2. Create Host-Visible Staging Buffer to move pixels to the GPU
        VkBuffer stagingBuffer{ VK_NULL_HANDLE };
        VkDeviceMemory stagingMemory{ VK_NULL_HANDLE };

        VulkanContext* context = ServiceLocator::GetContext();

        VulkanUtils::createBuffer(context->device, context->physicalDevice, imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            stagingBuffer, stagingMemory);

        void* data{ nullptr };
        static_cast<void>(vkMapMemory(context->device, stagingMemory, 0U, imageSize, 0U, &data));
        static_cast<void>(std::memcpy(data, pixels, static_cast<size_t>(imageSize)));
        vkUnmapMemory(context->device, stagingMemory);

        stbi_image_free(pixels); // CPU memory is no longer needed after staging

        // 3. Create GPU Image (Device Local) with support for blitting (required for mipmaps)
        VulkanUtils::createImage(context->device, context->physicalDevice, width, height, mipLevels,
            VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
            (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);

        // 4. Transfer pixel data from Staging -> GPU Image
        VulkanUtils::transitionImageLayout(context->device, context->graphicsCommandPool, context->graphicsQueue,
            image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

        VulkanUtils::copyBufferToImage(context->device, context->graphicsCommandPool, context->graphicsQueue,
            stagingBuffer, image, width, height);

        // 5. Cleanup Staging Resources
        vkDestroyBuffer(context->device, stagingBuffer, nullptr);
        vkFreeMemory(context->device, stagingMemory, nullptr);

        // 6. Mipmap Generation: Blits the image down through the levels for distance filtering
        VulkanUtils::generateMipmaps(context->device, context->physicalDevice, context->graphicsCommandPool,
            context->graphicsQueue, image, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);

        // 7. Establish the Image View and Sampler
        imageView = VulkanUtils::createImageView(context->device, image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
        VulkanUtils::createTextureSampler(context->device, sampler, mipLevels);
    }

    /**
     * @brief Wrapper Constructor: Borrows existing GPU handles.
     * Used for Refraction Bridge snapshots where the PostProcessor owns the image.
     */
    Texture(VulkanContext* const inContext, VkImage existingImage, VkImageView existingView, VkSampler existingSampler)
        : image(existingImage), imageView(existingView), sampler(existingSampler), ownsResources(false)
    {
    }

    /**
     * @brief Destructor: Releases GPU handles ONLY if this object owns them.
     */
    ~Texture() {
        VulkanContext* context = ServiceLocator::GetContext();
        if (ownsResources && (context != nullptr) && (context->device != VK_NULL_HANDLE)) {
            vkDestroySampler(context->device, sampler, nullptr);
            vkDestroyImageView(context->device, imageView, nullptr);
            vkDestroyImage(context->device, image, nullptr);
            vkFreeMemory(context->device, memory, nullptr);
        }
    }

    // RAII: Unique ownership policy
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // --- Data Queries ---
    VkImageView getImageView() const { return imageView; }
    VkSampler getSampler() const { return sampler; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
};

} // namespace GE::Graphics