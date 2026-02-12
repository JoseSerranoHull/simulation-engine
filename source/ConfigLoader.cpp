#include "../include/ConfigLoader.h"

/* parasoft-begin-suppress ALL */
#include <fstream>
#include <sstream>
/* parasoft-end-suppress ALL */

/**
 * @brief Parses a scene configuration file and maps object names to their transforms.
 * Orchestrates the reading of text-based configuration into structured GPU-ready data.
 */
std::map<std::string, ObjectTransform> ConfigLoader::loadConfig(const std::string& path) {
    std::map<std::string, ObjectTransform> configs{};
    std::ifstream file(path);

    // Step 1: Safety check to ensure the configuration file exists and is accessible
    if (!file.is_open()) {
        return configs;
    }

    std::string line{ "" };
    std::string currentObject{ "" };

    // Step 2: Iterate through the file line by line
    while (std::getline(file, line)) {

        // Skip empty lines or comment lines starting with '#'
        if (line.empty() || (line[INDEX_FIRST] == CHAR_COMMENT)) {
            continue;
        }

        // Step 3: Handle section headers (e.g., "[ObjectName]") to identify target entities
        if (line[INDEX_FIRST] == CHAR_SECTION_BEG) {
            const size_t endPos = line.find(CHAR_SECTION_END);

            if (endPos != std::string::npos) {
                const size_t length = static_cast<size_t>(endPos - EngineConstants::OFFSET_ONE);
                currentObject = line.substr(EngineConstants::OFFSET_ONE, length);
            }
            continue;
        }

        // Step 4: Parse property keys and their associated values for the current object
        std::stringstream ss(line);
        std::string key{ "" };
        ss >> key;

        // Parse Standard Transformation: Position
        if (key == "pos:") {
            ss >> configs[currentObject].pos.x
                >> configs[currentObject].pos.y
                >> configs[currentObject].pos.z;
        }
        // Parse Standard Transformation: Rotation
        else if (key == "rot:") {
            ss >> configs[currentObject].rot.x
                >> configs[currentObject].rot.y
                >> configs[currentObject].rot.z;
        }
        // Parse Standard Transformation: Scale
        else if (key == "scale:") {
            ss >> configs[currentObject].scale.x
                >> configs[currentObject].scale.y
                >> configs[currentObject].scale.z;
        }
        // Parse Visual Properties: Base Color
        else if (key == "color:") {
            ss >> configs[currentObject].color.r
                >> configs[currentObject].color.g
                >> configs[currentObject].color.b;
        }
        // Parse Generic Dynamic Parameters (handles dynamic material or simulation properties)
        else if (!key.empty() && (key.back() == CHAR_KEY_DELIM)) {
            float val{ 0.0f };
            ss >> val;

            const size_t keyLen = static_cast<size_t>(key.size() - EngineConstants::OFFSET_ONE);
            configs[currentObject].params[key.substr(INDEX_FIRST, keyLen)] = val;
        }
    }

    // Step 5: Close file handle and return the populated configuration map
    file.close();
    return configs;
}