#pragma once

#include <optional>

#include "glm/glm.hpp"
#include "glwrap.hpp"
#include "model.hpp"
#include "camera.hpp"

class example
{
public:
    virtual bool custom_render() { return false; }
    virtual void reset_frame_buffer(GLsizei screen_width, GLsizei screen_height) { }
    virtual std::optional<camera> get_camera() { return std::nullopt; }
    virtual void on_switch_state() { }
    virtual std::optional<std::string_view> get_state() { return std::nullopt; }
    virtual void draw(glm::mat4 const &projection, camera & cam) = 0;
    virtual glwrap::shader_program *postprocessor_ptr() { return nullptr; }
    virtual bool is_hdr() { return false; }
};
