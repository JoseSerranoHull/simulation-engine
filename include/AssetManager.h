#pragma once

/* parasoft-begin-suppress ALL */
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
/* parasoft-end-suppress ALL */

// Engine Includes
#include "Texture.h"
#include "Material.h"
#include "Model.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "OBJLoader.h"
#include "../include/Logger.h"

/**
 * @class AssetManager
 * @brief Handles the loading, caching, and lifecycle of GPU resources.
 * * This manager orchestrates the creation of textures, materials, and meshes,
 * ensuring that resources (like textures) are not duplicated in VRAM and
 * that materials are allocated from a unified descriptor pool.
 */
class AssetManager final {
public:
    // --- Named Constants ---
    static constexpr uint32_t PBR_TEXTURE_COUNT = 5U;     /**< Albedo, Normal, AO, Metallic, Roughness. */
    static constexpr uint32_t SET_INDEX_MATERIAL = 1U;    /**< Target descriptor set for materials. */
    static constexpr uint32_t DESCRIPTOR_COUNT_ONE = 1U;  /**< Standard allocation count. */

    // --- Lifecycle ---

    /** @brief Initializes the manager with the shared Vulkan context and logging stream. */
    explicit AssetManager(std::ostream& logStream = std::cout);

    /** @brief Cleans up the asset registry. */
    ~AssetManager();

    // Strict RAII: Prevent copying to maintain unique ownership of asset handles.
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // --- Configuration ---

    /**
     * @brief Assigns the global descriptor pool used for material allocation.
     * Must be called before any material creation or model loading.
     */
    void setDescriptorPool(const VkDescriptorPool pool) { descriptorPool = pool; }

    // --- Core Loading Interface ---

    /**
     * @brief Loads a texture from disk or returns a cached pointer if already loaded.
     */
    std::shared_ptr<GE::Graphics::Texture> loadTexture(const std::string& path);

    /**
     * @brief Loads a 3D model, using the provided selector function to resolve materials by name.
     */
    std::unique_ptr<Model> loadModel(
        const std::string& path,
        const std::function<std::shared_ptr<Material>(const std::string&)>& materialSelector,
        const VkCommandBuffer setupCmd,
        std::vector<VkBuffer>& stagingBuffers,
        std::vector<VkDeviceMemory>& stagingMemories
    );

    /**
     * @brief Creates a PBR material instance and allocates its Descriptor Set from the assigned pool.
     */
    std::shared_ptr<Material> createMaterial(
        std::shared_ptr<GE::Graphics::Texture> albedo,
        std::shared_ptr<GE::Graphics::Texture> normal,
        std::shared_ptr<GE::Graphics::Texture> ao,
        std::shared_ptr<GE::Graphics::Texture> metallic,
        std::shared_ptr<GE::Graphics::Texture> roughness,
        GE::Graphics::Pipeline* const pipeline
    ) const;

    /**
     * @brief Uploads raw geometry data to GPU buffers and creates a Mesh object.
     */
    std::unique_ptr<Mesh> processMeshData(
        const OBJLoader::MeshData& data,
        std::shared_ptr<Material> material,
        const VkCommandBuffer setupCmd,
        std::vector<VkBuffer>& stagingBuffers,
        std::vector<VkDeviceMemory>& stagingMemories
    );

private:
    // --- Internal State & Resources ---

    std::ostream& log;              /**< Reference to the engine's log stream. */
    VkDescriptorPool descriptorPool; /**< Pool handle for material descriptors. */

    /** @brief Local cache to prevent redundant texture loading and VRAM duplication. */
    std::unordered_map<std::string, std::shared_ptr<GE::Graphics::Texture>> textureCache{};
};