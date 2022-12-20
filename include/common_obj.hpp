#pragma once

#include <memory>
#include "glwrap.hpp"
#include "model.hpp"
#include "camera.hpp"

class box final
{
public:
    box(glm::vec3 const &position, glm::vec3 const &size = glm::vec3(1, 1, 1), glm::vec4 const &color = glm::vec4(1, 1, 1, 1));
    box(box &&) noexcept;
    ~box();
    box &operator=(box &&other) noexcept;
    void draw(glm::mat4 const &projection, glm::mat4 const &view);

    glm::vec3 const& get_position() const noexcept;
    void set_position(glm::vec3 const &) noexcept;

    glm::vec3 const& get_size() const noexcept;
    void set_size(glm::vec3 const &) noexcept;

    void set_render_bright(bool value) noexcept;

private:
    class box_impl;
    std::unique_ptr<box_impl> impl_;
};

class wooden_box
{
public:
    wooden_box();
    wooden_box(wooden_box &&) noexcept;
    ~wooden_box();
    wooden_box &operator=(wooden_box &&other) noexcept;
    void draw(glm::mat4 const &projection, camera & cam);

    glm::mat4 const &get_transform() const noexcept;
    void set_transform(glm::mat4 const &transform) noexcept;
    void set_point_light(int index, glm::vec3 const &position, glm::vec3 const &attenuation, glm::vec3 const &color);
    void set_render_bright(bool value) noexcept;

private:
    class wooden_box_impl;
    std::unique_ptr<wooden_box_impl> impl_;
};