#include "examples.hpp"
#include "common_obj.hpp"

#include "imgui.h"

using namespace std::literals;
using namespace glwrap;

class bloom final : public example
{
public:
    bloom()
    {
        blur_program_.uniform("image").set_int(0);
        exposure_uniform_.set(exposure_);

        for (auto i = 0u; i < lights_.size(); ++i)
        {
            auto &light = lights_[i];
            wbox_.set_point_light(i, light.position, light.attenuation, light.color);
            boxes_[i].set_render_bright(true);
        }
        wbox_.set_render_bright(true);
        wbox_.set_light_count(lights_.size());
    }

    bool custom_render() override { return true; }

    void reset_frame_buffer(GLsizei width, GLsizei height) override
    {
        fbuffer_.emplace(2, width, height, 0, true);
        blur_buffer_.emplace(2, width, height, 0, true);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(5.1f, 0.1f, 5.4f));
    }

    void draw_gui() override
    {
        if (ImGui::SliderFloat("Exposure", &exposure_, 0.1f, 5.0f))
        {
            exposure_uniform_.set_float(exposure_);
        }
        ImGui::SliderInt("Blur Times", &blur_times_, 0, 20);
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        auto &fb = fbuffer_.value();

        fb.bind();
        fb.clear();
        fb.clear_color(glm::vec4(0), 1);
        fb.draw_buffers({0, 1});
        auto view = cam.view();
        for (auto &box : boxes_)
        {
            box.draw(projection, view);
        }

        wbox_.set_transform(glm::scale(glm::translate(glm::mat4(1), {0.0f, -1.0f, 0.0}), {12.5f, 0.5f, 12.5f}));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::scale(glm::translate(glm::mat4(1), {0.0f, 1.5f, 0.0}), glm::vec3(0.5f)));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::scale(glm::translate(glm::mat4(1), {2.0f, 0.0f, 1.0}), glm::vec3(0.5f)));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::rotate(glm::translate(glm::mat4(1), {-1.0f, -1.0f, 2.0}), glm::radians(60.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::rotate(glm::translate(glm::mat4(1), {0.0f, 2.7f, 4.0}), glm::radians(23.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::rotate(glm::translate(glm::mat4(1), {-2.0f, 1.0f, -3.0}), glm::radians(124.0f), glm::normalize(glm::vec3(1.0, 0.0, 1.0))));
        wbox_.draw(projection, cam);

        wbox_.set_transform(glm::scale(glm::translate(glm::mat4(1), {-3.0f, 0.0f, 0.0}), glm::vec3(0.5f)));
        wbox_.draw(projection, cam);

        // blur bright
        glDisable(GL_DEPTH_TEST);

        auto &quad_varray = utils::get_quad_varray();

        blur_program_.use();
        auto &bb = blur_buffer_.value();
        bb.clear_color();
        bb.bind();

        for (auto i = 0; i < blur_times_; ++i)
        {
            auto &bright = i == 0 ? fb.color_texture_at(1) : bb.color_texture_at(0);
            bright.bind_unit(0);
            blur_horizonal_.set_bool(true);
            bb.draw_buffers({1});
            quad_varray.draw(draw_mode::triangles);

            bb.color_texture_at(1).bind_unit(0);
            blur_horizonal_.set_bool(false);
            bb.draw_buffers({0});
            quad_varray.draw(draw_mode::triangles);
        }

        // final
        frame_buffer::unbind_all();
        fb.color_texture_at(0).bind_unit(0);
        bb.color_texture_at(0).bind_unit(1);
        bloom_final_program_.use();
        quad_varray.draw(draw_mode::triangles);
        glEnable(GL_DEPTH_TEST);
    }

private:
    struct light_t
    {
        glm::vec3 position;
        glm::vec3 attenuation;
        glm::vec3 color;
    };

    std::array<light_t, 4> lights_{
        light_t{{0.0f, 0.5f, 1.5f}, {1.0f, 0.22f, 0.20f}, {5.0f, 5.0f, 5.0f}},
        light_t{{-4.0f, 0.5f, -3.0f}, {1.0f, 0.22f, 0.20f}, {10.0f, 0.0f, 0.0f}},
        light_t{{3.0f, 0.5f, 1.0f}, {1.0f, 0.22f, 0.20f}, {0.0f, 0.0f, 15.0f}},
        light_t{{-.8f, 2.4f, -1.0f}, {1.0f, 0.22f, 0.20f}, {0.0f, 5.0f, 0.0f}},
    };

    std::array<box, 4> boxes_{
        box{lights_[0].position, glm::vec3(0.5f), glm::vec4(lights_[0].color, 1)},
        box{lights_[1].position, glm::vec3(0.5f), glm::vec4(lights_[1].color, 1)},
        box{lights_[2].position, glm::vec3(0.5f), glm::vec4(lights_[2].color, 1)},
        box{lights_[3].position, glm::vec3(0.5f), glm::vec4(lights_[3].color, 1)},
    };

    wooden_box wbox_;

    // -------- frame buffer --------------

    shader_program blur_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/bloom_blur_fs.glsl"_path
    )};
    shader_uniform blur_horizonal_{blur_program_.uniform("horizontal")};

    shader_program bloom_final_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/bloom_final_fs.glsl"_path,
        "screenTexture", 0,
        "blurredBright", 1
    )};
    float exposure_ = 2;
    shader_uniform exposure_uniform_{bloom_final_program_.uniform("exposure")};

    int blur_times_ = 10;

    std::optional<frame_buffer> fbuffer_{};
    std::optional<frame_buffer> blur_buffer_{};
};

std::unique_ptr<example> create_bloom()
{
    return std::make_unique<bloom>();
}