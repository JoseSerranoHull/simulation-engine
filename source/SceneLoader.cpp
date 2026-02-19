#include "../include/SceneLoader.h"
#include "../include/GeometryUtils.h"
#include "../include/PhysicsComponents.h"
#include "../include/SystemFactory.h"
#include "../include/Experience.h"

/* parasoft-begin-suppress ALL */
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include "../include/ParticleComponent.h"
#include "../include/SkyboxComponent.h"
/* parasoft-end-suppress ALL */

class ParticleSystem;

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
            else if (currentSection == "LightComponent") handleLightComponent(sectionProps, em);
            else if (currentSection == "RigidBody") handleRigidBody(sectionProps, em);
            else if (currentSection == "SphereCollider") handleSphereCollider(sectionProps, em);
            else if (currentSection == "PlaneCollider") handlePlaneCollider(sectionProps, em);
            else if (currentSection == "ParticleComponent") handleParticleComponent(sectionProps, em);
            else if (currentSection == "SkyboxComponent") handleSkyboxComponent(sectionProps, em);
            else if (currentSection == "BackgroundComponent") handleBackgroundComponent(sectionProps, em);

            sectionProps.clear();
            };

        while (std::getline(file, line)) {
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

                // 1. Trim both sides of the key
                key.erase(0, key.find_first_not_of(" \t\r\n"));
                size_t lastKey = key.find_last_not_of(" \t\r\n");
                if (lastKey != std::string::npos) key.erase(lastKey + 1);

                // 2. Trim both sides of the value (CRITICAL)
                val.erase(0, val.find_first_not_of(" \t\r\n"));
                size_t lastVal = val.find_last_not_of(" \t\r\n");
                if (lastVal != std::string::npos) val.erase(lastVal + 1);

                sectionProps[key] = val;
            }
        }
        processSection();
    }

    void SceneLoader::handleTexture(const std::string& id, const std::map<std::string, std::string>& props, AssetManager* am) {
        if (props.count("Path")) {
            std::string path = props.at("Path");

            // 1. Store the raw path for the Skybox registry
            m_texturePaths[id] = path;

            // 2. Load the actual GPU texture object for standard materials
            m_textures[id] = am->loadTexture(path);
        }
    }

    void GE::Scene::SceneLoader::handleMaterial(const std::string& id, const std::map<std::string, std::string>& props, AssetManager* am, const std::vector<std::unique_ptr<Pipeline>>& pipelines) {

        // Helper to fetch texture from registry or fallback
        auto getSafeTex = [&](const std::string& key, const std::string& fallbackID) {
            if (props.count(key) && m_textures.count(props.at(key))) {
                return m_textures[props.at(key)];
            }
            return m_textures.count(fallbackID) ? m_textures[fallbackID] : nullptr;
            };

        // Determine which pipeline index to use (0 = Opaque, 1 = Checkerboard)
        int pipeIdx = props.count("Pipeline") ? std::stoi(props.at("Pipeline")) : 0;

        // UNIFIED CREATION: Procedural materials no longer ignore custom textures
        auto mat = am->createMaterial(
            getSafeTex("Albedo", "White"),
            getSafeTex("Normal", "Placeholder"),
            getSafeTex("AO", "White"),
            getSafeTex("Metallic", "Black"),
            getSafeTex("Roughness", "Matte"),
            pipelines[pipeIdx].get()
        );

        // Common properties
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

        // Standard properties
        if (props.count("Position")) trans.m_position = parseVec3(props.at("Position"));
        if (props.count("Rotation")) trans.m_rotation = parseVec3(props.at("Rotation"));
        if (props.count("Scale"))    trans.m_scale = parseVec3(props.at("Scale"));

        // NEW: Agnostic Hierarchy Linking
        if (props.count("Parent")) {
            std::string parentName = props.at("Parent");
            // We use the Scene registry to find the EntityID by name
            auto* scene = ServiceLocator::GetScene();
            if (scene->hasEntity(parentName)) {
                trans.m_parentEntityID = scene->getEntityID(parentName);
            }
        }

        trans.m_state = GE::Scene::Components::Transform::TransformState::Dirty;
        em->AddComponent(m_currentEntity, trans);
    }

    void SceneLoader::handleMeshRenderer(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, AssetManager* am, VkCommandBuffer cmd, std::vector<VkBuffer>& sb, std::vector<VkDeviceMemory>& sm, std::vector<std::unique_ptr<Model>>& outOwnedModels) {
        GE::Components::MeshRenderer mr;

        // Step 1: Resolve the default material for this renderer
        std::shared_ptr<Material> defaultMat = nullptr;
        if (props.count("Material") && m_materials.count(props.at("Material"))) {
            defaultMat = m_materials.at(props.at("Material"));
        }

        // --- CASE A: Procedural Geometry (Fulfills Lab Primitives Requirement) ---
        if (props.count("Type") && props.at("Type") == "procedural") {
            const std::string shape = props.at("Shape");
            OBJLoader::MeshData data;

            if (shape == "Plane") {
                data = GeometryUtils::generatePlane(parseFloat(props.at("Width")), parseFloat(props.at("Depth")));
            }
            else if (shape == "Sphere") {
                // Standard Sphere: 32 segments, radius 1.0, full sphere (cutoff -1.0)
                data = GeometryUtils::generateSphere(32U, 1.0f, -1.0f, glm::vec3(1.0f));
            }
            else if (shape == "Cylinder") {
                // Standard Cylinder: 32 segments, bottom/top radius 1.0, height 2.0
                data = GeometryUtils::generateCylinder(32U, 1.0f, 1.0f, 2.0f, glm::vec3(1.0f));
            }
            else if (shape == "SandPlug") {
                // Specialized Snow Globe Terrain: 64 segments for high-res dunes
                data = GeometryUtils::generateSandPlug(64U, 1.0f, 0.9f, 0.5f, glm::vec3(1.0f));
            }
            else if (shape == "Capsule") {
                data = GeometryUtils::generateCapsule(parseFloat(props.at("Radius")), parseFloat(props.at("Height")), 32, 16);
            }

            // Process data into a GPU Mesh using your AssetManager
            auto meshPtr = am->processMeshData(data, defaultMat, cmd, sb, sm);

            if (meshPtr) {
                // To maintain memory safety, we wrap the procedural mesh in a Model object
                auto dummyModel = std::make_unique<Model>();
                Mesh* rawMeshPtr = meshPtr.get();
                dummyModel->addMesh(std::move(meshPtr));

                mr.subMeshes.push_back({ rawMeshPtr, defaultMat.get() });
                outOwnedModels.push_back(std::move(dummyModel));
            }
            else {
                GE_LOG_ERROR("SceneLoader: Failed to generate procedural shape: " + shape);
            }
        }
        // --- CASE B: Complex .obj Model with Material Mapping ---
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

                        // Clean whitespace from keys/values
                        mName.erase(0, mName.find_first_not_of(" \t"));
                        mName.erase(mName.find_last_not_of(" \t") + 1);
                        mMat.erase(0, mMat.find_first_not_of(" \t"));
                        mMat.erase(mMat.find_last_not_of(" \t") + 1);

                        matLookup[mName] = mMat;
                    }
                }
            }

            // 2. Define the selector lambda to match sub-meshes to materials
            auto selector = [&](const std::string& meshName) -> std::shared_ptr<Material> {
                std::string lowerMesh = meshName;
                std::transform(lowerMesh.begin(), lowerMesh.end(), lowerMesh.begin(), ::tolower);

                for (auto const& [key, matID] : matLookup) {
                    std::string lowerKey = key;
                    std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);

                    if (lowerMesh.find(lowerKey) != std::string::npos) {
                        return m_materials.count(matID) ? m_materials.at(matID) : defaultMat;
                    }
                }
                return defaultMat; // Fallback to base material
                };

            // 3. Load the complex model using the AssetManager pipeline
            auto model = am->loadModel(props.at("Mesh"), selector, cmd, sb, sm);

            if (model) {
                for (auto& meshPtr : model->getMeshes()) {
                    GE::Components::SubMesh sub;
                    sub.mesh = meshPtr.get();
                    sub.material = meshPtr->getMaterial();
                    mr.subMeshes.push_back(sub);
                }
                outOwnedModels.push_back(std::move(model));
            }
            else {
                GE_LOG_ERROR("SceneLoader: Failed to load external mesh: " + props.at("Mesh"));
            }
        }

        // Step 4: Finalize by attaching the MeshRenderer component to the ECS entity
        em->AddComponent(m_currentEntity, mr);
    }
    void SceneLoader::handleTag(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em, GE::Scene::Scene* scene) {
        GE::Scene::Components::Tag tag;
        tag.m_name = props.count("Name") ? props.at("Name") : "Unnamed Entity";
        em->AddComponent(m_currentEntity, tag);
        scene->addEntity(tag.m_name, m_currentEntity);
    }

    void SceneLoader::handleLightComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::LightComponent light;
        if (props.count("Color")) { light.color = parseVec3(props.at("Color")); }
        if (props.count("Intensity")) { light.intensity = parseFloat(props.at("Intensity")); }
        if (props.count("IsStatic")) { light.isStatic = (props.at("IsStatic") == "true"); }

        em->AddComponent(m_currentEntity, light);
    }

    void SceneLoader::handleRigidBody(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::RigidBody rb;
        if (props.count("Mass")) rb.mass = parseFloat(props.at("Mass"));
        if (props.count("Static")) rb.isStatic = (props.at("Static") == "true");
        if (props.count("Velocity")) rb.velocity = parseVec3(props.at("Velocity"));
        if (props.count("UseGravity")) rb.useGravity = (props.at("UseGravity") == "true");
        if (props.count("Restitution")) rb.restitution = parseFloat(props.at("Restitution"));

        em->AddComponent(m_currentEntity, rb);
    }

    void SceneLoader::handleSphereCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::SphereCollider sc;
        if (props.count("Radius")) sc.radius = parseFloat(props.at("Radius"));
        em->AddComponent(m_currentEntity, sc);
    }

    void SceneLoader::handlePlaneCollider(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::PlaneCollider pc;
        // Normal defaults to up (0,1,0) if not specified
        if (props.count("Normal")) pc.normal = parseVec3(props.at("Normal"));
        if (props.count("Offset")) pc.offset = parseFloat(props.at("Offset"));
        em->AddComponent(m_currentEntity, pc);
    }

    void SceneLoader::handleParticleComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        auto* ctx = ServiceLocator::GetContext();
        auto* exp = ServiceLocator::GetExperience();

        // 1. Validate Mandatory Agnostic Data
        // We require all three shader paths to build a functional system
        if (!props.count("ComputeShader") || !props.count("VertexShader") || !props.count("FragmentShader")) {
            GE_LOG_ERROR("SceneLoader: ParticleComponent missing mandatory shader paths. Skipping entity.");
            return;
        }

        // 2. Fetch dependencies required by the ParticleSystem constructor
        VkRenderPass transPass = exp->GetPostProcessor()->getTransparentRenderPass();
        VkDescriptorSetLayout globalLayout = ctx->globalSetLayout;
        VkSampleCountFlagBits msaa = ctx->msaaSamples;

        // 3. Extract configuration from .ini
        std::string comp = props.at("ComputeShader");
        std::string vert = props.at("VertexShader");
        std::string frag = props.at("FragmentShader");

        // Optional parameters with safe defaults
        uint32_t maxP = props.count("MaxParticles") ? std::stoul(props.at("MaxParticles")) : 1000U;
        glm::vec3 spawn = props.count("SpawnPos") ? parseVec3(props.at("SpawnPos")) : glm::vec3(0.0f);

        // 4. AGNOSTIC BUILDER CALL
        // The constructor handles the heavy lifting of GPU pipeline creation
        auto system = std::make_unique<ParticleSystem>(
            transPass,
            globalLayout,
            comp,
            vert,
            frag,
            spawn,
            maxP,
            msaa
        );

        // 5. Wrap in ECS Component
        GE::Components::ParticleComponent pc;
        pc.system = std::move(system);
        pc.enabled = (props.count("Enabled") && props.at("Enabled") == "true");

        // localOffset is used by ParticleEmitterSystem to position particles relative to parent
        pc.localOffset = props.count("Offset") ? parseVec3(props.at("Offset")) : glm::vec3(0.0f);

        em->AddComponent(m_currentEntity, pc);

        // Metadata logging using the 'Type' key if present
        std::string logType = props.count("Type") ? props.at("Type") : "Generic";
        GE_LOG_INFO("SceneLoader: Successfully built " + logType + " particle system.");
    }

    void SceneLoader::handleSkyboxComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::SkyboxComponent sc;
        auto* exp = ServiceLocator::GetExperience();

        const std::vector<std::string> keys = { "PX", "NX", "PY", "NY", "PZ", "NZ" };
        std::vector<std::string> facePaths;

        for (const auto& key : keys) {
            if (props.count(key)) {
                std::string texID = props.at(key);

                // Now this check will succeed!
                if (m_texturePaths.count(texID)) {
                    facePaths.push_back(m_texturePaths.at(texID));
                }
            }
        }

        // Trigger the GPU update using the gathered filenames
        if (facePaths.size() == 6U && exp->GetSkybox() != nullptr) {
            exp->GetSkybox()->loadTextures(facePaths);
        }

        sc.enabled = (props.count("Enabled") && props.at("Enabled") == "true");
        em->AddComponent(m_currentEntity, sc);
    }

    void SceneLoader::handleBackgroundComponent(const std::map<std::string, std::string>& props, GE::ECS::EntityManager* em) {
        GE::Components::BackgroundComponent bg;
        if (props.count("Color")) { bg.color = parseVec3(props.at("Color")); }
        em->AddComponent(m_currentEntity, bg);
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
