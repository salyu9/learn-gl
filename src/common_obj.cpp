#include "common_obj.hpp"

using namespace std::literals;
using namespace glwrap;

// ----------- box ---------------
struct box::box_impl
{
    glm::vec4 color_;
    glm::vec3 position_;
    glm::vec3 size_;

    shader_program program_{
        shader::compile_file("shaders/common/box_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/common/box_fs.glsl", shader_type::fragment),
    };
    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform view_model_{program_.uniform("viewModel")};
    shader_uniform color_uniform_{program_.uniform("color")};

    vertex_array varray_{vertex_array::load_simple_json("resources/simple_vertices/common_box.jsonc")};

    box_impl(glm::vec3 const &position, glm::vec3 const &size, glm::vec4 const &color)
        : position_(position), size_(size), color_(color)
    { }

    void draw(glm::mat4 const& projection, glm::mat4 const& view)
    {
        program_.use();

        projection_.set_mat4(projection);
        view_model_.set_mat4(view * glm::scale(glm::translate(glm::mat4(1), position_), size_));
        color_uniform_.set_vec4(color_);

        varray_.bind();
        varray_.draw(draw_mode::triangles, 0, 36);
    }

    void set_render_bright(bool value)
    {
        program_.uniform("renderBright").set_bool(value);
    }
};

box::box(glm::vec3 const &position, glm::vec3 const &size, glm::vec4 const &color)
    : impl_(std::make_unique<box_impl>(position, size, color))
{ }

box::box(box&& other) noexcept
    : impl_(std::move(other.impl_))
{ }

box & box::operator=(box&& other) noexcept
{
    impl_ = std::move(other.impl_);
    return *this;
}

box::~box() = default;

void box::draw(glm::mat4 const &projection, glm::mat4 const &view)
{
    impl_->draw(projection, view);
}

glm::vec3 const &box::get_position() const noexcept
{
    return impl_->position_;
}

void box::set_position(glm::vec3 const &position) noexcept
{
    impl_->position_ = position;
}

glm::vec3 const &box::get_size() const noexcept
{
    return impl_->size_;
}

void box::set_size(glm::vec3 const &size) noexcept
{
    impl_->size_ = size;
}

glm::vec4 box::get_color() const noexcept
{
    return impl_->color_;
}

void box::set_color(glm::vec4 const &color) noexcept
{
    impl_->color_ = color;
}

void box::set_render_bright(bool value) noexcept
{
    impl_->set_render_bright(value);
}
// ------------ wooden box --------------------

struct wooden_box::wooden_box_impl
{
    glm::mat4 transform_;

    shader_program program_{
        shader::compile_file("shaders/common/wooden_box_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/common/wooden_box_fs.glsl", shader_type::fragment),
    };
    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform view_{program_.uniform("view")};
    shader_uniform model_{program_.uniform("model")};
    shader_uniform view_position_{program_.uniform("viewPosition")};
    shader_uniform normal_mat_{program_.uniform("normalMat")};
    shader_uniform diffuse_{program_.uniform("diffuseTexture")};
    shader_uniform specular_{program_.uniform("specularTexture")};

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
    std::array<light_uniform_t, 4> light_uniforms{
        light_uniform_t{program_, 0},
        light_uniform_t{program_, 1},
        light_uniform_t{program_, 2},
        light_uniform_t{program_, 3},
    };

    texture2d diffuse_tex_{"resources/textures/container2.png"sv, true};
    texture2d specular_tex_{"resources/textures/container2_specular.png"sv, true};

    vertex_array varray_{vertex_array::load_simple_json("resources/simple_vertices/wooden_box.jsonc")};

    wooden_box_impl()
        : transform_(glm::mat4(1))
    {
        diffuse_.set_int(0);
        specular_.set_int(1);
    }

    void set_point_light(size_t index, glm::vec3 const &position, glm::vec3 const &attenuation, glm::vec3 const &color)
    {
        auto &light = light_uniforms.at(index);
        light.position.set_vec3(position);
        light.attenuation.set_vec3(attenuation);
        light.color.set_vec3(color);
    }

    void draw(glm::mat4 const &projection, camera & cam)
    {
        program_.use();

        projection_.set_mat4(projection);

        auto view = cam.view();
        model_.set_mat4(transform_);
        view_.set_mat4(view);
        normal_mat_.set_mat4(glm::transpose(glm::inverse(transform_)));
        view_position_.set_vec3(cam.position());
        diffuse_tex_.bind_unit(0);
        specular_tex_.bind_unit(1);
        varray_.bind();
        varray_.draw(draw_mode::triangles, 0, 36);
    }

    void set_render_bright(bool value)
    {
        program_.uniform("renderBright").set_bool(value);
    }
};

wooden_box::wooden_box()
    : impl_(std::make_unique<wooden_box_impl>())
{
}

wooden_box::wooden_box(wooden_box &&other) noexcept
    : impl_(std::move(other.impl_))
{
}

wooden_box &wooden_box::operator=(wooden_box &&other) noexcept
{
    impl_ = std::move(other.impl_);
    return *this;
}

wooden_box::~wooden_box() = default;

void wooden_box::draw(glm::mat4 const &projection, camera & cam)
{
    impl_->draw(projection, cam);
}

glm::mat4 const &wooden_box::get_transform() const noexcept
{
    return impl_->transform_;
}

void wooden_box::set_transform(glm::mat4 const &transform) noexcept
{
    impl_->transform_ = transform;
}

void wooden_box::set_point_light(int index, glm::vec3 const &position, glm::vec3 const &attenuation, glm::vec3 const &color)
{
    impl_->set_point_light(index, position, attenuation, color);
}

void wooden_box::set_render_bright(bool value) noexcept
{
    impl_->set_render_bright(value);
}