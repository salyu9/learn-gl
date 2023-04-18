#pragma once

#include <memory>
#include "glwrap.hpp"
#include "model.hpp"
#include "camera.hpp"

class box final
{
public:
    box(glm::vec3 const &position = glm::vec3(0), glm::vec3 const &size = glm::vec3(1, 1, 1), glm::vec4 const &color = glm::vec4(1, 1, 1, 1));
    box(box &&) noexcept;
    ~box();
    box &operator=(box &&other) noexcept;
    void draw(glm::mat4 const &projection, glm::mat4 const &view, glwrap::shader_program *program_override = nullptr);

    glm::vec3 const& get_position() const noexcept;
    void set_position(glm::vec3 const &) noexcept;

    glm::vec3 const& get_size() const noexcept;
    void set_size(glm::vec3 const &) noexcept;

    glm::vec4 get_color() const noexcept;
    void set_color(glm::vec4 const &color) noexcept;

    void set_render_bright(bool value) noexcept;

private:
    class box_impl;
    std::unique_ptr<box_impl> impl_;
};

class wooden_box
{
public:
    wooden_box();
    wooden_box(glwrap::shader_program &&program);
    wooden_box(wooden_box &&) noexcept;
    ~wooden_box();
    wooden_box &operator=(wooden_box &&other) noexcept;
    void draw(glm::mat4 const &projection, view_info &view_info, glwrap::shader_program *program_override = nullptr);

    glm::mat4 const &get_transform() const noexcept;
    void set_transform(glm::mat4 const &transform) noexcept;
    void set_dir_light(glm::vec3 const &dir, glm::vec3 const &color) noexcept;
    void set_point_light(int index, glm::vec3 const &position, glm::vec3 const &attenuation, glm::vec3 const &color) noexcept;
    void set_ambient_light(glm::vec3 const &color) noexcept;
    void set_light_count(int count) noexcept;
    void set_render_bright(bool value) noexcept;

    glwrap::shader_program &program() noexcept;

private:
    class wooden_box_impl;
    std::unique_ptr<wooden_box_impl> impl_;
};