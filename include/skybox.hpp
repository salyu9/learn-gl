#include <memory>
#include "glwrap.hpp"

#pragma once

namespace glwrap
{
    class skybox final
    {
    public:
        skybox(cubemap &&);
        skybox(skybox &&) noexcept;
        ~skybox();
        void draw(glm::mat4x4 const &proj_mat, glm::mat4x4 const &view_mat);

    private:
        struct skybox_impl;
        std::unique_ptr<skybox_impl> impl_;
    };
}