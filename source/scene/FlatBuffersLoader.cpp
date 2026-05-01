/* parasoft-begin-suppress ALL */
#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>
/* parasoft-end-suppress ALL */

// Windows API for file dialog
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commdlg.h>

#include "scene/FlatBuffersLoader.h"
#include "core/Logger.h"

namespace GE::Scene {

// ===========================================================================
// SECTION 1: openFileDialog()
// ===========================================================================

std::string FlatBuffersLoader::openFileDialog(const std::string& initialDir) {
    char szFile[MAX_PATH] = {};

    // Resolve to absolute path — lpstrInitialDir must be absolute and must
    // outlive the OPENFILENAME struct, so store it in a local std::string.
    std::string absDir;
    if (!initialDir.empty()) {
        std::error_code ec;
        absDir = std::filesystem::absolute(initialDir, ec).string();
    }

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize     = sizeof(ofn);
    ofn.hwndOwner       = nullptr;
    ofn.lpstrFile       = szFile;
    ofn.nMaxFile        = sizeof(szFile);
    ofn.lpstrFilter     = "FlatBuffers Binary Files\0*.bin\0All Files\0*.*\0";
    ofn.nFilterIndex    = 1;
    ofn.lpstrTitle      = "Select FlatBuffers Scene";
    ofn.lpstrInitialDir = absDir.empty() ? nullptr : absDir.c_str();
    ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    return "";
}

// ===========================================================================
// SECTION 2: copyBin()
// ===========================================================================

std::string FlatBuffersLoader::copyBin(const std::string& binPath,
                                        const std::string& outDir)
{
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    if (ec) {
        GE_LOG_ERROR("FlatBuffersLoader: Failed to create output dir: " + outDir);
        return "";
    }

    const std::filesystem::path dest =
        std::filesystem::path(outDir) / std::filesystem::path(binPath).filename();

    std::filesystem::copy_file(binPath, dest,
        std::filesystem::copy_options::overwrite_existing, ec);

    if (ec) {
        GE_LOG_ERROR("FlatBuffersLoader: Failed to copy .bin: " + ec.message());
        return "";
    }

    return dest.string();
}

// ===========================================================================
// SECTION 3: pickAndPrepare()
// ===========================================================================

std::string FlatBuffersLoader::pickAndPrepare(const std::string& outDir) {
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    const std::string selected = openFileDialog(outDir);
    if (selected.empty()) { return ""; }

    return copyBin(selected, outDir);
}

// ===========================================================================
// SECTION 4: scanDirectory()
// ===========================================================================

std::vector<std::string> FlatBuffersLoader::scanDirectory(const std::string& dir) {
    std::vector<std::string> results;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) { break; }
        if (entry.path().extension() == ".bin") {
            results.push_back(entry.path().string());
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

} // namespace GE::Scene
