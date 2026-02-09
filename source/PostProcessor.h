#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <memory>
#include <vector>
/* parasoft-end-suppress ALL */

#include "VulkanUtils.h"
#include "ShaderModule.h"
#include "VulkanContext.h"
#include "Texture.h"
#include "CommonStructs.h"
#include "ISystem.h"

/**
 * @class PostProcessor
 * @brief Orchestrates offscreen HDR rendering, MSAA resolution, and fullscreen effects.
 * Manages the transition from high-poly scene geometry to a resolved 2D image
 * with effects like Bloom and refraction-ready snapshots.
 */
class PostProcessor final : public ISystem{
public:
    // --- Layout and Pipeline Constants ---
    static constexpr uint32_t FULLSCREEN_TRI_VERTS = 3U;
    static constexpr uint32_t ATTACHMENT_COUNT_OFFSCREEN = 2U;
    static constexpr uint32_t DEPENDENCY_COUNT_OFFSCREEN = 2U;
    static constexpr uint32_t LAYOUT_COUNT_POST = 2U;

    /**
     * @brief Constructor: Initializes the post-processing framework.
     */
    explicit PostProcessor(
        const uint32_t inWidth,
        const uint32_t inHeight,
        const VkFormat inSwapChainFormat,
        const VkRenderPass finalRenderPass,
        const VkSampleCountFlagBits inMsaaSamples
    );

    ~PostProcessor();

    // RAII: Unique ownership of HDR and MSAA render targets
    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    // --- Core API ---

    /** @brief Reallocates textures and framebuffers on window resize. */
    void resize(const VkExtent2D& extent);

    /** @brief Renders the final fullscreen triangle with post-processing logic. */
    void draw(const VkCommandBuffer commandBuffer, const bool enableBloom) const;

    /** @brief Captures a snapshot of the opaque scene for use in refraction shaders. */
    void copyScene(const VkCommandBuffer cb) const;

    /** @brief Rebuilds the post-processing pipeline (Set 0: Resolve Image). */
    void createPipeline(const VkRenderPass finalRenderPass);

    // --- Synchronization Getters ---

    VkRenderPass getOffscreenRenderPass() const { return offscreenRenderPass; }
    VkFramebuffer getOffscreenFramebuffer() const { return offscreenFramebuffer; }
    Texture* getBackgroundTexture() const { return backgroundTextureWrapper.get(); }
    VkRenderPass getTransparentRenderPass() const { return transparentRenderPass; }
    VkImageView getBackgroundImageView() const { return backgroundImageView; }
    VkSampler getBackgroundSampler() const { return backgroundSampler; }
    VkImage getResolveImage() const { return resolveImage; }

    /** @brief Standard ISystem update override (Unused for this base class, as it requires command buffer context).*/
    void update(float deltaTime) override { /* Interface stub */ }

private:
    // Dependencies
    uint32_t width{ 0U };
    uint32_t height{ 0U };
    VkFormat swapChainFormat{ VK_FORMAT_UNDEFINED };

    // HDR Precision: Required for high-intensity light calculation
    const VkFormat hdrFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormat depthFormat{ VK_FORMAT_UNDEFINED };
    VkSampleCountFlagBits msaaSamples{ VK_SAMPLE_COUNT_1_BIT };

    // --- Opaque Stage: Multi-sampled HDR Scene Target ---
    VkImage offscreenImage{ VK_NULL_HANDLE };
    VkDeviceMemory offscreenMemory{ VK_NULL_HANDLE };
    VkImageView offscreenImageView{ VK_NULL_HANDLE };
    VkSampler offscreenSampler{ VK_NULL_HANDLE };
    VkRenderPass offscreenRenderPass{ VK_NULL_HANDLE };
    VkFramebuffer offscreenFramebuffer{ VK_NULL_HANDLE };

    // Primary Scene Depth Buffer
    VkImage internalDepthImage{ VK_NULL_HANDLE };
    VkDeviceMemory internalDepthMemory{ VK_NULL_HANDLE };
    VkImageView internalDepthView{ VK_NULL_HANDLE };

    // --- Refraction Stage: Snapshot of the Opaque Scene ---
    VkImage backgroundImage{ VK_NULL_HANDLE };
    VkDeviceMemory backgroundMemory{ VK_NULL_HANDLE };
    VkImageView backgroundImageView{ VK_NULL_HANDLE };
    VkSampler backgroundSampler{ VK_NULL_HANDLE };

    // Wrapper to allow refraction snapshots to be treated as standard materials
    std::unique_ptr<Texture> backgroundTextureWrapper{ nullptr };

    // Transparent Stage: Re-opens offscreen buffer for alpha blending (Glass/Snow)
    VkRenderPass transparentRenderPass{ VK_NULL_HANDLE };

    // --- Final Stage: Fullscreen Composition ---
    VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
    VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VkPipeline  pipeline{ VK_NULL_HANDLE };
    VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };

    // The 1x Resolved HDR result (Output of MSAA Resolve)
    VkImage resolveImage{ VK_NULL_HANDLE };
    VkDeviceMemory resolveMemory{ VK_NULL_HANDLE };
    VkImageView resolveImageView{ VK_NULL_HANDLE };

    // --- Lifecycle Helpers ---
    void createOffscreenResources();
    void createRenderPass();
    void createTransparentRenderPass();
    void createFramebuffer();
    void createDescriptors();
    void createBackgroundResources();
    void cleanupResources();


    void internalCreateRenderPass(bool isTransparent);
};