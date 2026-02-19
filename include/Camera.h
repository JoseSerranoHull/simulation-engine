#pragma once

/* parasoft-begin-suppress ALL */
#include "libs.h"
/* parasoft-end-suppress ALL */

/**
 * @enum CameraMovement
 * @brief Defines possible directions for camera movement.
 */
enum class CameraMovement {
    FORWARD = 0,
    BACKWARD = 1,
    LEFT = 2,
    RIGHT = 3,
    UP = 4,
    DOWN = 5
};

/**
 * @class Camera
 * @brief An Euler-angle based camera system providing 6-DOF movement and view matrix generation.
 */
class Camera final {
public:
    enum class ProjectionMode { PERSPECTIVE, ORTHOGRAPHIC };

    // --- Named Constants ---
    static constexpr float PITCH_LIMIT = 89.0f;
    static constexpr float DEFAULT_YAW = -90.0f;
    static constexpr float DEFAULT_PITCH = 0.0f;
    static constexpr float DEFAULT_SPEED = 2.5f;
    static constexpr float DEFAULT_SENSITIVITY = 0.1f;
    static constexpr float DEFAULT_ZOOM = 45.0f;

    // --- Lifecycle ---

    /**
     * @brief Constructor with specific position and look direction.
     */
    Camera(const glm::vec3& position, const glm::vec3& direction)
        : Position(position),
        WorldUp(0.0f, 1.0f, 0.0f),
        MovementSpeed(DEFAULT_SPEED),
        MouseSensitivity(DEFAULT_SENSITIVITY),
        Zoom(DEFAULT_ZOOM)
    {
        // Step 1: Initialize look direction from input
        Front = glm::normalize(direction);

        // Step 2: Derive initial Euler angles from direction vector
        Pitch = static_cast<float>(glm::degrees(std::asin(static_cast<double>(Front.y))));
        Yaw = static_cast<float>(glm::degrees(std::atan2(static_cast<double>(Front.z), static_cast<double>(Front.x))));

        // Step 3: Compute initial Right and Up basis vectors
        updateCameraVectors();
    }

    ~Camera() = default;

    // --- Core Logic Interface ---

    /** @brief Generates the View matrix used by the rendering pipelines. */
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(Position, (Position + Front), Up);
    }

    /**
     * @brief Processes keyboard input to move the camera.
     */
    void processKeyboard(const CameraMovement direction, const float deltaTime) {
        const float velocity = MovementSpeed * deltaTime;

        switch (direction) {
        case CameraMovement::FORWARD:
            Position += (Front * velocity);
            break;
        case CameraMovement::BACKWARD:
            Position -= (Front * velocity);
            break;
        case CameraMovement::LEFT:
            Position -= (Right * velocity);
            break;
        case CameraMovement::RIGHT:
            Position += (Right * velocity);
            break;
        case CameraMovement::UP:
            Position += (WorldUp * velocity);
            break;
        case CameraMovement::DOWN:
            Position -= (WorldUp * velocity);
            break;
        default:
            break;
        }
    }

    /**
     * @brief Updates Euler angles based on mouse movement.
     */
    void processMouseMovement(const float xoffset, const float yoffset, const bool constrainPitch = true) {
        // Step 1: Apply mouse offsets scaled by sensitivity
        Yaw += (xoffset * MouseSensitivity);
        Pitch += (yoffset * MouseSensitivity);

        // Step 2: Constrain Pitch to prevent gimbal lock
        if (constrainPitch) {
            if (Pitch > PITCH_LIMIT) {
                Pitch = PITCH_LIMIT;
            }
            if (Pitch < -PITCH_LIMIT) {
                Pitch = -PITCH_LIMIT;
            }
        }

        // Step 3: Recompute basis vectors
        updateCameraVectors();
    }

    /** * @brief Re-calculates internal Front, Right, and Up vectors from updated Euler angles.
     */
    void updateCameraVectors() {
        glm::vec3 front;

        // Step 1: Calculate direction vector components from Euler angles
        front.x = static_cast<float>(std::cos(static_cast<double>(glm::radians(Yaw))) * std::cos(static_cast<double>(glm::radians(Pitch))));
        front.y = static_cast<float>(std::sin(static_cast<double>(glm::radians(Pitch))));
        front.z = static_cast<float>(std::sin(static_cast<double>(glm::radians(Yaw))) * std::cos(static_cast<double>(glm::radians(Pitch))));

        // Step 2: Rebuild the orthonormal basis
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up = glm::normalize(glm::cross(Right, Front));
    }

    // --- Accessors (Getters & Setters) ---

    const glm::vec3& getPosition() const { return Position; }
    void setPosition(const glm::vec3& pos) { Position = pos; }

    const glm::vec3& getFront() const { return Front; }
    float getZoom() const { return Zoom; }

    float getMovementSpeed() const { return MovementSpeed; }
    void setMovementSpeed(float speed) { MovementSpeed = speed; }

    float getMouseSensitivity() const { return MouseSensitivity; }
    void setMouseSensitivity(float s) { MouseSensitivity = s; }

    void setYaw(float yaw) {
        Yaw = yaw;
        updateCameraVectors();
    }

    void setPitch(float pitch) {
        Pitch = pitch;
        if (Pitch > PITCH_LIMIT) {
            Pitch = PITCH_LIMIT;
        }
        if (Pitch < -PITCH_LIMIT) {
            Pitch = -PITCH_LIMIT;
        }
        updateCameraVectors();
    }

    /** * @brief Generates the Projection matrix based on the current mode.
         * Fulfills Requirement: view a scenario from orthographic positions aligned to an axis.
         */
    glm::mat4 getProjectionMatrix(float aspect, float nearP = 0.1f, float farP = 100.0f) const {
        if (m_mode == ProjectionMode::ORTHOGRAPHIC) {
            // 'Zoom' acts as the vertical half-extent for the ortho box
            // A value of 5.0f is a good default for your scene scale
            float size = 5.0f;
            return glm::ortho(-size * aspect, size * aspect, -size, size, nearP, farP);
        }

        return glm::perspective(glm::radians(Zoom), aspect, nearP, farP);
    }

    // --- Mode Controls ---
    void setProjectionMode(ProjectionMode mode) { m_mode = mode; }

    ProjectionMode getProjectionMode() const { return m_mode; }

    // include/Camera.h - Add to public section
	/** @brief Generates a static View-Projection matrix for specialized quadrants. */
    static glm::mat4 getStaticVP(glm::vec3 pos, float pitch, float yaw, float aspect, float size = 7.5f) {
        // 1. Generate View Matrix
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        glm::mat4 view = glm::lookAt(pos, pos + glm::normalize(front), glm::vec3(0, 1, 0));

        // 2. Generate Orthographic Projection (Fixed size for Lab 2)
        glm::mat4 proj = glm::ortho(-size * aspect, size * aspect, -size, size, 0.1f, 100.0f);
        proj[1][1] *= -1.0f; // Vulkan correction

        return proj * view;
    }

private:
    ProjectionMode m_mode = ProjectionMode::PERSPECTIVE;

    // --- Vector Basis State ---
    glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 Front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 Up{ 0.0f, 1.0f, 0.0f };
    glm::vec3 Right{ 1.0f, 0.0f, 0.0f };
    glm::vec3 WorldUp{ 0.0f, 1.0f, 0.0f };

    // --- Euler Orientation ---
    float Yaw{ DEFAULT_YAW };
    float Pitch{ DEFAULT_PITCH };

    // --- Configuration Parameters ---
    float MovementSpeed{ DEFAULT_SPEED };
    float MouseSensitivity{ DEFAULT_SENSITIVITY };
    float Zoom{ DEFAULT_ZOOM };
};