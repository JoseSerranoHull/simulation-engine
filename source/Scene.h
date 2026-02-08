#pragma once

/* parasoft-begin-suppress ALL */
#include <map>
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
/* parasoft-end-suppress ALL */

#include "Model.h"
#include "VulkanContext.h"

/**
 * @class Scene
 * @brief Central registry and orchestrator for all 3D entities in the world.
 * * This class manages the lifecycle of Model objects and provides a high-level
 * interface for simulating procedural animations (like hovering or rotation)
 * across the entire model collection.
 */
class Scene final {
public:
    // --- Model Identification Keys ---
    inline static const std::string KEY_MAGIC_CIRCLE = "MagicCircle";
    inline static const std::string KEY_DESERT_QUEEN = "DesertQueen";

    // --- Animation Constants ---
    static constexpr float CIRCLE_ROT_SPEED = 60.0f;
    static constexpr float QUEEN_HOVER_FREQ = 2.0f;
    static constexpr float QUEEN_HOVER_AMP = 0.05f;
    static constexpr float QUEEN_BASE_HEIGHT = 0.5f;

    // --- Lifecycle ---

    /** @brief Constructor: Links the scene to the Vulkan hardware context. */
    explicit Scene();

    /** @brief Default destructor. */
    ~Scene() = default;

    // RAII: Prevent duplication to ensure unique ownership of Model pointers.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // --- Core API ---

    /**
     * @brief Updates all procedural animations and model transforms.
     * Implementation resides in .cpp to reduce header bloat (OPT.18).
     */
    void update(const float deltaTime);

    /**
     * @brief Records draw calls for every model registered in the scene.
     */
    void draw(VkCommandBuffer cb, VkDescriptorSet globalSet, const Pipeline* const pipelineOverride = nullptr);

    // --- Model Registry Management ---

    /** @brief Returns a pointer to a model by its string key; returns nullptr if not found. */
    Model* getModel(const std::string& key);

    /** @brief Returns the full registry for iteration (e.g., in the Renderer). */
    const std::map<std::string, std::unique_ptr<Model>>& getModels() const { return models; }

    /** @brief Transfers unique ownership of a Model into the scene. */
    void addModel(const std::string& name, std::unique_ptr<Model> model) { models[name] = std::move(model); }

    /** @brief Checks if a model exists in the registry. */
    bool hasModel(const std::string& key) const { return models.find(key) != models.end(); }

private:
    // --- Internal State ---
    std::map<std::string, std::unique_ptr<Model>> models; /**< Map-based model storage for O(log n) lookup. */
};