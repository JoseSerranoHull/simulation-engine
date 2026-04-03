#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <vector>
/* parasoft-end-suppress ALL */

namespace GE::Scene {

    /**
     * @class FlatBuffersLoader
     * @brief Static utility that drives the full "pick → compile → load" workflow for
     *        FlatBuffers scene binaries. Windows-only (GetOpenFileNameA, CreateProcess).
     *
     *        Workflow:
     *          1. openFileDialog() — Windows native open-file dialog filtered to .bin/.json
     *          2. If .json selected: compileFbJson() runs flatc to produce .bin in outDir
     *             If .bin selected: copyBin() copies the file to outDir
     *          3. Caller receives the final .bin path and calls requestScenarioChange()
     *
     *        flatc discovery order:
     *          a. First line of ./config/flatbufferConfig/flatc_path.txt (user-editable)
     *          b. %USERPROFILE%\flatbuffers\Release\flatc.exe (discovered location)
     *          c. Returns "" → binary-only mode (no .json compilation)
     */
    class FlatBuffersLoader {
    public:
        /**
         * @brief Opens a Windows file dialog, prepares the selected file, and returns
         *        the destination .bin path in outDir.
         * @param outDir Managed directory to receive the prepared binary.
         * @return Full path of the ready-to-load .bin, or "" on cancel/error.
         */
        static std::string pickAndPrepare(const std::string& outDir);

        /**
         * @brief Scans dir for *.bin files.
         * @return Sorted list of full absolute-style paths.
         */
        static std::vector<std::string> scanDirectory(const std::string& dir);

    private:
        // Opens GetOpenFileNameA filtered to *.bin;*.json. Returns path or "".
        static std::string openFileDialog();

        // Discovers flatc.exe using the priority order documented above.
        static std::string findFlatc();

        // Runs flatc --binary to compile jsonPath into outDir.
        // Returns destination .bin path on success, "" on failure.
        static std::string compileFbJson(const std::string& jsonPath,
                                          const std::string& outDir);

        // Copies an existing .bin file into outDir (overwrites if present).
        // Returns destination path on success, "" on failure.
        static std::string copyBin(const std::string& binPath,
                                    const std::string& outDir);
    };

} // namespace GE::Scene
