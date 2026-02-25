#pragma once
#include "ecs/IECSystem.h"
#include "ecs/EntityManager.h"
#include <glm/glm.hpp>

namespace GE::Systems {

    /**
     * @enum IntegrationMethod
     * @brief Selects the numerical integration scheme used each physics tick.
     * Fulfills Simulation Lab 3 Q2 requirement for at least two methods.
     */
    enum class IntegrationMethod {
        Euler,          ///< Explicit (Forward) Euler — position updated before velocity
        SemiImplicit,   ///< Symplectic (Semi-Implicit) Euler — velocity updated first (default)
        RK4             ///< 4th-order Runge-Kutta — highest accuracy, best for validation
    };

    /**
     * @class PhysicsSystem
     * @brief ECS System responsible for Newtonian integration and collision resolution.
     * Fulfills Simulation Lab 3 Requirements Q2, Q3, and Q4.
     */
    class PhysicsSystem : public ECS::ICpuSystem {
    public:
        PhysicsSystem() {
            m_typeID = ECS::IECSystem::GetUniqueISystemTypeID<PhysicsSystem>();
            m_stage  = ECS::ESystemStage::Physics;
            m_state  = SystemState::Running;
        }

        ~PhysicsSystem() override = default;

        /** @brief CPU-only: orchestrates Newtonian integration then collision resolution. */
        void OnUpdate(float dt) override;

        ERROR_CODE Shutdown() override {
            m_state = SystemState::ShuttingDown;
            return ERROR_CODE::OK;
        }

        /** @brief Integration method selected at runtime via ImGui. Public for OnGUI access. */
        IntegrationMethod m_integrationMethod{ IntegrationMethod::SemiImplicit };

    private:
        /** @brief Internal derivative state used by the RK4 integrator. */
        struct Derivative { glm::vec3 dPos{ 0.0f }; glm::vec3 dVel{ 0.0f }; };

        /** @brief Fulfills Q2 & Q3: Accumulates forces and integrates velocity/position. */
        void Integrate(float dt);

        /** @brief RK4 helper — integrates pos and vel over dt given constant acceleration. */
        static void IntegrateRK4(glm::vec3& pos, glm::vec3& vel,
                                  const glm::vec3& accel, float dt);

        /** @brief Fulfills Q4: Detects and resolves sphere-plane and sphere-sphere intersections. */
        void ResolveCollisions();
    };
}
