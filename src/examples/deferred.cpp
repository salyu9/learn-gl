#include <random>

#include "examples.hpp"
#include "glwrap.hpp"
#include "common_obj.hpp"

using namespace glwrap;
using namespace std::literals;

class deferred : public example
{
public:

    deferred()
    {
        g_buffer_program_.uniform("diffuseTexture").set_int(0);
        g_buffer_program_.uniform("specularTexture").set_int(1);
        g_lighting_program_.uniform("input0").set_int(0);
        g_lighting_program_.uniform("input1").set_int(1);
        g_lighting_program_.uniform("input2").set_int(2);

        for (int i = 0; i < lights_.size(); ++i)
        {
            auto& light = lights_[i];
            auto uniform = light_uniform_t{g_lighting_program_, i};
            uniform.position.set_vec3(light.position);
            uniform.attenuation.set_vec3(light.attenuation);
            uniform.color.set_vec3(light.color);
        }
        g_lighting_program_.uniform("lightCount").set_int(light_count);
        post_program_.uniform("screenTexture").set_int(0);
        post_program_.uniform("exposure").set_float(1);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera({6.45f, 4.30f, 9.34f});
    }

    bool custom_render() override { return true; }

    void reset_frame_buffer(GLsizei width, GLsizei height) override
    {
        g_buffer_.emplace(3, width, height, 0, true);
        g_buffer_.value().draw_buffers({0, 1, 2});

        post_buffer_.emplace(width, height, 0, true);
    }

    enum class draw_type
    {
        full,
        base_color,
        normal,

    };

    void on_switch() override
    {

    }

    void draw(glm::mat4 const& projection, camera & cam)
    {
        auto &gb = g_buffer_.value();

        // geometry pass

        gb.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gb.draw_buffers({0, 1, 2});
        g_buffer_program_.use();

        auto view = cam.view();
        g_projection_.set_mat4(projection);
        g_view_.set_mat4(view);

        for (auto &pos : backpack_positions_)
        {
            auto model = glm::translate(glm::mat4(1), pos);

            g_model_.set_mat4(model);
            g_normal_mat_.set_mat4(glm::transpose(glm::inverse(model)));
            for (auto &mesh : backpack_.meshes())
            {
                mesh.get_texture(texture_type::diffuse).bind_unit(0);
                mesh.get_texture(texture_type::specular).bind_unit(1);
                auto & varray = mesh.get_varray();
                varray.bind();
                varray.draw(draw_mode::triangles);
            }
        }

        auto &pb = post_buffer_.value();
        pb.bind();
        pb.clear();

        // lighting pass

        glDisable(GL_DEPTH_TEST);
        gb.color_texture_at(0).bind_unit(0);
        gb.color_texture_at(1).bind_unit(1);
        gb.color_texture_at(2).bind_unit(2);
        g_lighting_program_.use();
        lighting_view_pos_.set_vec3(cam.position());
        quad_varray_.bind();
        quad_varray_.draw(draw_mode::triangles);

        // draw light box

        gb.blit_to(pb, GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        for (auto & light : lights_)
        {
            box_.set_position(light.position);
            box_.set_color(glm::vec4(light.color, 1));
            box_.draw(projection, view);
        }

        // post

        glDisable(GL_DEPTH_TEST);
        frame_buffer::unbind_all();
        post_program_.use();
        pb.color_texture().bind_unit(0);
        quad_varray_.bind();
        quad_varray_.draw(draw_mode::triangles);

        glEnable(GL_DEPTH_TEST);
    }

private:
    box box_{glm::vec3(0), glm::vec3(0.125f)};

    std::vector<glm::vec3> backpack_positions_{
        {-3.0f, -0.5f, -3.0f},
        {0.0f, -0.5f, -3.0f},
        {3.0f, -0.5f, -3.0f},
        {-3.0f, -0.5f, 0.0f},
        {0.0f, -0.5f, 0.0f},
        {3.0f, -0.5f, 0.0f},
        {-3.0f, -0.5f, 3.0f},
        {0.0f, -0.5f, 3.0f},
        {3.0f, -0.5f, 3.0f},
    };

    shader_program g_buffer_program_{
        shader::compile_file("shaders/g_buffer_vs.glsl"sv, shader_type::vertex),
        shader::compile_file("shaders/g_buffer_fs.glsl"sv, shader_type::fragment)
    };
    shader_uniform g_projection_{g_buffer_program_.uniform("projection")};
    shader_uniform g_model_{g_buffer_program_.uniform("model")};
    shader_uniform g_view_{g_buffer_program_.uniform("view")};
    shader_uniform g_normal_mat_{g_buffer_program_.uniform("normalMat")};

    shader_program g_lighting_program_{
        shader::compile_file("shaders/base/fbuffer_vs.glsl"sv, shader_type::vertex),
        shader::compile_file("shaders/g_lighting_fs.glsl"sv, shader_type::fragment)
    };
    shader_uniform lighting_view_pos_{g_lighting_program_.uniform("viewPos")};

    model backpack_{model::load_file("resources/models/backpack_modified/backpack.obj",
                                     texture_type::diffuse | texture_type::normal | texture_type::specular)};

    shader_program post_program_{
        shader::compile_file("shaders/base/fbuffer_vs.glsl"sv, shader_type::vertex),
        shader::compile_file("shaders/hdr_exposure_fs.glsl"sv, shader_type::fragment)
    };

    struct light_t
    {
        glm::vec3 position;
        glm::vec3 attenuation;
        glm::vec3 color;
    };

    static constexpr uint32_t light_count = 32u;

    std::vector<light_t> lights_ = []()
    {
        auto rng = [
            mt = std::mt19937{std::random_device()()},
            dist = std::uniform_real_distribution<float>(0.0f, 1.0f)
        ]() mutable { return dist(mt); };

        std::vector<light_t> lights;

        for (auto i = 0u; i < light_count; ++i)
        {
            lights.push_back(light_t{
                .position = glm::vec3(rng() * 8.0 - 4.0, rng() * 7.0 - 4.0, rng() * 8.0 - 4.0),
                .attenuation = {1.0f, 0.22f, 0.20f},
                .color = glm::vec3(rng() * 0.75f + 0.25f, rng() * 0.75f + 0.25f, rng() * 0.75f + 0.25f)
            });
        }

        return lights;
    }();

    struct light_uniform_t
    {
        shader_uniform position;
        shader_uniform attenuation;
        shader_uniform color;
        light_uniform_t(shader_program &p, int index)
            : position{p.uniform(std::format("lights[{}].position", index))},
              attenuation{p.uniform(std::format("lights[{}].attenuation", index))},
              color{p.uniform(std::format("lights[{}].color", index))}
        { }
    };

    // -------- frame buffer --------------

    std::optional<frame_buffer> g_buffer_{};

    std::optional<frame_buffer> post_buffer_{};

    vertex_array quad_varray_{utils::get_quad_varray()};
};

std::unique_ptr<example> create_deferred()
{
    return std::make_unique<deferred>();
}