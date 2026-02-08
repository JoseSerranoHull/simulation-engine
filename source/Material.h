#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <memory>
#include <utility>
/* parasoft-end-suppress ALL */

#include "Texture.h"
#include "Pipeline.h"

/**
 * @class Material
 * @brief Represents a PBR (Physically Based Rendering) material.
 * * Orchestrates the binding between a specific GPU Pipeline and the Descriptor Set (Set 1)
 * containing the material's textures (Albedo, Normal, AO, Metallic, Roughness).
 */
class Material final {
private:
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    Pipeline* pipeline{ nullptr };

    // Resource tracking: shared_ptr ensures textures stay in VRAM while used by any material.
    std::shared_ptr<Texture> baseColor{ nullptr };
    std::shared_ptr<Texture> normalMap{ nullptr };
    std::shared_ptr<Texture> aoMap{ nullptr };
    std::shared_ptr<Texture> metallicMap{ nullptr };
    std::shared_ptr<Texture> roughnessMap{ nullptr };

public:
    /**
     * @brief Minimal Constructor (Delegated logic).
     * Used primarily by AssetManager when descriptor updates are handled externally.
     */
    Material(const VkDescriptorSet inDescriptorSet, Pipeline* const inPipeline)
        : descriptorSet(inDescriptorSet), pipeline(inPipeline)
    {
    }

    /**
     * @brief Full Constructor for PBR Materials.
     * Maps the full suite of textures to this material instance and takes ownership of references.
     */
    Material(
        Pipeline* const inPipeline,
        const VkDescriptorSet inDescriptorSet,
        std::shared_ptr<Texture> inColor,
        std::shared_ptr<Texture> inNormal,
        std::shared_ptr<Texture> inAo,
        std::shared_ptr<Texture> inMetallic,
        std::shared_ptr<Texture> inRoughness
    ) : 
        descriptorSet(inDescriptorSet),
        pipeline(inPipeline),
        baseColor(std::move(inColor)),
        normalMap(std::move(inNormal)),
        aoMap(std::move(inAo)),
        metallicMap(std::move(inMetallic)),
        roughnessMap(std::move(inRoughness))
    {
    }

    ~Material() = default;

    // RAII Safety: Materials own specific descriptor sets from a pool; 
    // copying would lead to double-use or invalid binding state.
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    // --- Accessors ---

    /** * @brief Returns the handle for the Material Descriptor Set (Set 1).
     * Optimized with 'const' to satisfy MISRA and Renderer requirements.
     */
    VkDescriptorSet getDescriptorSet() const {
        return descriptorSet;
    }

    /** * @brief Returns the graphics pipeline required to render this material.
     * Optimized with 'const' to satisfy MISRA and Renderer requirements.
     */
    Pipeline* getPipeline() const {
        return pipeline;
    }
};