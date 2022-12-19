#pragma once

#include <string_view>
#include <optional>
#include <vector>
#include <functional>

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
    virtual void update() { }
    virtual void draw(glm::mat4 const &projection, camera & cam) = 0;
    virtual glwrap::shader_program *postprocessor_ptr() { return nullptr; }
    virtual bool is_hdr() { return false; }
};

using example_creator = std::function<std::unique_ptr<example>()>;
std::unique_ptr<example> create_nanosuit();
std::unique_ptr<example> create_nanosuit_explode();
std::unique_ptr<example> create_asteroids();
std::unique_ptr<example> create_asteroids_instanced();
std::unique_ptr<example> create_normal_map();
std::unique_ptr<example> create_parallax_map();
std::unique_ptr<example> create_ldr();
std::unique_ptr<example> create_hdr();
std::unique_ptr<example> create_bloom();

inline std::vector<std::tuple<std::string_view, example_creator>>
    get_examples()
{
    return {
        {"model load", create_nanosuit},
        {"explode (geometry shader)", create_nanosuit_explode},
        {"asteroid field", create_asteroids},
        {"asteroid field (instancing)", create_asteroids_instanced},
        {"normal mapping", create_normal_map},
        {"parallax mapping", create_parallax_map},
        // {"ldr", create_ldr},
        // {"hdr / tone mapping", create_hdr},
        // {"bloom", create_bloom},
    };
}