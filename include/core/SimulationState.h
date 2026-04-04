#pragma once

/* parasoft-begin-suppress ALL */
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
/* parasoft-end-suppress ALL */

#include <glm/glm.hpp>

namespace GE {

/**
 * @struct EntitySnapshot
 * @brief A single entity's render-relevant transform at a given physics tick.
 *        Stored in the SimulationState double-buffer so the renderer never
 *        touches ECS component arrays while the physics thread is writing them.
 */
struct EntitySnapshot {
    uint32_t  id;
    glm::mat4 worldMatrix;
};

/**
 * @struct SimulationState
 * @brief One buffer of the double-buffered simulation snapshot.
 *        The physics thread writes to the "back" instance;
 *        the render thread reads from the "front" instance.
 *        Swapping the active index is done atomically by the physics thread.
 */
struct SimulationState {
    std::vector<EntitySnapshot> snapshots;
    std::mutex                  mutex;
};

} // namespace GE
