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
    virtual void init() { }
    virtual std::optional<camera> get_camera() { return std::nullopt; }
    virtual void update() { }
    virtual void draw(glm::mat4 const &projection, glm::mat4 const &view) = 0;
    virtual glwrap::shader_program *postprocessor_ptr() { return nullptr; }
};

using example_creator = std::function<std::unique_ptr<example>()>;
std::unique_ptr<example> create_nanosuit();
std::unique_ptr<example> create_nanosuit_explode();
std::unique_ptr<example> create_asteroids();
std::unique_ptr<example> create_asteroids_instanced();

inline std::vector<std::tuple<std::string_view, example_creator>> get_examples()
{
    return {
        {"model load", create_nanosuit},
        {"explode (geometry shader)", create_nanosuit_explode},
        {"asteroid field", create_asteroids},
        {"asteroid field (instancing)", create_asteroids_instanced}
    };
}