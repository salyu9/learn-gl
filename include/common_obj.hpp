#pragma once

#include <memory>
#include "glwrap.hpp"
#include "model.hpp"

class box
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

private:
    class box_impl;
    std::unique_ptr<box_impl> impl_;
};