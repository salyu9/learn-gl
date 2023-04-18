#include <array>
#include <limits>
#include <utility>
#include <random>

#include "glwrap.hpp"
#include "common_obj.hpp"
#include "examples.hpp"
#include "skybox.hpp"
#include "imgui.h"

using namespace glwrap;

inline glm::vec4 transform_point(glm::mat4 const &transform, glm::vec4 const &v)
{
    auto v4 = transform * v;
    return v4 / v4.w;
}

class cascaded_shadow_map final : public example
{
    const int shadow_map_unit = 3;

public:
    bool custom_render() override { return true; }

    void reset_frame_buffer(GLsizei width, GLsizei height) override
    {
        screen_width_ = width;
        screen_height_ = height;
        fbuffer_.emplace(width, height, 0, true);
    }

    void switch_state(int i) override
    {
        draw_type_ = i;
    }

    std::vector<std::string> const &get_states() override
    {
        static std::vector<std::string> states = []
        {
            std::vector<std::string> res{"scene"};
            for (size_t i = 0; i < cascaded_level_count; ++i)
            {
                res.push_back("shadow map " + std::to_string(i));
            }
            return res;
        }();
        return states;
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(-4.0f, 3.0f, 3.0f), {}, 0.1f, 1000.0f);
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        shadow_fb_.bind();
        shadow_fb_.clear_depth();
        glViewport(0, 0, shadow_map_width, shadow_map_height);

        auto near_z = cam.near_z(), far_z = cam.far_z(), range_z = far_z - near_z;

        for (int i = 0; i < cascaded_level_count; ++i)
        {
            auto nz = cascaded_levels[i] * range_z + near_z;
            auto fz = cascaded_levels[i + 1] * range_z + near_z;
            auto inv = glm::inverse(cam.projection(nz, fz) * cam.view());
            // auto inv = glm::inverse(cam.projection() * cam.view());

            std::array corners{
                transform_point(inv, glm::vec4(-1, -1, -1, 1)),
                transform_point(inv, glm::vec4(-1, -1, +1, 1)),
                transform_point(inv, glm::vec4(-1, +1, -1, 1)),
                transform_point(inv, glm::vec4(-1, +1, +1, 1)),
                transform_point(inv, glm::vec4(+1, -1, -1, 1)),
                transform_point(inv, glm::vec4(+1, -1, +1, 1)),
                transform_point(inv, glm::vec4(+1, +1, -1, 1)),
                transform_point(inv, glm::vec4(+1, +1, +1, 1)),
            };
            auto sum = glm::vec4();
            for (auto &corner : corners)
            {
                sum += corner;
            }
            auto center = glm::vec3(sum);
            center /= corners.size();
            auto light_view = glm::lookAt(center + light_dir, center, glm::vec3(0, 1, 0));

            auto min_x = std::numeric_limits<float>::max(),
                 min_y = std::numeric_limits<float>::max(),
                 min_z = std::numeric_limits<float>::max(),
                 max_x = std::numeric_limits<float>::lowest(),
                 max_y = std::numeric_limits<float>::lowest(),
                 max_z = std::numeric_limits<float>::lowest();
            for (auto &corner : corners)
            {
                auto trf = transform_point(light_view, corner);
                min_x = std::min(min_x, trf.x);
                min_y = std::min(min_y, trf.y);
                min_z = std::min(min_z, trf.z);
                max_x = std::max(max_x, trf.x);
                max_y = std::max(max_y, trf.y);
                max_z = std::max(max_z, trf.z);
            }
            constexpr float z_mult = 10.0f;
            min_z = min_z < 0 ? min_z * z_mult : min_z / z_mult;
            max_z = max_z < 0 ? max_z / z_mult : max_z * z_mult;
            auto light_projection = glm::ortho(min_x, max_x, min_y, max_y, min_z, max_z);
            light_space_mats_[i] = light_projection * light_view;
        }

        glCullFace(GL_FRONT);
        draw_scene(projection, nullptr, true);
        glCullFace(GL_BACK);

        glViewport(0, 0, screen_width_, screen_height_);

        if (draw_type_ == 0)
        {
            auto &fb = fbuffer_.value();
            fb.clear({0.1f, 0.1f, 0.1f, 1.0f});
            fb.bind();
            shadow_fb_.depth_texture_array().bind_unit(shadow_map_unit);

            draw_scene(projection, &cam, false);

            box_.set_position(light_dir);
            box_.draw(projection, cam.view());

            box_.set_position({});
            box_.draw(projection, cam.view());

            glDisable(GL_DEPTH_TEST);
            frame_buffer::unbind_all();
            fb_program_.use();
            fb.color_texture().bind_unit(0);
            auto &qarray = utils::get_quad_varray();
            qarray.draw(draw_mode::triangles);
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
            frame_buffer::unbind_all();
            shadow_fb_.depth_texture_array().bind_unit(0);
            cascaded_shadow_debug_program_.use();
            cascaded_shadow_debug_layer_.set(draw_type_ - 1);
            auto &qarray = utils::get_quad_varray();
            qarray.draw(draw_mode::triangles);
            glEnable(GL_DEPTH_TEST);
        }
    }

    void draw_gui() override
    {
        // ImGui::Image((void *)(intptr_t)shadow_fb_.depth_texture_array().handle(), {256, 256}, {0, 1}, {1, 0});
        if (ImGui::Button("Regenerate"))
        {
            box_transforms_ = generate_transforms();
        }
    }

private:
    constexpr static int cascaded_level_count = 5;
    constexpr static std::array cascaded_levels{0.0f, 1 / 50.0f, 1 / 25.0f, 1 / 10.0f, 1 / 2.0f, 1.0f};

    glm::vec3 light_dir{glm::normalize(glm::vec3(20.0f, 50, 20.0f))};
    glm::vec3 light_color{1.0f, 1.0f, 1.0f};
    glm::vec3 ambient_light{0.1f, 0.1f, 0.1f};

    std::vector<glm::mat4> box_transforms_ = generate_transforms();

    std::vector<glm::mat4> generate_transforms()
    {
        static std::uniform_real_distribution<float> offset_dist = std::uniform_real_distribution<float>(-10, 10);
        static std::uniform_real_distribution<float> scale_dist = std::uniform_real_distribution<float>(1.0, 2.0);
        static std::uniform_real_distribution<float> rotation_dist = std::uniform_real_distribution<float>(0, 180);

        std::default_random_engine rng{std::random_device()()};

        std::vector<glm::mat4> res;
        for (int i = 0; i < 10; ++i)
        {
            auto model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(offset_dist(rng), offset_dist(rng) + 10.0f, offset_dist(rng)));
            model = glm::rotate(model, glm::radians(rotation_dist(rng)), glm::normalize(glm::vec3(1.0, 0.0, 1.0)));
            model = glm::scale(model, glm::vec3(scale_dist(rng)));
            res.push_back(model);
        }
        return res;
    }

    std::array<glm::mat4, cascaded_level_count> light_space_mats_;

    void draw_scene(glm::mat4x4 const &proj, camera *cam, bool shadow_casting)
    {
        floor_tex_.bind_unit(0);
        if (shadow_casting)
        {
            shadow_cast_program_.use();
            for (int i = 0; i < cascaded_level_count; ++i)
            {
                shadow_cast_mats_[i].set(light_space_mats_[i]);
            }
            shadow_cast_model_.set(glm::mat4(1.0f));
        }
        else
        {
            floor_program_.use();
            floor_projection_.set(proj);
            floor_view_.set(cam->view());
            floor_model_.set(glm::mat4(1.0f));
            floor_normal_mat_.set(transpose(inverse(glm::mat4(1.0f))));
            floor_view_position_.set(cam->position());
            floor_dir_light_color_.set(light_color);
            floor_dir_light_dir_.set(light_dir);
            for (int i = 0; i < cascaded_level_count; ++i)
            {
                floor_light_space_mats_[i].set(light_space_mats_[i]);
            }
            floor_ambient_light_.set(ambient_light);

            auto near_z = cam->near_z(), far_z = cam->far_z(), range_z = far_z - near_z;
            for (int i = 0; i < cascaded_level_count; ++i) {
                floor_cascade_plane_distances_[i].set(cascaded_levels[i + 1] * range_z + near_z);
                wbox_cascade_plane_distances_[i].set(cascaded_levels[i + 1] * range_z + near_z);
            }
        }

        floor_varray_.draw(draw_mode::triangles);

        if (!shadow_casting)
        {
            wbox_.set_dir_light(light_dir, light_color);
            wbox_.set_ambient_light(ambient_light);
        }

        // cubes
        for (auto &trans : box_transforms_)
        {
            wbox_.set_transform(trans);
            shadow_cast_model_.set(trans);
            wbox_.draw(proj, *cam, shadow_casting ? &shadow_cast_program_ : nullptr);
        }
    }

    constexpr static int shadow_map_width = 1024, shadow_map_height = 1024;
    float screen_width_, screen_height_;

    frame_buffer shadow_fb_ = []
    {
        texture2d_array shadow_tex{shadow_map_width, shadow_map_height, cascaded_level_count, 0, GL_DEPTH_COMPONENT32F, GL_CLAMP_TO_BORDER};
        shadow_tex.set_border_color({1.0f, 1.0f, 1.0f, 1.0f});
        return frame_buffer{std::vector<texture2d>{},
                            std::move(shadow_tex),
                            shadow_map_width, shadow_map_height, 0};
    }();
    std::optional<frame_buffer> fbuffer_{};

    struct floor_vert_t
    {
        using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2>;
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 tex_coord;
    };

    vertex_array floor_varray_{auto_vertex_array(
        vertex_buffer<floor_vert_t>{
            // positions              // normals          // texcoords
            {{25.0f, -0.5f, 25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f, 0.0f}},
            {{-25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 25.0f}},
            {{-25.0f, -0.5f, 25.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
            {{25.0f, -0.5f, 25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f, 0.0f}},
            {{25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, {25.0f, 25.0f}},
            {{-25.0f, -0.5f, -25.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 25.0f}},
        })};
    texture2d floor_tex_{"resources/textures/wood.png"_path, true, texture2d_elem_type::u8, texture2d_format::unspecified};
    shader_program floor_program_{make_vf_program(
        "shaders/common/simple_position_normal_texcoord_vs.glsl"_path,
        "shaders/cascaded_shadow_blinn_phong_fs.glsl"_path,
        "diffuseTexture", 0,
        "specularTexture", 1,
        "dirLight.shadowMap", shadow_map_unit,
        "dirLight.cascadeCount", cascaded_level_count,
        "hasDirLight", true)};
    shader_uniform floor_projection_{floor_program_.uniform("projection")};
    shader_uniform floor_model_{floor_program_.uniform("model")};
    shader_uniform floor_view_{floor_program_.uniform("view")};
    shader_uniform floor_view_position_{floor_program_.uniform("viewPosition")};
    shader_uniform floor_normal_mat_{floor_program_.uniform("normalMat")};
    shader_uniform floor_dir_light_color_{floor_program_.uniform("dirLight.color")};
    shader_uniform floor_dir_light_dir_{floor_program_.uniform("dirLight.dir")};
    std::array<shader_uniform, cascaded_level_count> floor_light_space_mats_ = utils::make_uniform_array<cascaded_level_count>(floor_program_, "dirLight.lightSpaceMats");
    std::array<shader_uniform, cascaded_level_count> floor_cascade_plane_distances_ = utils::make_uniform_array<cascaded_level_count>(floor_program_, "dirLight.cascadePlaneDistances");
    shader_uniform floor_ambient_light_{floor_program_.uniform("ambientLight")};

    wooden_box wbox_{
        make_vf_program(
            "shaders/common/simple_position_normal_texcoord_vs.glsl"_path,
            "shaders/cascaded_shadow_blinn_phong_fs.glsl"_path,
            "dirLight.shadowMap", shadow_map_unit,
            "dirLight.cascadeCount", cascaded_level_count)};
    std::array<shader_uniform, cascaded_level_count> wbox_light_space_mats_ = utils::make_uniform_array<cascaded_level_count>(wbox_.program(), "dirLight.lightSpaceMats");
    std::array<shader_uniform, cascaded_level_count> wbox_cascade_plane_distances_ = utils::make_uniform_array<cascaded_level_count>(wbox_.program(), "dirLight.cascadePlaneDistances");

    shader_program shadow_cast_program_{make_vgf_program(
        "shaders/cascaded_shadow_cast_vs.glsl"_path,
        "shaders/cascaded_shadow_cast_gs.glsl"_path,
        "shaders/shadow_cast_fs.glsl"_path)};
    std::array<shader_uniform, cascaded_level_count> shadow_cast_mats_ = utils::make_uniform_array<cascaded_level_count>(shadow_cast_program_, "lightSpaceMats");

    shader_uniform shadow_cast_model_{shadow_cast_program_.uniform("model")};

    shader_program fb_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/hdr_exposure_fs.glsl"_path,
        "screenTexture", 0,
        "exposure", 1.0f)};

    shader_program cascaded_shadow_debug_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/cascaded_shadow_map_debug_fs.glsl"_path,
        "screenTexture", 0)};

    shader_uniform cascaded_shadow_debug_layer_{cascaded_shadow_debug_program_.uniform("layer")};

    int draw_type_{0};

    box box_{{}, glm::vec3(0.1f, 0.1f, 0.1f)};
};

std::unique_ptr<example> create_cascaded_shadow_map()
{
    return std::make_unique<cascaded_shadow_map>();
}