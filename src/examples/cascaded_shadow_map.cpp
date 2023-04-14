#include "glwrap.hpp"
#include "common_obj.hpp"
#include "examples.hpp"
#include "skybox.hpp"
#include "imgui.h"

using namespace glwrap;

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
        static std::vector<std::string> states{
            "scene",
            "shadow map",
        };
        return states;
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(-4.0f, 3.0f, 3.0f));
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        shadow_fb_.clear_depth();
        shadow_fb_.bind();
        glViewport(0, 0, shadow_map_width, shadow_map_height);
        glCullFace(GL_FRONT);
        draw_scene(projection, light_view_, true);
        glCullFace(GL_BACK);

        glViewport(0, 0, screen_width_, screen_height_);

        if (draw_type_ == 0)
        {
            auto &fb = fbuffer_.value();
            fb.clear({0.1f, 0.1f, 0.1f, 1.0f});
            fb.bind();
            shadow_fb_.depth_texture().bind_unit(shadow_map_unit);

            auto handle = shadow_fb_.depth_texture().handle();

            draw_scene(projection, cam, false);

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
            shadow_fb_.depth_texture().bind_unit(0);
            shadow_debug_program_.use();
            auto &qarray = utils::get_quad_varray();
            qarray.draw(draw_mode::triangles);
            glEnable(GL_DEPTH_TEST);
        }
    }

private:
    glm::vec3 light_dir{-2.0f, 4.0f, -1.0f};
    glm::vec3 light_color{1.0f, 1.0f, 1.0f};
    glm::vec3 ambient_light{0.1f, 0.1f, 0.1f};

    struct light_view_info : public view_info
    {
        glm::mat4 projection() const noexcept override
        {
            return projection_;
        }

        glm::mat4 view() const noexcept override
        {
            return view_;
        }

        glm::vec3 position() const noexcept override
        {
            return glm::vec3(0);
        }

        light_view_info(glm::mat4 const &proj, glm::mat4 const &view, glm::vec3 const &position)
            : projection_{proj}, view_{view}, position_{position}
        {
        }

        glm::mat4 projection_;
        glm::mat4 view_;
        glm::vec3 position_;

    } light_view_{
        glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 7.5f),
        glm::lookAt(light_dir,
                    glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::vec3(0)};

    void draw_scene(glm::mat4x4 const &proj, view_info &view_info, bool shadow_casting)
    {
        floor_tex_.bind_unit(0);
        auto light_space_mat = light_view_.projection() * light_view_.view();
        if (shadow_casting)
        {
            shadow_cast_program_.use();
            shadow_cast_projection_.set(light_view_.projection());
            shadow_cast_view_.set(light_view_.view());
            shadow_cast_model_.set(glm::mat4(1.0f));
        }
        else
        {
            floor_program_.use();
            floor_projection_.set(proj);
            floor_view_.set(view_info.view());
            floor_model_.set(glm::mat4(1.0f));
            floor_normal_mat_.set(transpose(inverse(glm::mat4(1.0f))));
            floor_view_position_.set(view_info.position());
            floor_dir_light_color_.set(light_color);
            floor_dir_light_dir_.set(light_dir);
            floor_light_space_mat_.set(light_space_mat);
            floor_ambient_light_.set(ambient_light);
        }

        floor_varray_.draw(draw_mode::triangles);

        if (!shadow_casting)
        {
            wbox_.set_dir_light(light_dir, light_color);
            wbox_.set_dir_light_space(light_space_mat, shadow_map_unit);
            wbox_.set_ambient_light(ambient_light);
        }

        // cubes
        auto trans = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 0.0f)), glm::vec3(0.5f));
        wbox_.set_transform(trans);
        shadow_cast_model_.set(trans);
        wbox_.draw(proj, view_info, shadow_casting ? &shadow_cast_program_ : nullptr);

        trans = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 1.0f)), glm::vec3(0.5f));
        wbox_.set_transform(trans);
        shadow_cast_model_.set(trans);
        wbox_.draw(proj, view_info, shadow_casting ? &shadow_cast_program_ : nullptr);

        trans = glm::scale(glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.0f, 2.0f)), glm::radians(60.0f), glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f))), glm::vec3(0.25));
        wbox_.set_transform(trans);
        shadow_cast_model_.set(trans);
        wbox_.draw(proj, view_info, shadow_casting ? &shadow_cast_program_ : nullptr);
    }

    constexpr static int shadow_map_width = 1024, shadow_map_height = 1024;
    float screen_width_, screen_height_;

    frame_buffer shadow_fb_ = []
    {
        texture2d shadow_tex{shadow_map_width, shadow_map_height, 0, GL_DEPTH_COMPONENT32F, GL_CLAMP_TO_BORDER};
        shadow_tex.set_border_color({1.0f, 1.0f, 1.0f, 1.0f});
        return frame_buffer{std::vector<texture2d>{},
                            std::move(shadow_tex),
                            shadow_map_width, shadow_map_height, 0};
    }();
    std::optional<frame_buffer> fbuffer_{};

    model model_{model::load_file("resources/models/backpack_modified/backpack.obj", texture_type::diffuse | texture_type::specular)};

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
        "shaders/common/simple_blinn_phong_fs.glsl"_path,
        "diffuseTexture", 0,
        "specularTexture", 1,
        "dirLight.shadowMap", shadow_map_unit,
        "hasDirLight", true)};
    shader_uniform floor_projection_{floor_program_.uniform("projection")};
    shader_uniform floor_model_{floor_program_.uniform("model")};
    shader_uniform floor_view_{floor_program_.uniform("view")};
    shader_uniform floor_view_position_{floor_program_.uniform("viewPosition")};
    shader_uniform floor_normal_mat_{floor_program_.uniform("normalMat")};
    shader_uniform floor_dir_light_color_{floor_program_.uniform("dirLight.color")};
    shader_uniform floor_dir_light_dir_{floor_program_.uniform("dirLight.dir")};
    shader_uniform floor_light_space_mat_{floor_program_.uniform("dirLight.lightSpaceMat")};
    shader_uniform floor_ambient_light_{floor_program_.uniform("ambientLight")};

    wooden_box wbox_;

    shader_program shadow_cast_program_{make_vf_program(
        "shaders/common/simple_position_normal_texcoord_vs.glsl"_path,
        "shaders/shadow_cast_fs.glsl"_path)};
    shader_uniform shadow_cast_projection_{shadow_cast_program_.uniform("projection")};
    shader_uniform shadow_cast_view_{shadow_cast_program_.uniform("view")};
    shader_uniform shadow_cast_model_{shadow_cast_program_.uniform("model")};

    shader_program fb_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/hdr_exposure_fs.glsl"_path,
        "screenTexture", 0,
        "exposure", 1.0f)};

    shader_program shadow_debug_program_{make_vf_program(
        "shaders/base/fbuffer_vs.glsl"_path,
        "shaders/base/shadow_map_debug_fs.glsl"_path,
        "screenTexture", 0)};

    int draw_type_{0};

    box box_{{}, glm::vec3(0.1f, 0.1f, 0.1f)};
};

std::unique_ptr<example> create_cascaded_shadow_map()
{
    return std::make_unique<cascaded_shadow_map>();
}