#include "assets/AssetManager.h"

using namespace GE::Graphics;
using namespace GE::Assets;

/* parasoft-begin-suppress ALL */
#include <iostream>
#include <stdexcept>
#include <cstring>
/* parasoft-end-suppress ALL */

// ========================================================================
// SECTION 1: LIFECYCLE MANAGEMENT
// ========================================================================

/**
 * @brief Constructor: Initializes the AssetManager with context and logging.
 */
AssetManager::AssetManager(std::ostream& logStream)
    : log(logStream),
    descriptorPool(VK_NULL_HANDLE)
{
    log << "Engine: AssetManager Initialized via Service Locator." << std::endl;
}

/**
 * @brief Destructor: Orchestrates safe cleanup of the resource cache.
 */
AssetManager::~AssetManager() {
    try {
        // Step 1: Clear the texture cache. 
        // std::shared_ptr handles the destruction of Texture objects automatically.
        textureCache.clear();
        log << "Engine: AssetManager Cleaned Up." << std::endl;
    }
    catch (...) {
        // Step 2: Safety "swallowing" of exceptions in a destructor to prevent std::terminate.
    }
}

// ========================================================================
// SECTION 2: HIGH-LEVEL ASSET LOADING
// ========================================================================

/**
 * @brief Loads a texture from disk or returns a cached instance.
 */
std::shared_ptr<Texture> AssetManager::loadTexture(const std::string& path) {
    // Step 1: Cache Lookup to prevent redundant GPU memory usage.
    const auto it = textureCache.find(path);
    if (it != textureCache.end()) {
        return it->second;
    }

    // Step 2: Resource Creation via Texture constructor.
    auto newTexture = std::make_shared<Texture>(path);

    // Step 3: Registration in cache and logging.
    textureCache[path] = newTexture;

    log << "AssetManager: Loaded and cached texture -> " << path << std::endl;
    return newTexture;
}

/**
 * @brief Orchestrates the loading of a 3D model consisting of multiple sub-meshes.
 */
std::unique_ptr<Model> AssetManager::loadModel(
    const std::string& path,
    const std::function<std::shared_ptr<Material>(const std::string&)>& materialSelector,
    const VkCommandBuffer setupCmd,
    std::vector<VkBuffer>& stagingBuffers,
    std::vector<VkDeviceMemory>& stagingMemories)
{
    // Step 1: Initialize the Model container.
    auto model = std::make_unique<Model>();

    // Step 2: Parse raw geometry data from disk using OBJLoader.
    const auto meshData = OBJLoader::loadOBJ(path.c_str());

    // Step 3: Iterate through parsed mesh data and convert to GPU-ready Mesh objects.
    for (const OBJLoader::MeshData& data : meshData) {
        const std::shared_ptr<Material> selectedMat = materialSelector(data.name);

        if (selectedMat != nullptr) {
            // Transfer RAII ownership of processed mesh to the model container.
            model->addMesh(std::move(processMeshData(data, selectedMat, setupCmd, stagingBuffers, stagingMemories)));
        }
    }

    log << "AssetManager: Model loaded from " << path
        << " (" << static_cast<uint32_t>(meshData.size()) << " sub-meshes processed)." << std::endl;

    return model;
}

/**
 * @brief Creates a PBR Material and updates its associated Descriptor Set.
 */
 /**
  * @brief Creates a PBR Material and updates its associated Descriptor Set.
  * Refactored with Null-Checks to prevent memory access violations.
  */
std::shared_ptr<Material> AssetManager::createMaterial(
    std::shared_ptr<Texture> albedo,
    std::shared_ptr<Texture> normal,
    std::shared_ptr<Texture> ao,
    std::shared_ptr<Texture> metallic,
    std::shared_ptr<Texture> roughness,
    GraphicsPipeline* const pipeline
) const {
    VulkanContext* context = ServiceLocator::GetContext();

    // Step 1: Validation of hardware prerequisite
    if (descriptorPool == VK_NULL_HANDLE) {
        throw std::runtime_error("AssetManager: createMaterial called before setDescriptorPool!");
    }

    // Step 2: Allocation of the Descriptor Set
    VkDescriptorSet materialDescriptorSet{ VK_NULL_HANDLE };
    VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = DESCRIPTOR_COUNT_ONE;
    allocInfo.pSetLayouts = &context->materialSetLayout;

    if (vkAllocateDescriptorSets(context->device, &allocInfo, &materialDescriptorSet) != VK_SUCCESS) {
        throw std::runtime_error("AssetManager: Failed to allocate material descriptor set!");
    }

    // Step 3: Batch Update of the 5 PBR textures
    const std::array<std::shared_ptr<Texture>, PBR_TEXTURE_COUNT> textures = {
        albedo, normal, ao, metallic, roughness
    };

    std::array<VkWriteDescriptorSet, PBR_TEXTURE_COUNT> descriptorWrites{};
    std::array<VkDescriptorImageInfo, PBR_TEXTURE_COUNT> imageInfos{};

    for (uint32_t i = 0U; i < PBR_TEXTURE_COUNT; ++i) {
        // --- CRITICAL SAFETY FIX ---
        // If the pointer is null, the engine would crash. Now we throw a descriptive error.
        if (textures[i] == nullptr) {
            std::string slotNames[] = { "Albedo", "Normal", "AO", "Metallic", "Roughness" };
            throw std::runtime_error("AssetManager: Cannot create material. Texture slot [" +
                slotNames[i] + "] is NULL. Check your .ini registry!");
        }

        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[i].imageView = textures[i]->getImageView();
        imageInfos[i].sampler = textures[i]->getSampler();

        descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[i].dstSet = materialDescriptorSet;
        descriptorWrites[i].dstBinding = i;
        descriptorWrites[i].dstArrayElement = 0U;
        descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[i].descriptorCount = DESCRIPTOR_COUNT_ONE;
        descriptorWrites[i].pImageInfo = &imageInfos[i];
    }

    vkUpdateDescriptorSets(context->device, static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(), 0U, nullptr);

    // Step 4: Encapsulate in RAII Material object.
    return std::make_shared<Material>(materialDescriptorSet, pipeline);
}

// ========================================================================
// SECTION 3: LOW-LEVEL GPU GEOMETRY PROCESSING
// ========================================================================

/**
 * @brief Processes raw vertex data into optimized GPU buffers and Mesh objects.
 */
std::unique_ptr<Mesh> AssetManager::processMeshData(
    const OBJLoader::MeshData& data,
    std::shared_ptr<Material> material,
    const VkCommandBuffer setupCmd,
    std::vector<VkBuffer>& stagingBuffers,
    std::vector<VkDeviceMemory>& stagingMemories)
{
    VulkanContext* context = ServiceLocator::GetContext();

    // --- CRITICAL SAFETY CHECK ---
    if (data.vertices.empty() || data.indices.empty()) {
        GE_LOG_ERROR("AssetManager: Attempted to process empty mesh data! Returning null.");
        return nullptr;
    }

    // Step 1: Resource requirement calculation.
    const VkDeviceSize vertexSize = static_cast<VkDeviceSize>(sizeof(Vertex)) * static_cast<VkDeviceSize>(data.vertices.size());
    const VkDeviceSize indexSize = static_cast<VkDeviceSize>(sizeof(uint32_t)) * static_cast<VkDeviceSize>(data.indices.size());
    const VkDeviceSize totalSize = vertexSize + indexSize;

    // Step 2: Transfer Logic - Create Host-Visible Staging Buffer.
    VkBuffer stagingBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory stagingMemory{ VK_NULL_HANDLE };
    VulkanUtils::createBuffer(
        context->device, context->physicalDevice, totalSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        stagingBuffer, stagingMemory
    );

    // Step 3: Memory Mapping - Populate Staging Buffer with geometric data.
    void* mappedData{ nullptr };
    static_cast<void>(vkMapMemory(context->device, stagingMemory, 0ULL, totalSize, 0U, &mappedData));
    static_cast<void>(std::memcpy(mappedData, data.vertices.data(), static_cast<size_t>(vertexSize)));
    static_cast<void>(std::memcpy(static_cast<char*>(mappedData) + vertexSize, data.indices.data(), static_cast<size_t>(indexSize)));
    vkUnmapMemory(context->device, stagingMemory);

    // Step 4: Device Logic - Create Final Device-Local Buffer handle.
    VkBuffer deviceBuffer{ VK_NULL_HANDLE };
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = totalSize;
    bufferInfo.usage = (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(context->device, &bufferInfo, nullptr, &deviceBuffer) != VK_SUCCESS) {
        throw std::runtime_error("AssetManager: Failed to create mesh buffer handle!");
    }

    // Step 5: Resource Binding - Delegate allocation to central engine allocator.
    const VkMemoryRequirements memReqs = VulkanUtils::getBufferMemoryRequirements(context->device, deviceBuffer);
    const VkDeviceSize offset = context->allocator.allocate(memReqs);
    static_cast<void>(vkBindBufferMemory(context->device, deviceBuffer, context->allocator.getMemoryHandle(), offset));

    // Step 6: GPU Synchronization - Record command to copy data from Staging to Device.
    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0ULL;
    copyRegion.dstOffset = 0ULL;
    copyRegion.size = totalSize;
    vkCmdCopyBuffer(setupCmd, stagingBuffer, deviceBuffer, DESCRIPTOR_COUNT_ONE, &copyRegion);

    // Step 7: Deferred Cleanup - Register staging resources for safe destruction post-transfer.
    stagingBuffers.push_back(stagingBuffer);
    stagingMemories.push_back(stagingMemory);

    // Step 8: Final Object Assembly.
    auto mesh = std::make_unique<Mesh>(deviceBuffer, static_cast<uint32_t>(data.indices.size()), vertexSize, material);
    mesh->setName(data.name);

    return mesh;
}