#include <random>

#include "examples.hpp"
#include "glwrap.hpp"
#include "common_obj.hpp"

using namespace glwrap;
using namespace std::literals;

class deferred final : public example
{
public:
    deferred()
    {
        for (int i = 0; i < lights_.size(); ++i)
        {
            auto &light = lights_[i];
            auto uniform = light_uniform_t{g_lighting_program_, i};
            uniform.set(light);
        }
        g_lighting_program_.uniform("lightCount").set_int(light_count);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera({6.45f, 4.30f, 9.34f});
    }

    bool custom_render() override { return true; }

    void reset_frame_buffer(GLsizei width, GLsizei height) override
    {
        std::vector<texture2d> colors;
        colors.emplace_back(width, height, 0, GL_RG16_SNORM); // octahedral normal
        colors.emplace_back(width, height, 0, GL_RGB8);       // albedo
        colors.emplace_back(width, height, 0, GL_RGB8);       // specular

        g_buffer_.emplace(std::move(colors), width, height, 0);
        g_buffer_.value().draw_buffers({0, 1, 2});

        post_buffer_.emplace(width, height, 0, true);
    }

    void switch_state(int i) override
    {
        draw_type_ = static_cast<draw_type>(i);
        if (draw_type_ == draw_type::max_value)
        {
            draw_type_ = draw_type::single_pass;
        }
    }

    std::vector<std::string> const& get_states() override
    {
        static std::vector<std::string> states{
            "single_pass",
            "accumulate",
            "position",
            "normal",
            "albedo",
            "specular",
            "light_range",
        };
        return states;
    }

    void draw(glm::mat4 const &projection, camera &cam)
    {

        auto &gb = g_buffer_.value();

        // geometry pass

        gb.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        g_buffer_program_.use();

        glEnable(GL_DEPTH_TEST);
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
                mesh.get_texture(texture_type::normal).bind_unit(2);
                auto &varray = mesh.get_varray();
                varray.draw(draw_mode::triangles);
            }
        }

        auto &pb = post_buffer_.value();
        pb.bind();
        pb.clear();

        auto &quad_varray = utils::get_quad_varray();

        glDisable(GL_DEPTH_TEST);
        if (draw_type_ == draw_type::single_pass)
        {
            // lighting pass
            gb.depth_texture().bind_unit(0);
            gb.color_texture_at(0).bind_unit(1);
            gb.color_texture_at(1).bind_unit(2);
            gb.color_texture_at(2).bind_unit(3);
            g_lighting_program_.use();
            lighting_inverse_view_projection_.set_mat4(glm::inverse(projection * view));
            lighting_view_pos_.set_vec3(cam.position());
            lighting_frame_size_.set_vec2({gb.width(), gb.height()});
            quad_varray.draw(draw_mode::triangles);

            // draw light box
            gb.blit_to(pb, GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            for (auto &light : lights_)
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
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::accumulate)
        {
            // lighting pass
            gb.depth_texture().bind_unit(0);
            gb.color_texture_at(0).bind_unit(1);
            gb.color_texture_at(1).bind_unit(2);
            gb.color_texture_at(2).bind_unit(3);
            g_lighting_accumulate_program_.use();
            accumulate_projection_.set_mat4(projection);
            accumulate_inverse_view_projection_.set_mat4(glm::inverse(projection * view));
            accumulate_view_pos_.set_vec3(cam.position());
            accumulate_frame_size_.set_vec2({gb.width(), gb.height()});
            glCullFace(GL_FRONT);
            glEnable(GL_BLEND);
            glBlendFunc(GL_ONE, GL_ONE);
            for (auto &light : lights_)
            {
                auto model = glm::scale(glm::translate(glm::mat4(1), light.position), glm::vec3(light.range));
                accumulate_view_model_.set_mat4(view * model);
                accumulate_light_position_.set_vec3(light.position);
                accumulate_light_attenuation_.set_vec3(light.attenuation);
                accumulate_light_color_.set_vec3(light.color);
                accumulate_light_range_.set_float(light.range);
                sphere_.draw(draw_mode::triangles);
            }
            glCullFace(GL_BACK);
            glDisable(GL_BLEND);

            // draw light box
            gb.blit_to(pb, GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            for (auto &light : lights_)
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
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::position)
        {
            frame_buffer::unbind_all();
            g_debug_position_program_.use();
            g_debug_inverse_view_projection_.set_mat4(glm::inverse(projection * view));
            g_debug_frame_size_.set_vec2({gb.width(), gb.height()});
            gb.depth_texture().bind_unit(0);
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::normal)
        {
            frame_buffer::unbind_all();
            g_debug_normal_program_.use();
            gb.color_texture_at(0).bind_unit(0);
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::albedo)
        {
            frame_buffer::unbind_all();
            post_program_.use();
            gb.color_texture_at(1).bind_unit(0);
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::specular)
        {
            frame_buffer::unbind_all();
            post_program_.use();
            gb.color_texture_at(2).bind_unit(0);
            quad_varray.draw(draw_mode::triangles);
        }
        else if (draw_type_ == draw_type::light_range)
        {
            // lighting pass
            pb.bind();
            post_program_.use();
            gb.color_texture_at(2).bind_unit(0);
            quad_varray.draw(draw_mode::triangles);

            // draw light box
            gb.blit_to(pb, GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glCullFace(GL_FRONT);
            glDepthMask(GL_FALSE);
            g_light_range_program_.uniform("projection").set_mat4(projection);
            for (auto &light : lights_)
            {
                box_.set_position(light.position);
                box_.set_color(glm::vec4(light.color, 1));
                box_.draw(projection, view);
                g_light_range_program_.use();

                auto trans = glm::scale(glm::translate(glm::mat4(1), light.position), glm::vec3(light.range));
                g_light_range_program_.uniform("viewModel").set_mat4(view * trans);
                g_light_range_program_.uniform("color").set_vec4({light.color, 0.5f});
                sphere_.draw(draw_mode::triangles);
            }
            glDisable(GL_BLEND);
            glCullFace(GL_BACK);
            glDepthMask(GL_TRUE);

            // post
            glDisable(GL_DEPTH_TEST);
            frame_buffer::unbind_all();
            post_program_.use();
            pb.color_texture().bind_unit(0);
            quad_varray.draw(draw_mode::triangles);
        }
    }

private:
    box box_{glm::vec3(0), glm::vec3(0.125f)};
    vertex_array sphere_{utils::create_uv_sphere(10, 10)};

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

    shader_program g_buffer_program_{make_vf_program(
        "shaders/g_buffer_vs.glsl"_path,
        "shaders/g_buffer_fs.glsl"_path,
        "diffuseTexture", 0,
        "specularTexture", 1,
        "normalTexture", 2)};
    shader_uniform g_projection_{g_buffer_program_.uniform("projection")};
    shader_uniform g_model_{g_buffer_program_.uniform("model")};
    shader_uniform g_view_{g_buffer_program_.uniform("view")};
    shader_uniform g_normal_mat_{g_buffer_program_.uniform("normalMat")};

    shader_program g_lighting_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/g_lighting_fs.glsl"_path,
        "depthTexture", 0,
        "inputNormal", 1,
        "input1", 2,
        "input2", 3)};
    shader_uniform lighting_view_pos_{g_lighting_program_.uniform("viewPos")};
    shader_uniform lighting_frame_size_{g_lighting_program_.uniform("frameSize")};
    shader_uniform lighting_inverse_view_projection_{g_lighting_program_.uniform("inverseViewProjection")};

    shader_program g_lighting_accumulate_program_{make_vf_program(
        "shaders/common/sphere_vs.glsl"_path,
        "shaders/g_lighting_accumulate_fs.glsl"_path,
        "depthTexture", 0,
        "inputNormal", 1,
        "input1", 2,
        "input2", 3)};
    shader_uniform accumulate_projection_{g_lighting_accumulate_program_.uniform("projection")};
    shader_uniform accumulate_view_model_{g_lighting_accumulate_program_.uniform("viewModel")};
    shader_uniform accumulate_view_pos_{g_lighting_accumulate_program_.uniform("viewPos")};
    shader_uniform accumulate_frame_size_{g_lighting_accumulate_program_.uniform("frameSize")};
    shader_uniform accumulate_inverse_view_projection_{g_lighting_accumulate_program_.uniform("inverseViewProjection")};
    shader_uniform accumulate_light_position_{g_lighting_accumulate_program_.uniform("light.position")};
    shader_uniform accumulate_light_attenuation_{g_lighting_accumulate_program_.uniform("light.attenuation")};
    shader_uniform accumulate_light_color_{g_lighting_accumulate_program_.uniform("light.color")};
    shader_uniform accumulate_light_range_{g_lighting_accumulate_program_.uniform("light.range")};

    shader_program g_debug_position_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/g_debug_position_fs.glsl"_path,
        "depthTexture", 0)};
    shader_uniform g_debug_frame_size_{g_debug_position_program_.uniform("frameSize")};
    shader_uniform g_debug_inverse_view_projection_{g_debug_position_program_.uniform("inverseViewProjection")};

    shader_program g_debug_normal_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/g_debug_normal_fs.glsl"_path,
        "normalTexture", 0)};

    shader_program g_light_range_program_{make_vf_program(
        "shaders/common/sphere_vs.glsl"_path,
        "shaders/common/pure_color_fs.glsl"_path)};

    model backpack_{model::load_file("resources/models/backpack_modified/backpack.obj",
                                     texture_type::diffuse | texture_type::normal | texture_type::specular)};

    shader_program post_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/hdr_exposure_fs.glsl"_path,
        "screenTexture", 0,
        "exposure", 1.0f)};

    struct light_t
    {
        glm::vec3 position;
        glm::vec3 attenuation;
        glm::vec3 color;
        float range;
    };

    static constexpr uint32_t light_count = 32u;

    std::vector<light_t> lights_ = []()
    {
        auto rng = [mt = std::mt19937{std::random_device()()},
                    dist = std::uniform_real_distribution<float>(0.0f, 1.0f)]() mutable
        { return dist(mt); };

        auto constant = 1.0f;
        auto linear = 0.7f;
        auto quadratic = 1.8f;

        std::vector<light_t> lights;
        for (auto i = 0u; i < light_count; ++i)
        {
            auto color = utils::hsv(static_cast<int>(rng() * 360.0f), rng() * 0.5f + 0.5f, rng() * 0.5f + 1.5f);
            auto color_max = std::max({color.r, color.g, color.b});
            auto range = (-linear + std::sqrtf(linear * linear - 4 * quadratic * (constant - (256.0 / 5.0) * color_max))) / (2 * quadratic);

            lights.push_back(light_t{
                .position = glm::vec3(rng() * 8.0 - 4.0, rng() * 7.0 - 4.0, rng() * 8.0 - 4.0),
                .attenuation = glm::vec3(constant, linear, quadratic),
                .color = color,
                .range = range,
            });
        }

        return lights;
    }();

    struct light_uniform_t
    {
        shader_uniform position;
        shader_uniform attenuation;
        shader_uniform color;
        shader_uniform range;
        light_uniform_t(shader_program &p, int index)
            : position{p.uniform(std::format("lights[{}].position", index))},
              attenuation{p.uniform(std::format("lights[{}].attenuation", index))},
              color{p.uniform(std::format("lights[{}].color", index))},
              range{p.uniform(std::format("lights[{}].range", index))}
        {
        }

        void set(light_t const &light)
        {
            position.set_vec3(light.position);
            attenuation.set_vec3(light.attenuation);
            color.set_vec3(light.color);
            range.set_float(light.range);
        }
    };

    // -------- frame buffer --------------

    std::optional<frame_buffer> g_buffer_{};

    std::optional<frame_buffer> post_buffer_{};

    // ------- debug draw type --------

    enum class draw_type : int
    {
        single_pass,
        accumulate,
        position,
        normal,
        albedo,
        specular,
        light_range,
        max_value
    };
    draw_type draw_type_{};
};

std::unique_ptr<example> create_deferred()
{
    return std::make_unique<deferred>();
}