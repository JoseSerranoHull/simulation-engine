#include "SimpleAllocator.h"

/**
 * @brief Queries the physical device to find a memory heap that matches the filter and properties.
 */
uint32_t SimpleAllocator::findMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    // Step 1: Iterate through available memory types to find a bitmask and property match
    for (uint32_t i = TYPE_IDX_ZERO; i < memProperties.memoryTypeCount; ++i) {
        const uint32_t bitFlag = (SHIFT_ONE << i);
        const bool isMatch = ((typeFilter & bitFlag) != TYPE_IDX_ZERO);
        const bool hasProperties = ((memProperties.memoryTypes[i].propertyFlags & properties) == properties);

        if (isMatch && hasProperties) {
            return i;
        }
    }
    throw std::runtime_error("SimpleAllocator: No suitable memory heap found on the GPU!");
}

/**
 * @brief Reserves a single large block of VRAM (Super-Block).
 */
void SimpleAllocator::init(const VkDevice logicalDevice, const VkPhysicalDevice physicalDevice, const VkDeviceSize size) {
    this->device = logicalDevice;
    this->totalSize = size;
    this->currentOffset = VAL_ZERO;

    // Step 1: Create a dummy buffer to query memory requirements for general usage
    VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = size;
    bufferInfo.usage = (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer dummyBuffer{ VK_NULL_HANDLE };
    if (vkCreateBuffer(device, &bufferInfo, nullptr, &dummyBuffer) != VK_SUCCESS) {
        throw std::runtime_error("SimpleAllocator: Failed to create diagnostic buffer!");
    }

    // Step 2: Determine appropriate memory type index for device-local storage
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, dummyBuffer, &memRequirements);
    vkDestroyBuffer(device, dummyBuffer, nullptr);

    this->memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Step 3: Perform the primary GPU memory allocation
    VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocInfo.allocationSize = totalSize;
    allocInfo.memoryTypeIndex = this->memoryTypeIndex;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &memoryChunk) != VK_SUCCESS) {
        throw std::runtime_error("SimpleAllocator: Out of VRAM!");
    }
}

/**
 * @brief Sub-allocates a portion of the Super-Block based on alignment requirements.
 */
VkDeviceSize SimpleAllocator::allocate(const VkMemoryRequirements& requirements) {
    // Step 1: Calculate necessary padding to satisfy hardware alignment
    VkDeviceSize padding = VAL_ZERO;
    const VkDeviceSize remainder = currentOffset % requirements.alignment;

    if (remainder != VAL_ZERO) {
        padding = requirements.alignment - remainder;
    }

    const VkDeviceSize alignedOffset = currentOffset + padding;

    // Step 2: Ensure the super-block has sufficient capacity for the request
    if ((alignedOffset + requirements.size) > totalSize) {
        throw std::runtime_error("SimpleAllocator: VRAM Super-Block exhausted!");
    }

    // Step 3: Update the tracking offset and return the sub-allocation start point
    currentOffset = alignedOffset + requirements.size;
    return alignedOffset;
}

/**
 * @brief Releases the GPU memory heap and resets internal state.
 */
void SimpleAllocator::cleanup() {
    if ((device != VK_NULL_HANDLE) && (memoryChunk != VK_NULL_HANDLE)) {
        vkFreeMemory(device, memoryChunk, nullptr);
        memoryChunk = VK_NULL_HANDLE;
    }
}