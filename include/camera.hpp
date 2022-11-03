﻿#pragma once

#include "utils.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glad/gl.h>

namespace camera_movement
{
    enum camera_movement : int
    {
        none = 0,
        forward = 1,
        backward = 2,
        left = 4,
        right = 8,
        up = 16,
        down = 32,
    };
}
// Default camera values
const constexpr float default_yaw = -90.0f;

const constexpr float default_pitch = 0.0f;
const constexpr float pitch_limit = 89.0f;

const constexpr float default_speed = 2.5f;
const constexpr float default_sensitivity = 0.05f;
const constexpr float default_zoom = 45.0f;
const constexpr float min_zoom = 1.0f;
const constexpr float max_zoom = 45.0f;
const constexpr float default_near_z = 0.1f;
const constexpr float default_far_z = 100.0f;

class camera
{
public:
    explicit camera(
        glm::vec3 const &position = glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3 const &up = glm::vec3(0.0f, 1.0f, 0.0f),
        float yaw = default_yaw,
        float pitch = default_pitch,
        float near_z = default_near_z,
        float far_z = default_far_z) noexcept
        : position_(position), front_(glm::vec3(0.0f, 0.0f, -1.0f)),
          world_up_(up),
          yaw_(yaw),
          pitch_(pitch),
          near_z_(near_z),
          far_z_(far_z)
    {
        update_camera_vectors();
    }

    glm::mat4 projection(float aspect) const noexcept
    {
        return glm::perspective(glm::radians(zoom_), aspect, near_z_, far_z_);
    }

    // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 view() const noexcept
    {
        return glm::lookAt(position_, position_ + front_, up_);
    }

    glm::vec3 position() const noexcept
    {
        return position_;
    }

    // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void process_keyboard(camera_movement::camera_movement direction, float delta_time) noexcept
    {
        float velocity = movement_speed_ * delta_time;
        if (direction & camera_movement::forward)
            position_ += front_ * velocity;
        if (direction & camera_movement::backward)
            position_ -= front_ * velocity;
        if (direction & camera_movement::left)
            position_ -= right_ * velocity;
        if (direction & camera_movement::right)
            position_ += right_ * velocity;
        if (direction & camera_movement::up)
            position_ += up_ * velocity;
        if (direction & camera_movement::down)
            position_ -= up_ * velocity;
    }

    // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void process_mouse_movement(float xoffset, float yoffset, GLboolean constrain_pitch = true) noexcept
    {
        xoffset *= mouse_sensitivity_;
        yoffset *= mouse_sensitivity_;

        yaw_ += xoffset;

        // Make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrain_pitch)
        {
            pitch_ = std::clamp(pitch_ + yoffset, -pitch_limit, pitch_limit);
        }
        else
        {
            pitch_ += yoffset;
        }

        // Update front_, right_ and up_ Vectors using the updated Euler angles
        update_camera_vectors();
    }

    // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void process_mouse_scroll(float delta) noexcept
    {
        zoom_ = std::clamp(zoom_ -= delta, min_zoom, max_zoom);
        update_camera_vectors();
    }

private:
    // Camera Attributes
    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;
    // Euler Angles
    float yaw_;
    float pitch_;
    // Camera options
    float near_z_;
    float far_z_;
    float zoom_{default_zoom};
    float movement_speed_{default_speed};
    float mouse_sensitivity_{default_sensitivity};

    // Calculates the front vector from the Camera's (updated) Euler Angles
    void update_camera_vectors() noexcept
    {
        // Calculate the new front_ vector
        glm::vec3 front(
            cos(glm::radians(yaw_)) * cos(glm::radians(pitch_)),
            sin(glm::radians(pitch_)),
            sin(glm::radians(yaw_)) * cos(glm::radians(pitch_)));
        front_ = glm::normalize(front);
        // Also re-calculate the right_ and up_ vector
        right_ = glm::normalize(glm::cross(front_, world_up_)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        up_ = glm::normalize(glm::cross(right_, front_));
    }
};
