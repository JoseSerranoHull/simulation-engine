#include "SceneLoader.h"
#include "GeometryUtils.h"

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
        auto getTex = [&](const std::string& key) {
            return (props.count(key) && m_textures.count(props.at(key))) ? m_textures[props.at(key)] : nullptr;
            };

        int pipeIdx = props.count("Pipeline") ? std::stoi(props.at("Pipeline")) : 0;

        m_materials[id] = am->createMaterial(
            getTex("Albedo"), getTex("Normal"), getTex("AO"),
            getTex("Metallic"), getTex("Roughness"), pipelines[pipeIdx].get()
        );
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
        auto mat = m_materials.count(props.at("Material")) ? m_materials[props.at("Material")] : nullptr;
        mr.material = mat.get();
        mr.castShadows = (props.count("CastShadows") && props.at("CastShadows") == "true");

        if (props.count("Type") && props.at("Type") == "procedural") {
            std::string shape = props.at("Shape");
            OBJLoader::MeshData data;
            if (shape == "Plane") data = GeometryUtils::generatePlane(parseFloat(props.at("Width")), parseFloat(props.at("Depth")));
            else if (shape == "Capsule") data = GeometryUtils::generateCapsule(parseFloat(props.at("Radius")), parseFloat(props.at("Height")), 32, 16);

            // Transfer geometry to GPU and keep reference
            mr.mesh = am->processMeshData(data, mat, cmd, sb, sm).release();
        }
        else if (props.count("Mesh")) {
            auto model = am->loadModel(props.at("Mesh"), [&](const std::string&) { return mat; }, cmd, sb, sm);
            if (!model->getMeshes().empty()) {
                mr.mesh = model->getMeshes()[0].get();
            }
            outOwnedModels.push_back(std::move(model));
        }

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