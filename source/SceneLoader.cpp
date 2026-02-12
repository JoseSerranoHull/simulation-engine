#include "../include/SceneLoader.h"
#include "../include/GeometryUtils.h"

/* parasoft-begin-suppress ALL */
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
/* parasoft-end-suppress ALL */

namespace GE::Scene {

    void SceneLoader::load(const std::string& path, GE::ECS::EntityManager* em, AssetManager* am, GE::Scene::Scene* scene,
        const std::vector<std::unique_ptr<Pipeline>>& pipelines, VkCommandBuffer setupCmd,
        std::vector<VkBuffer>& stagingBufs, std::vector<VkDeviceMemory>& stagingMems, std::vector<std::unique_ptr<Model>>& outOwnedModels) {

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "SceneLoader: Failed to open " << path << std::endl;
            return;
        }

        std::string line, currentSection, currentID;
        std::map<std::string, std::string> sectionProps;

        // Internal lambda to dispatch section processing
        auto processSection = [&]() {
            if (currentSection.empty()) return;

            if (currentSection == "Texture") handleTexture(currentID, sectionProps, am);
            else if (currentSection == "Material") handleMaterial(currentID, sectionProps, am, pipelines);
            else if (currentSection == "Entity") handleEntity(em);
            else if (currentSection == "Transform") handleTransform(sectionProps, em);
            else if (currentSection == "MeshRenderer") handleMeshRenderer(sectionProps, em, am, setupCmd, stagingBufs, stagingMems, outOwnedModels);
            else if (currentSection == "Tag") handleTag(sectionProps, em, scene);

            sectionProps.clear();
            };

        while (std::getline(file, line)) {
            // Trim and skip comments
            line.erase(0, line.find_first_not_of(" \t\r\n"));
            line.erase(line.find_last_not_of(" \t\r\n") + 1);
            if (line.empty() || line[0] == ';' || line[0] == '/') continue;

            if (line[0] == '[') {
                processSection();
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    currentSection = line.substr(1, colon - 1);
                    currentID = line.substr(colon + 1, line.find(']') - colon - 1);
                }
                else {
                    currentSection = line.substr(1, line.find(']') - 1);
                    currentID = "";
                }
                continue;
            }

            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);

                // Trim whitespace from key/val
                key.erase(key.find_last_not_of(" ") + 1);
                val.erase(0, val.find_first_not_of(" "));

                sectionProps[key] = val;
            }
        }
        processSection(); // Process the final section
    }

    void SceneLoader::handleTexture(const std::string& id, const std::map<std::string, std::string>& props, AssetManager* am) {
        if (props.count("Path")) {
            m_textures[id] = am->loadTexture(props.at("Path"));
        }
    }

    void SceneLoader::handleMaterial(const std::string& id, const std::map<std::string, std::string>& props, AssetManager* am, const std::vector<std::unique_ptr<Pipeline>>& pipelines) {

        // Helper to get a texture or return a default fallback to prevent nullptr crashes
        auto getSafeTex = [&](const std::string& key, const std::string& fallbackID) {
            if (props.count(key) && m_textures.count(props.at(key))) {
                return m_textures[props.at(key)];
            }
            // Fallback to a guaranteed texture defined in your [Texture] registry
            return m_textures.count(fallbackID) ? m_textures[fallbackID] : nullptr;
            };

        int pipeIdx = props.count("Pipeline") ? std::stoi(props.at("Pipeline")) : 0;

        // Provide sensible defaults (White for Albedo/Metallic, Black for AO/Roughness)
        auto mat = am->createMaterial(
            getSafeTex("Albedo", "White"),
            getSafeTex("Normal", "Placeholder"),
            getSafeTex("AO", "White"),
            getSafeTex("Metallic", "Black"),
            getSafeTex("Roughness", "Matte"),
            pipelines[pipeIdx].get()
        );

        bool shadows = (props.count("CastShadows") ? (props.at("CastShadows") == "true") : true);
        mat->SetCastsShadows(shadows);

        if (props.count("Pass")) {
            std::string passVal = props.at("Pass");
            if (passVal == "Transparent") mat->SetPassType(RenderPassType::Transparent);
            else if (passVal == "ShadowOnly") mat->SetPassType(RenderPassType::ShadowOnly);
            else mat->SetPassType(RenderPassType::Opaque);
        }

        m_materials[id] = mat;
    }

    void SceneLoader::handleEntity(GE::ECS::EntityManager* em) {
        m_currentEntity = em->CreateEntity();
    }

    void SceneLoader::handleTransform(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Scene::Components::Transform trans;
        if (props.count("Position")) trans.m_position = parseVec3(props.at("Position"));
        if (props.count("Rotation")) trans.m_rotation = parseVec3(props.at("Rotation"));
        if (props.count("Scale"))    trans.m_scale = parseVec3(props.at("Scale"));

        trans.m_state = GE::Scene::Components::Transform::TransformState::Dirty;
        em->AddComponent(m_currentEntity, trans);
    }

    void SceneLoader::handleMeshRenderer(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, AssetManager* am, VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm, std::vector<std::unique_ptr<Model>>& outOwnedModels) {
        GE::Components::MeshRenderer mr;

        // Resolve a default material if provided (used as fallback or for procedural)
        std::shared_ptr<Material> defaultMat = nullptr;
        if (props.count("Material") && m_materials.count(props.at("Material"))) {
            defaultMat = m_materials.at(props.at("Material"));
        }

        // --- CASE A: Procedural Geometry ---
        if (props.count("Type") && props.at("Type") == "procedural") {
            std::string shape = props.at("Shape");
            OBJLoader::MeshData data;

            if (shape == "Plane") {
                data = GeometryUtils::generatePlane(parseFloat(props.at("Width")), parseFloat(props.at("Depth")));
            }
            else if (shape == "Capsule") {
                data = GeometryUtils::generateCapsule(parseFloat(props.at("Radius")), parseFloat(props.at("Height")), 32, 16);
            }
            // (Add other shapes like Sphere or Cylinder here as needed)

            auto meshPtr = am->processMeshData(data, defaultMat, cmd, sb, sm);

            // Create the dummy model to keep the mesh alive in Experience::ownedModels
            auto dummyModel = std::make_unique<Model>();

            Mesh* rawMeshPtr = meshPtr.get();
            dummyModel->addMesh(std::move(meshPtr));

            GE::Components::SubMesh sub;
            sub.mesh = rawMeshPtr; // Point to the mesh now owned by dummyModel
            sub.material = defaultMat.get();
            mr.subMeshes.push_back(sub);

            outOwnedModels.push_back(std::move(dummyModel));
        }
        // --- CASE B: Complex .obj Model with potential Material Map ---
        else if (props.count("Mesh")) {
            // 1. Parse the MaterialMap: "SubmeshName:MaterialID,SubmeshName:MaterialID"
            std::map<std::string, std::string> matLookup;
            if (props.count("MaterialMap")) {
                std::stringstream ss(props.at("MaterialMap"));
                std::string pair;
                while (std::getline(ss, pair, ',')) {
                    size_t colon = pair.find(':');
                    if (colon != std::string::npos) {
                        std::string mName = pair.substr(0, colon);
                        std::string mMat = pair.substr(colon + 1);

                        // Simple trim logic for cleaner parsing
                        mName.erase(0, mName.find_first_not_of(" \t"));
                        mName.erase(mName.find_last_not_of(" \t") + 1);
                        mMat.erase(0, mMat.find_first_not_of(" \t"));
                        mMat.erase(mMat.find_last_not_of(" \t") + 1);

                        matLookup[mName] = mMat;
                    }
                }
            }

            // 2. Define the selector lambda for the AssetManager
            auto selector = [&](const std::string& meshName) -> std::shared_ptr<Material> {
                std::string lowerMesh = meshName;
                std::transform(lowerMesh.begin(), lowerMesh.end(), lowerMesh.begin(), ::tolower);

                // Check if this sub-mesh name matches any key in our MaterialMap
                for (auto const& [key, matID] : matLookup) {
                    std::string lowerKey = key;
                    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

                    if (lowerMesh.find(lowerKey) != std::string::npos) {
                        return m_materials.count(matID) ? m_materials.at(matID) : defaultMat;
                    }
                }
                return defaultMat; // Fallback
                };

            // 3. Load the model using our dynamic selector
            auto model = am->loadModel(props.at("Mesh"), selector, cmd, sb, sm);

            // CRITICAL: If the model failed to load, we must stop here
            if (!model) {
                std::cerr << "SceneLoader: Failed to load mesh: " << props.at("Mesh") << std::endl;
                return;
            }

            for (auto& meshPtr : model->getMeshes()) {
                GE::Components::SubMesh sub;
                sub.mesh = meshPtr.get();
                sub.material = meshPtr->getMaterial();
                mr.subMeshes.push_back(sub);
            }
            outOwnedModels.push_back(std::move(model));
        }

        // Attach the finalized component to the entity currently being parsed
        em->AddComponent(m_currentEntity, mr);
    }

    void SceneLoader::handleTag(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, GE::Scene::Scene* scene) {
        GE::Scene::Components::Tag tag;
        tag.m_name = props.count("Name") ? props.at("Name") : "Unnamed Entity";
        em->AddComponent(m_currentEntity, tag);
        scene->addEntity(tag.m_name, m_currentEntity);
    }

    // --- Parsing Helpers ---

    glm::vec3 SceneLoader::parseVec3(const std::string& val) {
        std::stringstream ss(val);
        glm::vec3 vec;
        ss >> vec.x >> vec.y >> vec.z;
        return vec;
    }

    float SceneLoader::parseFloat(const std::string& val) {
        try { return std::stof(val); }
        catch (...) { return 0.0f; }
    }

}