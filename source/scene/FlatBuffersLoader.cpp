/* parasoft-begin-suppress ALL */
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
/* parasoft-end-suppress ALL */

// Windows API for file dialog and process creation
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
    ofn.lpstrFilter     = "FlatBuffers Files\0*.bin;*.fbs;*.json\0All Files\0*.*\0";
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
// SECTION 2: findFlatc()
// ===========================================================================

std::string FlatBuffersLoader::findFlatc() {
    // Priority 1: user-editable config file
    {
        std::ifstream cfg("./config/flatbufferConfig/flatc_path.txt");
        if (cfg.is_open()) {
            std::string line;
            if (std::getline(cfg, line) && !line.empty()) {
                // Trim trailing carriage return if present
                if (!line.empty() && line.back() == '\r') { line.pop_back(); }
                if (std::filesystem::exists(line)) {
                    return line;
                }
            }
        }
    }

    // Priority 2: known location based on USERNAME
    char* username = nullptr;
    std::size_t usernameLen = 0U;
    _dupenv_s(&username, &usernameLen, "USERNAME");
    if (username != nullptr) {
        std::string candidate = std::string("C:\\Users\\") + username +
                                "\\flatbuffers\\Release\\flatc.exe";
        free(username);
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    GE_LOG_WARN("FlatBuffersLoader: flatc.exe not found — binary-only mode.");
    return "";
}

// ===========================================================================
// SECTION 3: compileFbJson()
// ===========================================================================

std::string FlatBuffersLoader::compileFbJson(const std::string& jsonPath,
                                              const std::string& outDir)
{
    const std::string flatcPath = findFlatc();
    if (flatcPath.empty()) {
        GE_LOG_ERROR("FlatBuffersLoader: Cannot compile .json — flatc not found.");
        return "";
    }

    // Ensure output directory exists
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);
    if (ec) {
        GE_LOG_ERROR("FlatBuffersLoader: Failed to create output dir: " + outDir);
        return "";
    }

    // Build command line: flatc --binary -o outDir ./flatbuffers/Scene.fbs jsonPath
    // Note: CreateProcess modifies the command string, so use a non-const buffer
    const std::string cmd = "\"" + flatcPath + "\" --binary -o \"" + outDir +
                            "\" ./flatbuffers/Scene.fbs \"" + jsonPath + "\"";

    std::vector<char> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back('\0');

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    const BOOL created = CreateProcessA(
        nullptr,
        cmdBuf.data(),
        nullptr, nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr, nullptr,
        &si, &pi);

    if (!created) {
        GE_LOG_ERROR("FlatBuffersLoader: CreateProcess failed for flatc.");
        return "";
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exitCode = 0U;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (exitCode != 0U) {
        GE_LOG_ERROR("FlatBuffersLoader: flatc exited with code " +
                     std::to_string(static_cast<int>(exitCode)));
        return "";
    }

    // Derive the output .bin path: outDir/<stem>.bin
    const std::filesystem::path stem =
        std::filesystem::path(jsonPath).stem();
    const std::string destPath =
        (std::filesystem::path(outDir) / (stem.string() + ".bin")).string();

    return destPath;
}

// ===========================================================================
// SECTION 4: copyBin()
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
// SECTION 5: pickAndPrepare()
// ===========================================================================

std::string FlatBuffersLoader::pickAndPrepare(const std::string& outDir) {
    // Ensure managed directory exists before opening dialog
    std::error_code ec;
    std::filesystem::create_directories(outDir, ec);

    const std::string selected = openFileDialog(outDir);
    if (selected.empty()) { return ""; }

    const std::string ext = std::filesystem::path(selected).extension().string();

    if (ext == ".json" || ext == ".JSON") {
        return compileFbJson(selected, outDir);
    }

    // .bin or anything else — copy to managed dir
    return copyBin(selected, outDir);
}

// ===========================================================================
// SECTION 6: scanDirectory()
// ===========================================================================

std::vector<std::string> FlatBuffersLoader::scanDirectory(const std::string& dir) {
    std::vector<std::string> results;

    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) { break; }
        const auto ext = entry.path().extension();
        if (ext == ".bin" || ext == ".fbs") {
            results.push_back(entry.path().string());
        }
    }

    std::sort(results.begin(), results.end());
    return results;
}

} // namespace GE::Scene
