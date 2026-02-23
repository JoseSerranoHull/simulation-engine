#pragma once
#include <string>
#include <vector>

namespace GE::Components {
    /**
     * @struct SkyboxComponent
     * @brief Data container for environment cubemaps.
     */
    struct SkyboxComponent {
        // Paths to the 6 faces (Order: +X, -X, +Y, -Y, +Z, -Z)
        std::vector<std::string> faces;
        bool enabled = true;
    };
}