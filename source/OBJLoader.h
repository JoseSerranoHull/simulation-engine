#pragma once

/* parasoft-begin-suppress ALL */
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <map>
#include <glm/glm.hpp>
/* parasoft-end-suppress ALL */

#include "Vertex.h"
#include "Common.h"

/**
 * @namespace OBJLoader
 * @brief Static utility for parsing Wavefront (.obj) files into GPU-ready mesh data.
 * Supports multi-object files and automatic triangulation of polygons.
 */
namespace OBJLoader {

    struct MeshData {
        std::string name{ "unnamed_mesh" };
        std::vector<std::string> groupNames{};
        std::vector<Vertex> vertices{};
        std::vector<uint32_t> indices{};
    };

    // --- Named Constants for OBJ Parsing ---
    static constexpr uint32_t TRI_VERT_COUNT = 3U;
    static constexpr uint32_t OBJ_INDEX_COUNT = 3U;
    static constexpr int32_t  OBJ_BASE_OFFSET = 1;
    static constexpr size_t   INDEX_FIRST = 0U;
    static constexpr char     CHAR_COMMENT = '#';
    static constexpr char     CHAR_DELIMITER = '/';
    static constexpr float    FLOAT_ONE = 1.0f;
    static constexpr int32_t  INVALID_INDEX = -1;
    static constexpr int32_t  OBJ_MIN_INDEX = 0;

    // Indices for the PTN (Position, Texture, Normal) face segments
    static constexpr uint32_t IDX_POS = 0U;
    static constexpr uint32_t IDX_TEX = 1U;
    static constexpr uint32_t IDX_NORM = 2U;

    // Command Prefixes
    static const std::string CMD_VERTEX = "v";
    static const std::string CMD_TEXCOORD = "vt";
    static const std::string CMD_NORMAL = "vn";
    static const std::string CMD_FACE = "f";
    static const std::string CMD_OBJECT = "o";
    static const std::string CMD_GROUP = "g";

    /**
     * @brief Parses an OBJ file and triangulates polygons into a vector of MeshData.
     * @param fileName Path to the .obj file.
     * @return A collection of meshes found within the file.
     */
    static std::vector<MeshData> loadOBJ(const char* const fileName)
    {
        std::vector<MeshData> loadedMeshes{};

        // 1. PERFORM SAFETY CHECK FIRST
        std::ifstream file(fileName);
        if (!file.is_open()) {
            std::cerr << "OBJLoader: Error opening file -> " << fileName << "\n";
            return loadedMeshes;
        }

        // 2. POSTPONE DEFINITIONS: Only initialize resources after file is confirmed open
        // Global attributes
        std::vector<glm::vec3> attrib_positions{};
        std::vector<glm::vec2> attrib_texcoords{};
        std::vector<glm::vec3> attrib_normals{};

        // Per-object transient data
        std::vector<Vertex> currentVertices{};
        std::vector<uint32_t> currentIndices{};
        std::vector<std::string> currentGroupNames{};
        std::string currentName{ "unnamed_group" };

        // Vertex Cache (A std::map is expensive to construct; postpone it!)
        std::map<std::string, uint32_t> vertexCache{};

        // 3. BEGIN PARSING
        std::string line{ "" };
        while (std::getline(file, line)) {
            if (line.empty()) {
                continue;
            }

            std::stringstream ss(line);
            std::string prefix{ "" };
            ss >> prefix;

            // 1. Skip comments or empty streams
            if (!ss || (prefix[INDEX_FIRST] == CHAR_COMMENT)) {
                continue;
            }

            // 2. Parse Geometric Attributes
            if (prefix == CMD_VERTEX) {
                glm::vec3 pos{ 0.0f, 0.0f, 0.0f };
                ss >> pos.x >> pos.y >> pos.z;
                attrib_positions.push_back(pos);
            }
            else if (prefix == CMD_TEXCOORD) {
                glm::vec2 tex{ 0.0f, 0.0f };
                ss >> tex.x >> tex.y;
                // Vulkan's (0,0) is top-left; OBJ (0,0) is bottom-left. Invert V coordinate.
                tex.y = FLOAT_ONE - tex.y;
                attrib_texcoords.push_back(tex);
            }
            else if (prefix == CMD_NORMAL) {
                glm::vec3 norm{ 0.0f, 0.0f, 0.0f };
                ss >> norm.x >> norm.y >> norm.z;
                attrib_normals.push_back(norm);
            }
            // 3. Handle Object/Group Switching (Logic for sub-meshes)
            else if ((prefix == CMD_OBJECT) || (prefix == CMD_GROUP)) {
                if (!currentVertices.empty()) {
                    loadedMeshes.push_back({ currentName, currentGroupNames, currentVertices, currentIndices });
                    currentVertices.clear();
                    currentIndices.clear();
                    vertexCache.clear();
                }
                ss >> currentName;
                if (prefix == CMD_GROUP) {
                    currentGroupNames.clear();
                    currentGroupNames.push_back(currentName);
                }
            }
            // 4. Triangulate Faces (Polygon to Triangle conversion)
            else if (prefix == CMD_FACE) {
                std::vector<std::string> vertexStrings{};
                std::string segment{ "" };
                while (ss >> segment) {
                    vertexStrings.push_back(segment);
                }

                // Simple fan triangulation for polygons with > 3 vertices
                const size_t vertCount = vertexStrings.size();
                for (size_t i = 1U; i < (vertCount - 1U); ++i) {
                    const std::string faceVerts[TRI_VERT_COUNT] = {
                        vertexStrings[INDEX_FIRST],
                        vertexStrings[i],
                        vertexStrings[i + 1U]
                    };

                    for (uint32_t v = 0U; v < TRI_VERT_COUNT; ++v) {
                        const std::string& vertString = faceVerts[v];

                        // Index Reuse: Check if this vertex configuration already exists
                        if (vertexCache.count(vertString) > INDEX_FIRST) {
                            currentIndices.push_back(vertexCache[vertString]);
                        }
                        else {
                            Vertex newVert{};
                            newVert.color = glm::vec3(FLOAT_ONE); // Default white tint

                            std::stringstream vss(vertString);
                            std::string indexStr{ "" };
                            int32_t ptnIndices[OBJ_INDEX_COUNT] = { INVALID_INDEX, INVALID_INDEX, INVALID_INDEX };
                            uint32_t segmentCount = 0U;

                            // Split the "pos/tex/norm" string by the '/' delimiter
                            while (std::getline(vss, indexStr, CHAR_DELIMITER) && (segmentCount < OBJ_INDEX_COUNT)) {
                                if (!indexStr.empty()) {
                                    ptnIndices[segmentCount] = std::stoi(indexStr);
                                }
                                ++segmentCount;
                            }

                            // OBJ indices are 1-based; convert to 0-based for vector access
                            if (ptnIndices[IDX_POS] > OBJ_MIN_INDEX) {
                                const size_t idx = static_cast<size_t>(ptnIndices[IDX_POS] - OBJ_BASE_OFFSET);
                                if (idx < attrib_positions.size()) { newVert.position = attrib_positions[idx]; }
                            }
                            if (ptnIndices[IDX_TEX] > OBJ_MIN_INDEX) {
                                const size_t idx = static_cast<size_t>(ptnIndices[IDX_TEX] - OBJ_BASE_OFFSET);
                                if (idx < attrib_texcoords.size()) { newVert.texcoord = attrib_texcoords[idx]; }
                            }
                            if (ptnIndices[IDX_NORM] > OBJ_MIN_INDEX) {
                                const size_t idx = static_cast<size_t>(ptnIndices[IDX_NORM] - OBJ_BASE_OFFSET);
                                if (idx < attrib_normals.size()) { newVert.normal = attrib_normals[idx]; }
                            }

                            currentVertices.push_back(newVert);
                            const uint32_t newIndex = static_cast<uint32_t>(currentVertices.size() - 1U);
                            currentIndices.push_back(newIndex);
                            vertexCache[vertString] = newIndex;
                        }
                    }
                }
            }
        }

        // Push the final object in the file
        if (!currentVertices.empty()) {
            loadedMeshes.push_back({ currentName, currentGroupNames, currentVertices, currentIndices });
        }

        file.close();
        return loadedMeshes;
    }
}