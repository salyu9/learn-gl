#pragma once

#include <memory>
#include <string>
#include <filesystem>

#include "glm/glm.hpp"

namespace glwrap
{
    struct vertex final
    {
        using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2, glm::vec3, glm::vec3>;
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texcoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    class model final
    {
    public:
        static model load_file(std::filesystem::path const &path);
        void draw(glm::mat4 const &projection, glm::mat4 const &model_view);
    private:
        model();
        friend class model_impl;
        std::shared_ptr<model_impl> impl_;
    };
}
