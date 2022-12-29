#include "examples.hpp"
#include "common_obj.hpp"
#include "utils.hpp"
#include "skybox.hpp"

using namespace glwrap;
using namespace std::literals;

inline constexpr GLsizei cubemap_size = 1024;
inline constexpr GLsizei diffuse_size = 32;
inline constexpr GLsizei prefilter_size = 128;
inline constexpr GLsizei prefilter_level = 5;
inline constexpr GLsizei split_sum_size = 512;

enum class draw_type
{
    final_color,
    final_texture,
    env,
    env_diffuse,
    env_prefiltered,
    split_sum,
    max,
};

cubemap make_diffuse(cubemap & input, GLsizei size)
{
    static auto prog = make_compute_program("shaders/compute/env_diffuse.glsl");
    input.bind_unit(0);
    cubemap c{size, GL_RGBA16F};
    c.bind_image_unit(1, image_bind_access::write);

    prog.use();

    glDispatchCompute(size, size, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return c;
}

cubemap make_prefilter(cubemap & input, GLsizei size, GLsizei levels)
{
    static auto prog = make_compute_program("shaders/compute/env_prefilter.glsl");
    static auto roughness_uniform = prog.uniform("roughness");
    input.bind_unit(0);
    cubemap c{size, GL_RGBA16F, levels};

    prog.use();

    for (int i = 0; i < levels; ++i)
    {
        c.bind_image_unit(1, image_bind_access::write, i);
        roughness_uniform.set(static_cast<float>(i) / (levels - 1));
        glDispatchCompute(size, size, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        size /= 2;
    }

    return c;
}

texture2d make_split_sum(cubemap & input, GLsizei size)
{
    static auto prog = make_compute_program("shaders/compute/env_split_sum.glsl");
    texture2d tex{size, size, 0, GL_RG16F};

    prog.use();
    input.bind_image_unit(0, image_bind_access::read);
    tex.bind_image_unit(1, image_bind_access::write);

    glDispatchCompute(size, size, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return tex;
}

class ibl_pbr final : public example
{
public:
    ibl_pbr()
    {
        for (auto i = 0u; i < lights_.size(); ++i)
        {
            auto color_light = light_uniform_t{color_program_, i};
            color_light.set(lights_[i].position, lights_[i].flux);

            auto texture_light = light_uniform_t{texture_program_, i};
            texture_light.set(lights_[i].position, lights_[i].flux);
        }

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
        case draw_type::env_prefiltered:
            return "environment - prefiltered";
        case draw_type::split_sum:
            return "split sum";
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
            env_skybox_.draw(projection, view);

            color_program_.use();
            color_projection_.set_mat4(projection);
            color_view_uniform_.set_mat4(view);
            color_view_pos_.set_vec3(view_pos);
            env_diffuse_.bind_unit(0);
            env_prefiltered_.bind_unit(1);
            split_sum_.bind_unit(2);

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
            env_skybox_.draw(projection, view);

            texture_program_.use();
            texture_projection_.set_mat4(projection);
            texture_view_uniform_.set_mat4(view);
            texture_view_pos_.set_vec3(view_pos);
            base_color_tex_.bind_unit(0);
            normal_tex_.bind_unit(1);
            metallic_tex_.bind_unit(2);
            roughness_tex_.bind_unit(3);
            ao_tex_.bind_unit(4);
            env_diffuse_.bind_unit(5);
            env_prefiltered_.bind_unit(6);
            split_sum_.bind_unit(7);

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
            env_skybox_.draw(projection, view);
        }
        else if (draw_type_ == draw_type::env_diffuse)
        {
            env_diffuse_skybox_.draw(projection, view);
        }
        else if (draw_type_ == draw_type::env_prefiltered)
        {
            glDepthMask(GL_FALSE);
            env_prefiltered_program_.use();
            env_prefiltered_projection_.set(projection);
            env_prefiltered_view_.set(glm::mat4(glm::mat3(view)));
            env_prefiltered_.bind_unit(0);
            auto &varray = utils::get_skybox();
            varray.bind();
            varray.draw(draw_mode::triangles);
            glDepthMask(GL_TRUE);
        }
        else if (draw_type_ == draw_type::split_sum)
        {
            split_sum_.bind_unit(0);
            quad_program_.use();
            auto &varray = utils::get_quad_varray();
            varray.bind();
            varray.draw(draw_mode::triangles);
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

    vertex_array sphere_{utils::create_uv_sphere(50, 50, true)};

    shader_program color_program_{make_vf_program(
        "shaders/pbr/sphere_pbr_color_vs.glsl"_path,
        "shaders/pbr/ibl_color_fs.glsl"_path,
        "albedo", glm::vec3(0.8f, 0.8f, 0.8f),
        "ao", 1.0f,
        "maxPrefilteredLevel", prefilter_level - 1.0f,
        "diffuseEnvCube", 0,
        "prefilteredCube", 1,
        "splitSumTex", 2
    )};
    shader_uniform color_projection_{color_program_.uniform("projection")};
    shader_uniform color_view_uniform_{color_program_.uniform("view")};
    shader_uniform color_model_uniform_{color_program_.uniform("model")};
    shader_uniform color_normal_mat_uniform_{color_program_.uniform("normalMat")};
    shader_uniform color_view_pos_{color_program_.uniform("viewPos")};
    shader_uniform color_metalness_{color_program_.uniform("metalness")};
    shader_uniform color_roughness_{color_program_.uniform("roughness")};

    shader_program texture_program_{make_vf_program(
        "shaders/pbr/sphere_pbr_texture_vs.glsl",
        "shaders/pbr/ibl_texture_fs.glsl",
        "albedoTexture", 0,
        "normalTexture", 1,
        "metallicTexture", 2,
        "roughnessTexture", 3,
        "aoTexture", 4,
        "maxPrefilteredLevel", prefilter_level - 1.0f,
        "diffuseEnvCube", 5,
        "prefilteredCube", 6,
        "splitSumTex", 7
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

    shader_program quad_program_{make_vf_program(
        "shaders/base/quad_vs.glsl",
        "shaders/base/quad_fs.glsl"
    )};

    shader_program env_prefiltered_program_{make_vf_program(
        "shaders/base/skybox_vs.glsl",
        "shaders/base/skybox_fs_leveled.glsl",
        "skybox", 0,
        "level", 4.f
    )};
    shader_uniform env_prefiltered_projection_{env_prefiltered_program_.uniform("projection")};
    shader_uniform env_prefiltered_view_{env_prefiltered_program_.uniform("view")};

    inline static std::string env_entries[] = {
        "resources/textures/Factory_Catwalk/Factory_Catwalk_2k.hdr",
        "resources/textures/Alexs_Apartment/Alexs_Apt_2k.hdr",
    };

    texture2d env_tex_{env_entries[0]};
    cubemap env_{cubemap::from_single_texture(env_tex_, cubemap_size)};
    skybox env_skybox_{env_};

    cubemap env_diffuse_{make_diffuse(env_, diffuse_size)};
    skybox env_diffuse_skybox_{env_diffuse_};

    cubemap env_prefiltered_{make_prefilter(env_, prefilter_size, prefilter_level)};
    skybox env_prefiltered_skybox_{env_prefiltered_};

    texture2d split_sum_{make_split_sum(env_, split_sum_size)};
};

std::unique_ptr<example> create_ibl_pbr()
{
    return std::make_unique<ibl_pbr>();
}