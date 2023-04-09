#include "examples.hpp"
#include "utils.hpp"
#include "common_obj.hpp"

using namespace glwrap;
using namespace std::literals;

class direct_light_pbr final : public example
{
public:
    direct_light_pbr()
    {
        for (auto i = 0u; i < lights_.size(); ++i)
        {
            auto color_light = light_uniform_t{color_program_, i};
            color_light.set(lights_[i].position, lights_[i].flux);

            auto texture_light = light_uniform_t{texture_program_, i};
            texture_light.set(lights_[i].position, lights_[i].flux);
        }
    }

    bool is_hdr() override { return true; }

    void switch_state(int i) override
    {
        draw_type_ = i;
    }

    std::vector<std::string> const& get_states() override
    {
        static std::vector<std::string> states{"color", "texture"};
        return states;
    }

    std::optional<camera> get_camera() override
    {
        return camera{glm::vec3(-19.0f, 2.0f, 25.0f), glm::vec3(0, 1, 0), -57, -6};
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        glClearColor(0.01f, 0.01f, 0.01f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto view = cam.view();
        auto view_pos = cam.position();

        if (draw_type_ == 0)
        {
            color_program_.use();
            color_projection_.set_mat4(projection);
            color_view_uniform_.set_mat4(view);
            color_view_pos_.set_vec3(view_pos);

            sphere_.bind();
            for (int i = 0; i < count; ++i)
            {
                auto x = static_cast<float>(i) / count;
                for (int j = 0; j < count; ++j)
                {
                    auto y = static_cast<float>(j) / count;
                    color_metalness_.set_float(y);
                    color_roughness_.set_float(glm::clamp(x, 0.05f, 1.0f));
                    auto position = glm::vec3(x - 0.5f, y - 0.5f, 0) * (3.0f * count);
                    auto trans = translate(glm::mat4(1), position);
                    color_model_uniform_.set_mat4(trans);
                    color_normal_mat_uniform_.set_mat4(inverse(trans));
                    sphere_.draw(draw_mode::triangles);
                }
            }
        }
        else
        {
            texture_program_.use();
            texture_projection_.set_mat4(projection);
            texture_view_uniform_.set_mat4(view);
            texture_view_pos_.set_vec3(view_pos);
            base_color_tex_.bind_unit(0);
            normal_tex_.bind_unit(1);
            metallic_tex_.bind_unit(2);
            roughness_tex_.bind_unit(3);
            ao_tex_.bind_unit(4);

            sphere_.bind();
            for (int i = 0; i < count; ++i)
            {
                auto x = static_cast<float>(i) / count;
                for (int j = 0; j < count; ++j)
                {
                    auto y = static_cast<float>(j) / count;
                    auto position = glm::vec3(x - 0.5f, y - 0.5f, 0) * (3.0f * count);
                    auto trans = translate(glm::mat4(1), position);
                    texture_model_uniform_.set_mat4(trans);
                    texture_normal_mat_uniform_.set_mat4(inverse(trans));
                    sphere_.draw(draw_mode::triangles);
                }
            }
        }

        for (auto & light: lights_)
        {
            box_.set_position(light.position);
            auto dist = length(view_pos - light.position);
            box_.set_color(glm::vec4(light.flux, 1));
            box_.draw(projection, view);
        }
    }

private:
    int draw_type_{};

    static constexpr int count = 7;

    box box_{glm::vec3(), glm::vec3(0.25f)};

    struct light_t
    {
        glm::vec3 position;
        glm::vec3 flux;
    };

    struct light_uniform_t
    {
        shader_uniform position_;
        shader_uniform flux_;
        light_uniform_t(shader_program &p, size_t index)
            : position_{p.uniform(std::format("lights[{}].position", index))}
            , flux_{p.uniform(std::format("lights[{}].flux", index))}
        { }

        void set(glm::vec3 const& position, glm::vec3 const& flux)
        {
            position_.set_vec3(position);
            flux_.set_vec3(flux);
        }
    };

    std::vector<light_t> lights_{
        {glm::vec3(-10.0f, 10.0f, 10.0f), glm::vec3(300.0f, 300.0f, 300.0f)},
        {glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(300.0f, 300.0f, 300.0f)},
        {glm::vec3(-10.0f, -10.0f, 10.0f), glm::vec3(300.0f, 300.0f, 300.0f)},
        {glm::vec3(10.0f, -10.0f, 10.0f), glm::vec3(300.0f, 300.0f, 300.0f)},
    };

    vertex_array sphere_{utils::create_uv_sphere(30, 30, true)};

    shader_program color_program_{make_vf_program(
        "shaders/pbr/sphere_pbr_color_vs.glsl"_path,
        "shaders/pbr/sphere_pbr_color_fs.glsl"_path,
        "albedo", glm::vec3(1, 0, 0),
        "ao", 1.0f
    )};
    shader_uniform color_projection_{color_program_.uniform("projection")};
    shader_uniform color_view_uniform_{color_program_.uniform("view")};
    shader_uniform color_model_uniform_{color_program_.uniform("model")};
    shader_uniform color_normal_mat_uniform_{color_program_.uniform("normalMat")};
    shader_uniform color_view_pos_{color_program_.uniform("viewPos")};
    shader_uniform color_metalness_{color_program_.uniform("metalness")};
    shader_uniform color_roughness_{color_program_.uniform("roughness")};

    shader_program texture_program_{make_vf_program(
        "shaders/pbr/sphere_pbr_texture_vs.glsl"_path,
        "shaders/pbr/sphere_pbr_texture_fs.glsl"_path,
        "albedoTexture", 0,
        "normalTexture", 1,
        "metallicTexture", 2,
        "roughnessTexture", 3,
        "aoTexture", 4
    )};
    shader_uniform texture_projection_{texture_program_.uniform("projection")};
    shader_uniform texture_view_uniform_{texture_program_.uniform("view")};
    shader_uniform texture_model_uniform_{texture_program_.uniform("model")};
    shader_uniform texture_normal_mat_uniform_{texture_program_.uniform("normalMat")};
    shader_uniform texture_view_pos_{texture_program_.uniform("viewPos")};
    texture2d base_color_tex_{"resources/textures/rustediron/basecolor.png", true};
    texture2d normal_tex_{"resources/textures/rustediron/normal.png"};
    texture2d metallic_tex_{"resources/textures/rustediron/metallic.png"};
    texture2d roughness_tex_{"resources/textures/rustediron/roughness.png"};
    texture2d ao_tex_{"resources/textures/rustediron/ao.png"};
};

std::unique_ptr<example> create_direct_light_pbr()
{
    return std::make_unique<direct_light_pbr>();
}