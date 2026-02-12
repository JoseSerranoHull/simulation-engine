#pragma once

/**
 * @class ISystem
 * @brief Base interface for all high-level engine systems.
 */
class ISystem {
public:
    virtual ~ISystem() = default;

    // Standard lifecycle hooks for any engine system
    virtual void update(float deltaTime) = 0;
};