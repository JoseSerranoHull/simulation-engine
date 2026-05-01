#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <vector>
/* parasoft-end-suppress ALL */

namespace GE::Scene {

    /**
     * @class FlatBuffersLoader
     * @brief Static utility for loading FlatBuffers binary scene files.
     *        Windows-only (GetOpenFileNameA).
     *
     *        Workflow:
     *          1. openFileDialog() — Windows native open-file dialog filtered to .bin
     *          2. copyBin() copies the selected file to outDir
     *          3. Caller receives the final .bin path and calls requestScenarioChange()
     */
    class FlatBuffersLoader {
    public:
        /**
         * @brief Opens a Windows file dialog, copies the selected .bin, and returns
         *        the destination path in outDir.
         * @param outDir Managed directory to receive the binary.
         * @return Full path of the ready-to-load .bin, or "" on cancel/error.
         */
        static std::string pickAndPrepare(const std::string& outDir);

        /**
         * @brief Scans dir for *.bin files.
         * @return Sorted list of full absolute-style paths.
         */
        static std::vector<std::string> scanDirectory(const std::string& dir);

    private:
        // Opens GetOpenFileNameA filtered to *.bin, starting in initialDir. Returns path or "".
        static std::string openFileDialog(const std::string& initialDir);

        // Copies an existing .bin file into outDir (overwrites if present).
        // Returns destination path on success, "" on failure.
        static std::string copyBin(const std::string& binPath,
                                    const std::string& outDir);
    };

} // namespace GE::Scene
