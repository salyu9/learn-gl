#include <memory>
#include "glwrap.hpp"

#pragma once

namespace glwrap
{
    class skybox final
    {
    public:
        skybox(cubemap &);
        skybox(cubemap &&);
        skybox(skybox &&) noexcept;
        ~skybox();
        skybox &operator=(skybox &&) noexcept;
        void draw(glm::mat4 const &proj_mat, glm::mat4 const &view_mat);

    private:
        struct skybox_impl;
        std::unique_ptr<skybox_impl> impl_;
    };
}