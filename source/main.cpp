/* parasoft-begin-suppress ALL */
#include "core/EngineOrchestrator.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib> // For EXIT_SUCCESS/FAILURE
/* parasoft-end-suppress ALL */

/**
 * @brief Vulkan Lab Entry Point.
 * Orchestrates the high-level lifecycle of the Sandy-Snow Globe engine.
 * * @return EXIT_SUCCESS on clean shutdown, EXIT_FAILURE on critical exception.
 */
int main() {
    // 1. Return Code Initialization
    int returnCode = EXIT_SUCCESS;

    try {
        // 2. Centralized Window Initialization Constants
        static constexpr uint32_t WINDOW_WIDTH = 1280U;
        static constexpr uint32_t WINDOW_HEIGHT = 720U;
        static constexpr char const* WINDOW_TITLE = "Vulkan Lab: Sandy-Snow Globe (Audited)";

        // 3. Initialize the EngineOrchestrator
        // RAII: The 'app' object owns all sub-systems. Construction handles
        // the full Vulkan handshake and asset loading sequence.
        EngineOrchestrator app(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE);

        // 4. Execution
        // Enters the primary OS message loop and simulation update cycle.
        app.run();
    }
    catch (const std::exception& e) {
        // 5. High-Integrity Exception Handling
        // Mandatory bracing for audit compliance and clear failure reporting.
        std::cerr << std::endl << "[CRITICAL ENGINE FAILURE]" << std::endl;
        std::cerr << "Location: main.cpp" << std::endl;
        std::cerr << "Details:  " << e.what() << std::endl << std::endl;

        // Note: RAII destructors for the 'app' object will fire automatically 
        // upon exiting the try block, ensuring partial GPU resources are released.
        returnCode = EXIT_FAILURE;
    }
    catch (...) {
        // Fallback for non-standard exceptions to prevent silent crashes.
        std::cerr << "Fatal Error: An unknown exception occurred." << std::endl;
        returnCode = EXIT_FAILURE;
    }

    return returnCode;
}