#include <cmath>

#include "examples.hpp"
#include "utils.hpp"

#include "imgui.h"

using namespace std::literals;
using namespace glwrap;

struct vert_t
{
    using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2>;
    glm::vec3 postion;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};
class hdr_scene final : public example
{
public:
    hdr_scene()
    {
        glDisable(GL_CULL_FACE);
        exposure_uniform_.set(exposure_);
    }

    std::optional<camera> get_camera() override
    {
        return camera{glm::vec3(1.12f, 0.50f, -2.52f), glm::vec3(0, 1, 0), 100.0f, -7.7f};
    }

    void switch_state(int i) override
    {
        hdr_ = i == 0;
    }

    std::vector<std::string> const &get_states() override
    {
        static std::vector<std::string> states{"HDR", "LDR"};
        return states;
    }

    void draw_gui() override
    {
        if (hdr_)
        {
            if (ImGui::SliderFloat("Exposure", &exposure_, 0, 5))
            {
                exposure_uniform_.set_float(exposure_);
            }
        }
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hdr_light_program_.use();

        projection_.set_mat4(projection);
        view_.set_mat4(cam.view());

        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
        model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
        model_.set_mat4(model);

        diffuse_.bind_unit(0);

        varray_.bind();
        varray_.draw(draw_mode::triangles);
    }

    shader_program *postprocessor_ptr() override
    {
        return hdr_ ? &postprocess_ : &postprocess_nonhdr_;
    }

    bool is_hdr() override { return true; }

private:
    bool hdr_{true};

    vertex_array varray_{vertex_array::load_simple_json("resources/simple_vertices/hdr_scene.jsonc")};

    texture2d diffuse_{"resources/textures/wood.png"sv, true};

    shader_program hdr_light_program_{make_vf_program(
        "shaders/hdr_light_vs.glsl"_path,
        "shaders/hdr_light_fs.glsl"_path,
        "lights[0].Position", glm::vec3(0.0f, 0.0f, 49.5f),
        "lights[1].Position", glm::vec3(-1.4f, -1.9f, 9.0f),
        "lights[2].Position", glm::vec3(0.0f, -1.8f, 4.0f),
        "lights[3].Position", glm::vec3(0.8f, -1.7f, 6.0f),
        "lights[0].Color", glm::vec3(200.0f, 200.0f, 200.0f),
        "lights[1].Color", glm::vec3(0.1f, 0.0f, 0.0f),
        "lights[2].Color", glm::vec3(0.0f, 0.0f, 0.2f),
        "lights[3].Color", glm::vec3(0.0f, 0.1f, 0.0f),
        "diffuseTexture", 0,
        "inverse_normals", true)};

    shader_uniform projection_{hdr_light_program_.uniform("projection")};
    shader_uniform view_{hdr_light_program_.uniform("view")};
    shader_uniform model_{hdr_light_program_.uniform("model")};

    shader_program postprocess_nonhdr_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/base/fbuffer_fs.glsl"_path)};

    shader_program postprocess_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/hdr_exposure_fs.glsl"_path)};
    float exposure_ = 2;
    shader_uniform exposure_uniform_{postprocess_.uniform("exposure")};
};

std::unique_ptr<example> create_hdr()
{
    return std::make_unique<hdr_scene>();
}
