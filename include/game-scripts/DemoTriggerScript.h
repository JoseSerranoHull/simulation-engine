#pragma once

/**
 * @file DemoTriggerScript.h
 * @brief Example trigger-volume script — demonstrates isTrigger colliders.
 *
 * HOW TO USE
 * ----------
 * Create a sphere entity with isTrigger = true and attach this script:
 *
 *      GE::ECS::EntityID zoneID = em->CreateEntity();
 *      em->AddComponent<GE::Components::Transform>(zoneID,
 *          []{ GE::Components::Transform t; t.m_position = {3.0f, 0.5f, 0.0f}; return t; }());
 *      em->AddComponent<GE::Components::Tag>(zoneID, GE::Components::Tag{"TriggerZone"});
 *
 *      GE::Components::SphereCollider sc;
 *      sc.radius    = 1.5f;
 *      sc.isTrigger = true;      // <-- this makes it a trigger (no impulse, OnTrigger* fires)
 *      em->AddComponent<GE::Components::SphereCollider>(zoneID, sc);
 *
 *      em->AddComponent<GE::Components::ScriptComponent>(
 *          zoneID,
 *          GE::Components::ScriptComponent{ std::make_shared<DemoTriggerScript>() }
 *      );
 *
 * When the player sphere overlaps this zone, OnTriggerEnter/Exit fire on BOTH scripts.
 */

#include "scripts/GameScriptComponent.h"
#include "core/Logger.h"

class DemoTriggerScript : public GE::Scripts::GameScriptComponent {
public:
    // Inspector fields
    bool  isArmed    { true };       ///< When false, suppress trigger callbacks
    int   enterCount { 0 };          ///< How many times something entered this zone
    glm::vec3 zoneColor { 0.0f, 1.0f, 0.0f };  ///< Visual hint (inspector display only)

    void OnDrawInspector() override {
        GameScriptComponent::OnDrawInspector();
        DrawField("Armed",      isArmed);
        DrawField("Enter Count", enterCount);
        DrawField("Zone Color",  zoneColor);
    }

    void Awake() override {
        GE_LOG_INFO("[DemoTriggerScript] Trigger zone armed on entity "
            + std::to_string(GetEntityID()));
    }

    void OnTriggerEnter(GE::ECS::EntityID other) override {
        if (!isArmed) return;
        ++enterCount;
        GE_LOG_INFO("[DemoTriggerScript] Entity " + std::to_string(other)
            + " entered zone! (total entries: " + std::to_string(enterCount) + ")");
    }

    void OnTriggerExit(GE::ECS::EntityID other) override {
        if (!isArmed) return;
        GE_LOG_INFO("[DemoTriggerScript] Entity " + std::to_string(other)
            + " exited zone.");
    }
};
