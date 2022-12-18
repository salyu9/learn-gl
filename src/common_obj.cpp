#include "common_obj.hpp"

using namespace glwrap;

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

    vertex_buffer<glm::vec3> vbuffer_{
        {-0.5f, -0.5f, -0.5f},
        { 0.5f, -0.5f, -0.5f},  
        { 0.5f,  0.5f, -0.5f},  
        { 0.5f,  0.5f, -0.5f},  
        {-0.5f,  0.5f, -0.5f}, 
        {-0.5f, -0.5f, -0.5f}, 

        {-0.5f, -0.5f,  0.5f}, 
        { 0.5f, -0.5f,  0.5f},  
        { 0.5f,  0.5f,  0.5f},  
        { 0.5f,  0.5f,  0.5f},  
        {-0.5f,  0.5f,  0.5f}, 
        {-0.5f, -0.5f,  0.5f}, 

        {-0.5f,  0.5f,  0.5f}, 
        {-0.5f,  0.5f, -0.5f}, 
        {-0.5f, -0.5f, -0.5f}, 
        {-0.5f, -0.5f, -0.5f}, 
        {-0.5f, -0.5f,  0.5f}, 
        {-0.5f,  0.5f,  0.5f}, 

        { 0.5f,  0.5f,  0.5f},  
        { 0.5f,  0.5f, -0.5f},  
        { 0.5f, -0.5f, -0.5f},  
        { 0.5f, -0.5f, -0.5f},  
        { 0.5f, -0.5f,  0.5f},  
        { 0.5f,  0.5f,  0.5f},  

        {-0.5f, -0.5f, -0.5f}, 
        { 0.5f, -0.5f, -0.5f},  
        { 0.5f, -0.5f,  0.5f},  
        { 0.5f, -0.5f,  0.5f},  
        {-0.5f, -0.5f,  0.5f}, 
        {-0.5f, -0.5f, -0.5f}, 

        {-0.5f,  0.5f, -0.5f}, 
        { 0.5f,  0.5f, -0.5f},  
        { 0.5f,  0.5f,  0.5f},  
        { 0.5f,  0.5f,  0.5f},  
        {-0.5f,  0.5f,  0.5f}, 
        {-0.5f,  0.5f, -0.5f}, 
    };

    vertex_array varray_{auto_vertex_array(vbuffer_)};

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
        varray_.draw(draw_mode::triangles);
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
