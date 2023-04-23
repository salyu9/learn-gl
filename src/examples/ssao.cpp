#include <random>

#include "glwrap.hpp"
#include "common_obj.hpp"
#include "examples.hpp"
#include "skybox.hpp"
#include "utils.hpp"
#include "imgui.h"

using namespace glwrap;

class ssao final : public example
{
public:
    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera({6.45f, 4.30f, 9.34f});
    }

    bool custom_render() override { return true; }

    void reset_frame_buffer(GLsizei width, GLsizei height) override
    {
        screen_width_ = width;
        screen_height_ = height;
        reset_frame_buffer();
    }

    void draw_gui() override
    {
        float f = fbuffer_exposure_.get_float();
        if (ImGui::SliderFloat("Exposure", &f, 0.1f, 5.0f))
        {
            fbuffer_exposure_.set(f);
        }
        if (ImGui::Checkbox("White Shading", &white_shading_))
        {
            lighting_white_shading_.set(white_shading_);
        }
        if (ImGui::Checkbox("Use Normal", &use_normal_))
        {
            ssao_use_normal_.set(use_normal_);
        }
        if (ImGui::SliderInt("Kernel Size", &kernel_size_, 1, 64))
        {
            ssao_kernel_size_.set(kernel_size_);
        }
        if (ImGui::SliderFloat("Kernel Radius", &kernel_radius_, 0.1f, 2.0f))
        {
            ssao_kernel_radius_.set(kernel_radius_);
        }
        if (ImGui::SliderFloat("Bias", &bias_, 0, 0.1f))
        {
            ssao_bias_.set(bias_);
        }
        ImGui::Checkbox("Blur", &blur_);
        if (ImGui::Checkbox("Range Check", &has_range_check_))
        {
            ssao_has_range_check_.set(has_range_check_);
        }
        if (ImGui::SliderFloat("Weight", &ssao_weight_, 0, 1.0f))
        {
            ssao_final_weight_.set(ssao_weight_);
        }

        ImGui::Image((void *)(intptr_t)ssao_buffer_.value().color_texture().handle(), {256, 256}, {0, 1}, {1, 0});
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        // g-buffer pass
        auto &gb = g_buffer_.value();
        gb.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        g_buffer_program_.use();

        g_projection_.set(projection);
        g_view_.set(cam.view());

        auto model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), {0.0f, 0.8f, 0.0}), glm::radians(-90.0f), {1.0f, 0, 0}), glm::vec3(1.0f));

        g_model_.set(model);
        g_normal_mat_.set(glm::transpose(glm::inverse(model)));
        for (auto &mesh : backpack_.meshes())
        {
            mesh.get_texture(texture_type::diffuse).bind_unit(0);
            mesh.get_texture(texture_type::specular).bind_unit(1);
            mesh.get_texture(texture_type::normal).bind_unit(2);
            auto &varray = mesh.get_varray();
            varray.draw();
        }

        g_plane_program_.use();
        plane_tex_.bind_unit(0);
        plane_projection_.set(projection);
        plane_view_.set(cam.view());

        auto &plane_varray = utils::get_plane_varray();
        auto plane_model = glm::scale(glm::translate(glm::mat4(1.0f), {0, 10, -10}), glm::vec3(20));
        plane_model_.set(plane_model);
        plane_normal_mat_.set(transpose(inverse(plane_model)));
        plane_varray.draw();

        plane_model = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), {-10, 10, 0}), glm::radians(90.0f), {0, 1, 0}), glm::vec3(20));
        plane_model_.set(plane_model);
        plane_normal_mat_.set(transpose(inverse(plane_model)));
        plane_varray.draw();

        plane_model = glm::scale(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), {1, 0, 0}), glm::vec3(20));
        plane_model_.set(plane_model);
        plane_normal_mat_.set(transpose(inverse(plane_model)));
        plane_varray.draw();

        // lighting pass
        glDisable(GL_DEPTH_TEST);
        auto &quad = utils::get_quad_varray();

        f_buffer_.value().bind();
        gb.depth_texture().bind_unit(0);
        gb.color_texture_at(0).bind_unit(1);
        gb.color_texture_at(1).bind_unit(2);
        gb.color_texture_at(2).bind_unit(3);

        lighting_inverse_view_projection_.set(glm::inverse(projection * cam.view()));
        lighting_view_pos_.set(cam.position());

        lighting_program_.use();

        quad.draw();

        // ssao pass

        ssao_buffer_.value().bind();
        std::array<glm::vec3, 64> kernel;
        for (int i = 0; i < kernel_size_; ++i)
        {
            auto dir = normalize(glm::vec3(rand_float() * 2 - 1, rand_float() * 2 - 1, use_normal_ ? rand_float() : rand_float() * 2 - 1));
            kernel[i] = dir * rand_float() * utils::lerp(0.1f, 1.0f, (i / kernel_size_) * (i / kernel_size_));
        }
        ssao_normal_view_mat_.set(transpose(inverse(cam.view())));
        ssao_kernel_.set_vec3s(std::span<glm::vec3>(kernel.begin(), kernel.begin() + kernel_size_));

        std::array<glm::vec3, 16> noise{};
        for (auto &n : noise)
        {
            n = {rand_float() * 2 - 1, rand_float() * 2 - 1, 0.0f};
        }
        texture2d noise_tex{4, 4, GL_RGBA32F, GL_RGB, GL_FLOAT, GL_REPEAT, value_ptr(*noise.data())};

        g_buffer_.value().depth_texture().bind_unit(0);
        g_buffer_.value().color_texture_at(0).bind_unit(1);
        noise_tex.bind_unit(2);

        ssao_projection_.set(projection);
        ssao_view_.set(inverse(transpose(cam.view())));
        ssao_inverse_projection_.set(glm::inverse(projection));
        ssao_program_.use();
        quad.draw();

        // ssao blur pass
        auto *p_ssao_color = &ssao_buffer_.value().color_texture();

        if (blur_)
        {
            ssao_blur_buffer_.value().bind();
            ssao_buffer_.value().color_texture().bind_unit(0);
            ssao_blur_program_.use();

            quad.draw();
            p_ssao_color = &ssao_blur_buffer_.value().color_texture();
        }

        // final pass

        frame_buffer::unbind_all();
        f_buffer_.value().color_texture().bind_unit(0);
        p_ssao_color->bind_unit(1);
        ssao_final_program_.use();
        quad.draw();
    }

private:
    float rand_float()
    {
        static std::default_random_engine rng{std::random_device{}()};
        static std::uniform_real_distribution<float> dist{0.0f, 1.0f};

        return dist(rng);
    }

    int kernel_size_{64};
    float kernel_radius_{0.5f};
    bool use_normal_{true}, white_shading_{};
    float ssao_weight_{1}, bias_{0.025f};
    bool blur_{true}, has_range_check_{true};

    GLsizei screen_width_, screen_height_;

    void reset_frame_buffer()
    {
        std::vector<texture2d> g_textures;
        g_textures.emplace_back(screen_width_, screen_height_, 0, GL_RG16_SNORM); // octahedral normal
        g_textures.emplace_back(screen_width_, screen_height_, 0, GL_RGB8);       // albedo
        g_textures.emplace_back(screen_width_, screen_height_, 0, GL_RGB8);       // specular

        g_buffer_.emplace(std::move(g_textures), screen_width_, screen_height_, 0);

        ssao_buffer_.emplace(texture2d{screen_width_, screen_height_, 0, GL_R8}, screen_width_, screen_height_, 0);

        ssao_blur_buffer_.emplace(texture2d{screen_width_, screen_height_, 0, GL_R8}, screen_width_, screen_height_, 0);

        f_buffer_.emplace(screen_width_, screen_height_, 0);

        lighting_frame_size_.set_vec2({screen_width_, screen_height_});
        ssao_frame_size_.set_vec2({screen_width_, screen_height_});
    }

    glm::vec3 dir_light_dir_{0.0f, 1.0f, 0.2f};
    glm::vec3 dir_light_color_{0.5f, 0.5f, 0.5f};
    glm::vec3 ambient_light_color_{0.2f, 0.2f, 0.2f};

    std::optional<frame_buffer> g_buffer_{};
    std::optional<frame_buffer> ssao_buffer_{};
    std::optional<frame_buffer> ssao_blur_buffer_{};
    std::optional<frame_buffer> f_buffer_{};

    model backpack_{model::load_file("resources/models/backpack_modified/backpack.obj", texture_type::diffuse | texture_type::specular | texture_type::normal)};

    shader_program g_buffer_program_{make_vf_program(
        "shaders/deferred/g_buffer_no_position_vs.glsl"_path,
        "shaders/deferred/g_buffer_no_position_fs.glsl"_path,
        "diffuseTexture", 0,
        "specularTexture", 1,
        "normalTexture", 2)};
    shader_uniform g_projection_{g_buffer_program_.uniform("projection")};
    shader_uniform g_model_{g_buffer_program_.uniform("model")};
    shader_uniform g_view_{g_buffer_program_.uniform("view")};
    shader_uniform g_normal_mat_{g_buffer_program_.uniform("normalMat")};

    shader_program g_plane_program_{make_vf_program(
        "shaders/ssao/g_plane_vs.glsl"_path,
        "shaders/ssao/g_plane_fs.glsl"_path)};
    shader_uniform plane_projection_{g_plane_program_.uniform("projection")};
    shader_uniform plane_view_{g_plane_program_.uniform("view")};
    shader_uniform plane_model_{g_plane_program_.uniform("model")};
    shader_uniform plane_normal_mat_{g_plane_program_.uniform("normalMat")};

    texture2d plane_tex_{"resources/textures/wood.png", true};

    shader_program lighting_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/ssao/ssao_lighting_fs.glsl"_path,
        "depthTexture", 0,
        "inputNormal", 1,
        "input1", 2,
        "input2", 3,
        "dirLightColor", dir_light_color_,
        "dirLightDir", dir_light_dir_,
        "ambientLightColor", ambient_light_color_)};
    shader_uniform lighting_view_pos_{lighting_program_.uniform("viewPos")};
    shader_uniform lighting_frame_size_{lighting_program_.uniform("frameSize")};
    shader_uniform lighting_inverse_view_projection_{lighting_program_.uniform("inverseViewProjection")};
    shader_uniform lighting_white_shading_{lighting_program_.uniform("whiteShading", white_shading_)};

    shader_program ssao_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/ssao/ssao_fs.glsl"_path,
        "depthTexture", 0,
        "normalTexture", 1,
        "noiseTexture", 2)};
    shader_uniform ssao_projection_{ssao_program_.uniform("projection")};
    shader_uniform ssao_view_{ssao_program_.uniform("view")};
    shader_uniform ssao_inverse_projection_{ssao_program_.uniform("inverseProjection")};
    shader_uniform ssao_frame_size_{ssao_program_.uniform("frameSize")};
    shader_uniform ssao_kernel_{ssao_program_.uniform("kernel")};
    shader_uniform ssao_use_normal_{ssao_program_.uniform("useNormal", use_normal_)};
    shader_uniform ssao_normal_view_mat_{ssao_program_.uniform("normalViewMat")};
    shader_uniform ssao_kernel_size_{ssao_program_.uniform("kernelSize", kernel_size_)};
    shader_uniform ssao_kernel_radius_{ssao_program_.uniform("kernelRadius", kernel_radius_)};
    shader_uniform ssao_has_range_check_{ssao_program_.uniform("hasRangeCheck", has_range_check_)};
    shader_uniform ssao_bias_{ssao_program_.uniform("bias", bias_)};

    shader_program ssao_blur_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/ssao/ssao_blur_fs.glsl"_path,
        "ssaoInput", 0)};

    shader_program ssao_final_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/ssao/ssao_final_fs.glsl"_path,
        "screenTexture", 0,
        "ssaoTexture", 1)};
    shader_uniform ssao_final_weight_{ssao_final_program_.uniform("weight", ssao_weight_)};
    shader_uniform fbuffer_exposure_{ssao_final_program_.uniform("exposure", 3.0f)};
};

std::unique_ptr<example> create_ssao()
{
    return std::make_unique<ssao>();
}
