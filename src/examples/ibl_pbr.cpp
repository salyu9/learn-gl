#include "examples.hpp"
#include "common_obj.hpp"
#include "utils.hpp"
#include "skybox.hpp"

using namespace glwrap;

enum class draw_type
{
    final_color,
    final_texture,
    env,
    env_diffuse,
    env_ss1,
    env_ss2,
    max,
};

class ibl_pbr final : public example
{
public:
    ibl_pbr()
    {
        color_program_.uniform("albedo").set_vec3(glm::vec3(0.8f, 0.8f, 0.8f));
        color_ao_.set_float(1);

        for (auto i = 0u; i < lights_.size(); ++i)
        {
            auto color_light = light_uniform_t{color_program_, i};
            color_light.set(lights_[i].position, lights_[i].flux);

            auto texture_light = light_uniform_t{texture_program_, i};
            texture_light.set(lights_[i].position, lights_[i].flux);
        }

        texture_program_.uniform("albedoTexture").set_int(0);
        texture_program_.uniform("normalTexture").set_int(1);
        texture_program_.uniform("metallicTexture").set_int(2);
        texture_program_.uniform("roughnessTexture").set_int(3);
        texture_program_.uniform("aoTexture").set_int(4);

        prepare_env();
    }

    bool is_hdr() override { return true; }

    void on_switch_state() override
    {
        draw_type_ = static_cast<draw_type>(static_cast<int>(draw_type_) + 1);
        if (draw_type_ == draw_type::max) {
            draw_type_ = {};
        }
    }

    std::optional<std::string_view> get_state() override
    {
        switch (draw_type_)
        {
        case draw_type::final_color:
            return "final - color";
        case draw_type::final_texture:
            return "final - texture";
        case draw_type::env:
            return "environment";
        case draw_type::env_diffuse:
            return "environment - diffuse";
        case draw_type::env_ss1:
            return "environment - ss1";
        case draw_type::env_ss2:
            return "environment - ss2";
        default:
            throw std::runtime_error(std::format("Invalid draw type: {}", static_cast<int>(draw_type_)));
        }
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

        if (draw_type_ == draw_type::final_color)
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
        else if (draw_type_ == draw_type::final_texture)
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
        else if (draw_type_ == draw_type::env)
        {
            glDepthMask(GL_FALSE);
            skybox_program_.use();
            skybox_program_.uniform("skybox").set_int(0);
            skybox_projection_.set_mat4(projection);
            skybox_view_.set_mat4(glm::mat4(glm::mat3(view)));
            env_.bind_unit(0);
            auto &varray = utils::get_skybox();
            varray.bind();
            varray.draw(draw_mode::triangles);
            glDepthMask(GL_TRUE);
        }
        else if (draw_type_ == draw_type::env_diffuse)
        {

        }
        else if (draw_type_ == draw_type::env_ss1)
        {

        }
        else if (draw_type_ == draw_type::env_ss2)
        {

        }
        else 
        {
            std::cout << "Invalid draw type" << std::endl;
        }

        for (auto &light : lights_)
        {
            box_.set_position(light.position);
            auto dist = length(view_pos - light.position);
            box_.set_color(glm::vec4(light.flux, 1));
            box_.draw(projection, view);
        }
    }

private:
    // pre
    void prepare_env()
    {

    }

private :
    // members

    draw_type draw_type_{};

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
            : position_{p.uniform(std::format("lights[{}].position", index))}, flux_{p.uniform(std::format("lights[{}].flux", index))}
        {
        }

        void set(glm::vec3 const &position, glm::vec3 const &flux)
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

    shader_program color_program_{
        shader::compile_file("shaders/pbr/sphere_pbr_color_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/pbr/ibl_color_fs.glsl", shader_type::fragment),
    };
    shader_uniform color_projection_{color_program_.uniform("projection")};
    shader_uniform color_view_uniform_{color_program_.uniform("view")};
    shader_uniform color_model_uniform_{color_program_.uniform("model")};
    shader_uniform color_normal_mat_uniform_{color_program_.uniform("normalMat")};
    shader_uniform color_view_pos_{color_program_.uniform("viewPos")};
    shader_uniform color_metalness_{color_program_.uniform("metalness")};
    shader_uniform color_roughness_{color_program_.uniform("roughness")};
    shader_uniform color_ao_{color_program_.uniform("ao")};

    shader_program texture_program_{
        shader::compile_file("shaders/pbr/sphere_pbr_texture_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/pbr/sphere_pbr_texture_fs.glsl", shader_type::fragment),
    };
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

    cubemap env_{cubemap::from_single_texture(texture2d{"resources/textures/Alexs_Apartment/Alexs_Apt_2k.hdr"}, 1024)};
    shader_program skybox_program_{
        shader::compile_file("shaders/base/skybox_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/base/skybox_fs.glsl", shader_type::fragment)
    };
    shader_uniform skybox_projection_{skybox_program_.uniform("projection")};
    shader_uniform skybox_view_{skybox_program_.uniform("view")};
};

std::unique_ptr<example> create_ibl_pbr()
{
    return std::make_unique<ibl_pbr>();
}