#pragma once

/* parasoft-begin-suppress ALL */
#include "../include/libs.h"
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
/* parasoft-end-suppress ALL */

#include "../include/VulkanContext.h"
#include "../include/ServiceLocator.h"

namespace GE::Graphics {

/**
 * @class ShaderModule
 * @brief RAII wrapper for a Vulkan Shader Module (SPIR-V).
 * Handles loading binary data from disk and managing the lifecycle of the GPU module.
 */
class ShaderModule final {
private:
    inline static const char* ENTRY_POINT = "main";
    static constexpr uint32_t SEEK_BEGIN = 0U;

    VkShaderModule shaderModule{ VK_NULL_HANDLE };
    VkShaderStageFlagBits stage{ VK_SHADER_STAGE_VERTEX_BIT };

    /**
     * @brief Loads binary SPIR-V data from the file system.
     * Uses std::ios::ate to determine file size efficiently.
     */
    static std::vector<char> readFile(const std::string& filename) {
        // Open at the end (ate) in binary mode
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("ShaderModule: Failed to open SPIR-V file -> " + filename);
        }

        const size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        // Reset to beginning and read full content
        static_cast<void>(file.seekg(static_cast<std::streamoff>(SEEK_BEGIN)));
        static_cast<void>(file.read(buffer.data(), static_cast<std::streamsize>(fileSize)));
        file.close();

        return buffer;
    }

public:
    /**
     * @brief Constructs and initializes a Vulkan Shader Module from a SPIR-V file.
     */
    explicit ShaderModule(const std::string& filepath, const VkShaderStageFlagBits inStage)
        : stage(inStage)
    {
        const std::vector<char> code = readFile(filepath);

        VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        createInfo.codeSize = code.size();

        /** * SPIR-V code must be a pointer to uint32_t.
         * Reinterpret_cast is used here because the binary data is read as char (byte),
         * but SPIR-V requires 4-byte alignment.
         */
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VulkanContext* context = ServiceLocator::GetContext();

        if (vkCreateShaderModule(context->device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("ShaderModule: Failed to create GPU module from -> " + filepath);
        }
    }

    /** @brief Destructor: Releases the shader module handle back to the Vulkan driver. */
    ~ShaderModule() {
        VulkanContext* context = ServiceLocator::GetContext();
        if ((context != nullptr) && (context->device != VK_NULL_HANDLE) && (shaderModule != VK_NULL_HANDLE)) {
            vkDestroyShaderModule(context->device, shaderModule, nullptr);
            shaderModule = VK_NULL_HANDLE;
        }
    }

    // RAII: Prevent copying to avoid multiple objects trying to destroy the same handle.
    ShaderModule(const ShaderModule&) = delete;
    ShaderModule& operator=(const ShaderModule&) = delete;

    // --- Accessors ---

    VkShaderModule getModule() const { return shaderModule; }
    VkShaderStageFlagBits getStage() const { return stage; }

    /**
     * @brief Generates the descriptor used by the Graphics/Compute pipeline.
     * @return Fully populated VkPipelineShaderStageCreateInfo.
     */
    VkPipelineShaderStageCreateInfo getStageInfo() const {
        VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stageInfo.stage = stage;
        stageInfo.module = shaderModule;

        // pName defines the entry point function within the SPIR-V code.
        stageInfo.pName = ENTRY_POINT;

        return stageInfo;
    }
};

} // namespace GE::Graphics