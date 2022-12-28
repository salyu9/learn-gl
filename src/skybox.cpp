#include "glwrap.hpp"
#include "skybox.hpp"
#include "utils.hpp"

using namespace glwrap;

struct skybox::skybox_impl final {
    std::optional<cubemap> holded_;
    cubemap *p_cubemap_;
    shader_program program_{
        shader::compile_file("shaders/base/skybox_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/base/skybox_fs.glsl", shader_type::fragment)};
    shader_uniform proj_uniform_{program_.uniform("projection")};
    shader_uniform view_uniform_{program_.uniform("view")};

    skybox_impl(cubemap &cubemap)
        : p_cubemap_(&cubemap)
    {
        program_.uniform("skybox").set_int(0);
    }

    skybox_impl(cubemap &&cubemap)
        : holded_{std::move(cubemap)}, p_cubemap_{&holded_.value()}
    {
        program_.uniform("skybox").set_int(0);
    }
};

skybox::skybox(cubemap &cubemap)
    : impl_{std::make_unique<skybox_impl>(cubemap)}
{ }

skybox::skybox(cubemap &&cubemap)
    : impl_{std::make_unique<skybox_impl>(std::move(cubemap))}
{ }

skybox::skybox(skybox &&) noexcept = default;
skybox::~skybox() = default;

skybox &skybox::operator = (skybox &&) noexcept = default;

void skybox::draw(glm::mat4 const & proj_mat, glm::mat4 const & view_mat)
{
    // glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);
    impl_->program_.use();
    impl_->proj_uniform_.set_mat4(proj_mat);
    impl_->view_uniform_.set_mat4(glm::mat4(glm::mat3(view_mat)));
    impl_->p_cubemap_->bind_unit(0);
    auto &varray = utils::get_skybox();
    varray.bind();
    varray.draw(draw_mode::triangles);
    //  glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
}
