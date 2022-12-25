#pragma once

#include <memory>
#include <string>
#include <filesystem>

#include "glm/glm.hpp"
#include "glwrap.hpp"

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

    enum class texture_type
    {
        none = 0,
        diffuse = 0x01,
        specular = 0x02,
        normal = 0x04,
        height = 0x08,

        // pbr
        albedo = 0x100,
        metallic = 0x200,
        roughness = 0x400,
        occlusion = 0x800,

    };

    inline texture_type operator|(texture_type a, texture_type b)
    {
        return static_cast<texture_type>(static_cast<std::underlying_type_t<texture_type>>(a) | static_cast<std::underlying_type_t<texture_type>>(b));
    }

    inline texture_type operator&(texture_type a, texture_type b)
    {
        return static_cast<texture_type>(static_cast<std::underlying_type_t<texture_type>>(a) & static_cast<std::underlying_type_t<texture_type>>(b));
    }

    inline std::string to_string(texture_type type)
    {
        switch (type)
        {
        case texture_type::diffuse:
            return "diffuse";
        case texture_type::specular:
            return "specular";
        case texture_type::normal:
            return "normal";
        case texture_type::height:
            return "height";
        default:
            return "unknown";
        }
    }

    class mesh
    {
    public:
        mesh(mesh &&) noexcept;
        mesh(mesh const &) = delete;
        virtual ~mesh();
        mesh & operator=(mesh &&other) noexcept;
        mesh & operator=(mesh const &) = delete;

        bool has_texture(texture_type) const noexcept;
        texture2d &get_texture(texture_type);

        vertex_buffer<vertex> &get_vbuffer() noexcept;
        index_buffer<uint32_t> &get_ibuffer() noexcept;
        vertex_array &get_varray() noexcept;

    private:
        friend class model;
        mesh();
        class mesh_impl;
        std::unique_ptr<mesh_impl> impl_;
    };

    class model final
    {
    public:
        model(model &&) noexcept;
        model(model const &) = delete;
        ~model();
        model &operator=(model &&other) noexcept;
        model &operator=(model const &) = delete;
        static model load_file(std::filesystem::path const &path, texture_type texture_types);
        std::vector<mesh> &meshes();

    private:
        friend class mesh;
        static mesh create_mesh();
        texture2d &get_texture(uint32_t);
        model();
        class model_impl;
        std::unique_ptr<model_impl> impl_;
    };
}
