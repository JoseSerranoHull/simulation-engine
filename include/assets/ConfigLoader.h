#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <map>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "core/Common.h"

/**
 * @struct ObjectTransform
 * @brief Container for entity transformation data and material parameters.
 * * This structure caches the initial state of an object loaded from the
 * configuration file, including TRS (Translate, Rotate, Scale) and custom
 * shader variables.
 */
struct ObjectTransform {
    // --- Transformation Data ---
    glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
    glm::vec3 rot{ 0.0f, 0.0f, 0.0f };
    glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

    // --- Appearance Data ---
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };

    // --- Dynamic Parameters ---
    /** @brief Map for custom properties like material intensity or growth factors. */
    std::map<std::string, float> params{};
};

/**
 * @class ConfigLoader
 * @brief Static utility for parsing scene-level configuration files.
 * * Handles the extraction of object metadata from text files to drive
 * procedural scene generation.
 */
class ConfigLoader final {
public:
    // --- Named Constants for Parsing ---
    static constexpr size_t INDEX_FIRST = 0U;
    static constexpr char CHAR_COMMENT = '#';
    static constexpr char CHAR_SECTION_BEG = '[';
    static constexpr char CHAR_SECTION_END = ']';
    static constexpr char CHAR_KEY_DELIM = ':';

    // --- Core Interface ---

    /**
     * @brief Parses a scene configuration file and maps object names to their transforms.
     * Implementation is located in the .cpp file to optimize compilation times.
     */
    static std::map<std::string, ObjectTransform> loadConfig(const std::string& path);
};