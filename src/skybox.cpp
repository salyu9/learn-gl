#include "glwrap.hpp"
#include "skybox.hpp"

using namespace glwrap;

struct skybox::skybox_impl {
    vertex_buffer<glm::vec3> vbuffer_;
    vertex_array varray_;
    cubemap cubemap_;
    shader_program program_;
    shader_uniform proj_uniform_;
    shader_uniform view_uniform_;

    skybox_impl(cubemap &&cubemap)
        : vbuffer_{
              // positions
              glm::vec3(-1.0f, 1.0f, -1.0f),
              glm::vec3(-1.0f, -1.0f, -1.0f),
              glm::vec3(1.0f, -1.0f, -1.0f),
              glm::vec3(1.0f, -1.0f, -1.0f),
              glm::vec3(1.0f, 1.0f, -1.0f),
              glm::vec3(-1.0f, 1.0f, -1.0f),

              glm::vec3(-1.0f, -1.0f, 1.0f),
              glm::vec3(-1.0f, -1.0f, -1.0f),
              glm::vec3(-1.0f, 1.0f, -1.0f),
              glm::vec3(-1.0f, 1.0f, -1.0f),
              glm::vec3(-1.0f, 1.0f, 1.0f),
              glm::vec3(-1.0f, -1.0f, 1.0f),

              glm::vec3(1.0f, -1.0f, -1.0f),
              glm::vec3(1.0f, -1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, -1.0f),
              glm::vec3(1.0f, -1.0f, -1.0f),

              glm::vec3(-1.0f, -1.0f, 1.0f),
              glm::vec3(-1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, -1.0f, 1.0f),
              glm::vec3(-1.0f, -1.0f, 1.0f),

              glm::vec3(-1.0f, 1.0f, -1.0f),
              glm::vec3(1.0f, 1.0f, -1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(1.0f, 1.0f, 1.0f),
              glm::vec3(-1.0f, 1.0f, 1.0f),
              glm::vec3(-1.0f, 1.0f, -1.0f),

              glm::vec3(-1.0f, -1.0f, -1.0f),
              glm::vec3(-1.0f, -1.0f, 1.0f),
              glm::vec3(1.0f, -1.0f, -1.0f),
              glm::vec3(1.0f, -1.0f, -1.0f),
              glm::vec3(-1.0f, -1.0f, 1.0f),
              glm::vec3(1.0f, -1.0f, 1.0f),
          },
          varray_{auto_vertex_array(vbuffer_)},
          cubemap_{std::move(cubemap)},
          program_{shader::compile_file("shaders/base/skybox_vs.glsl", shader_type::vertex), shader::compile_file("shaders/base/skybox_fs.glsl", shader_type::fragment)}, proj_uniform_{program_.uniform("projection")}, view_uniform_{program_.uniform("view")}
    {
        program_.uniform("skybox").set_int(0);
    }
};

skybox::skybox(cubemap &&cubemap)
    : impl_{new skybox::skybox_impl{std::move(cubemap)}}
{ }

skybox::skybox(skybox &&) noexcept = default;
skybox::~skybox() = default;

void skybox::draw(glm::mat4x4 const & proj_mat, glm::mat4x4 const & view_mat)
{
    // glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    impl_->program_.use();
    impl_->proj_uniform_.set_mat4(proj_mat);
    impl_->view_uniform_.set_mat4(glm::mat4(glm::mat3(view_mat)));
    impl_->cubemap_.bind_unit(0);
    impl_->varray_.bind();
    impl_->varray_.draw(draw_mode::triangles);
    // glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}
