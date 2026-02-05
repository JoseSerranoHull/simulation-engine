#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
#include <vector>
#include <stdexcept>
/* parasoft-end-suppress ALL */

/**
 * @class SimpleAllocator
 * @brief Manages a pre-allocated "Super-Block" of GPU memory.
 * * This allocator reduces driver overhead by performing a single large
 * VkAllocateMemory call and sub-allocating smaller chunks via offset tracking.
 */
class SimpleAllocator final {
public:
    // --- Lifecycle ---

    /** @brief Default constructor: Handles are initialized to VK_NULL_HANDLE via member defaults. */
    SimpleAllocator() = default;

    /** @brief Default destructor: Requires explicit call to cleanup() for safe GPU resource release. */
    ~SimpleAllocator() = default;

    // RAII: Prevent copying to ensure unique ownership of the memory heap handle.
    SimpleAllocator(const SimpleAllocator&) = delete;
    SimpleAllocator& operator=(const SimpleAllocator&) = delete;

    // --- Core API ---

    /**
     * @brief Reserves a single large block of VRAM.
     */
    void init(const VkDevice logicalDevice, const VkPhysicalDevice physicalDevice, const VkDeviceSize size);

    /**
     * @brief Sub-allocates a portion of the Super-Block based on hardware requirements.
     * @return The offset within the memoryChunk where the allocation starts.
     */
    VkDeviceSize allocate(const VkMemoryRequirements& requirements);

    /** @brief Returns the raw handle to the allocated GPU memory heap. */
    VkDeviceMemory getMemoryHandle() const { return memoryChunk; }

    /** @brief Releases the GPU memory heap and resets internal state. */
    void cleanup();

    // --- Named Constants ---
    static constexpr uint32_t SHIFT_ONE = 1U;
    static constexpr VkDeviceSize VAL_ZERO = 0ULL;
    static constexpr uint32_t TYPE_IDX_ZERO = 0U;

private:
    /**
     * @brief Queries the physical device to find a memory heap that matches the filter and properties.
     */
    uint32_t findMemoryType(const VkPhysicalDevice physicalDevice, const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const;

    // --- GPU Handles & State ---
    VkDevice device{ VK_NULL_HANDLE };
    VkDeviceMemory memoryChunk{ VK_NULL_HANDLE };
    VkDeviceSize currentOffset{ VAL_ZERO };
    VkDeviceSize totalSize{ VAL_ZERO };
    uint32_t memoryTypeIndex{ TYPE_IDX_ZERO };
};