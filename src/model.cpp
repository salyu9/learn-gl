#include <iostream>
#include <map>
#include <vector>
#include <format>
#include <cmath>
#include <span>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "glwrap.hpp"
#include "utils.hpp"
#include "model.hpp"

namespace glwrap
{
    inline glm::vec2 to_vec2(aiVector2D const &vec)
    {
        return {vec.x, vec.y};
    }

    inline glm::vec3 to_vec3(aiVector3D const &vec)
    {
        return {vec.x, vec.y, vec.z};
    }

    struct mesh::mesh_impl final
    {
        mesh_impl(
            std::string name,
            std::vector<vertex> const &vertices,
            std::vector<uint32_t> const &indices)
            : name(std::move(name)),
              vbuffer(vertices),
              ibuffer(indices),
              varray(auto_vertex_array(ibuffer, vbuffer)),
              parent(nullptr)
        {
        }

        void add_textures(texture_type tex_type, std::vector<uint32_t> const & indices)
        {
            for (auto i : indices) {
                textures.emplace(tex_type, i);
            }
        }

        std::string name;
        vertex_buffer<vertex> vbuffer;
        index_buffer<uint32_t> ibuffer;
        vertex_array varray;

        std::map<texture_type, uint32_t> textures;

        model *parent;
    };
    
    mesh::mesh()
    { }

    mesh::mesh(mesh && other) noexcept
        : impl_(std::move(other.impl_))
    { }

    mesh::~mesh() noexcept = default;

    bool mesh::has_texture(texture_type type) const noexcept
    {
        return impl_->textures.find(type) != impl_->textures.end();
    }

    mesh &mesh::operator=(mesh &&other) noexcept
    {
        impl_ = std::move(other.impl_);
        return *this;
    }

    vertex_buffer<vertex> & mesh::get_vbuffer() noexcept
    {
        return impl_->vbuffer;
    }

    index_buffer<uint32_t> & mesh::get_ibuffer() noexcept
    {
        return impl_->ibuffer;
    }

    vertex_array & mesh::get_varray() noexcept
    {
        return impl_->varray;
    }

    struct model::model_impl final
    {
        std::filesystem::path directory_;
        std::vector<mesh> meshes_;
        std::vector<texture2d> textures_;
        std::map<std::string, uint32_t> texture_map_;

        void load_file(std::filesystem::path const &path, texture_type tex_types)
        {
            Assimp::Importer importer;
            auto ai_scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!ai_scene || ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !ai_scene->mRootNode)
            {
                throw std::invalid_argument(std::string("Load model failed: ") + importer.GetErrorString());
            }
            directory_ = path.parent_path();
            import_node(ai_scene->mRootNode, ai_scene, tex_types);
        }

        void import_node(aiNode const *ai_node, aiScene const *ai_scene, texture_type tex_types)
        {
            for (auto mesh_index : utils::ptr_range(ai_node->mMeshes, ai_node->mNumMeshes))
            {
                import_mesh(ai_scene->mMeshes[mesh_index], ai_scene, tex_types);
            }
            for (auto *child : utils::ptr_range(ai_node->mChildren, ai_node->mNumChildren))
            {
                import_node(child, ai_scene, tex_types);
            }
        }

        void import_mesh(aiMesh const *ai_mesh, aiScene const *ai_scene, texture_type tex_types)
        {
            std::vector<vertex> vertices;
            std::vector<uint32_t> indices;

            for (auto i : utils::range(ai_mesh->mNumVertices))
            {
                auto position = to_vec3(ai_mesh->mVertices[i]);
                auto normal = to_vec3(ai_mesh->mNormals[i]);
                auto texcoords = ai_mesh->mTextureCoords[0]
                                     ? to_vec3(ai_mesh->mTextureCoords[0][i])
                                     : glm::vec2(0.0f, 0.0f);

                auto tangent = to_vec3(ai_mesh->mTangents[i]);
                auto bitangent = to_vec3(ai_mesh->mBitangents[i]);

                vertices.push_back({position, normal, texcoords, tangent, bitangent});
            }
            for (auto &&face : utils::ptr_range(ai_mesh->mFaces, ai_mesh->mNumFaces))
            {
                std::copy(face.mIndices, face.mIndices + face.mNumIndices, std::back_inserter(indices));
            }

            auto ai_material = ai_scene->mMaterials[ai_mesh->mMaterialIndex];

            meshes_.push_back(create_mesh());
            auto &mesh = meshes_.back();
            mesh.impl_ = std::make_unique<mesh::mesh_impl>(ai_mesh->mName.C_Str(), std::move(vertices), std::move(indices));
            if ((tex_types & texture_type::diffuse) != texture_type::none)
            {
                mesh.impl_->add_textures(texture_type::diffuse, load_textures(ai_material, aiTextureType_DIFFUSE, texture_type::diffuse, ai_scene));
            }
            if ((tex_types & texture_type::specular) != texture_type::none)
            {
                mesh.impl_->add_textures(texture_type::specular, load_textures(ai_material, aiTextureType_SPECULAR, texture_type::specular, ai_scene));
            }
            if ((tex_types & texture_type::normal) != texture_type::none)
            {
                mesh.impl_->add_textures(texture_type::normal, load_textures(ai_material, aiTextureType_HEIGHT, texture_type::normal, ai_scene));
            }
            if ((tex_types & texture_type::height) != texture_type::none) {
                mesh.impl_->add_textures(texture_type::height, load_textures(ai_material, aiTextureType_AMBIENT, texture_type::diffuse, ai_scene));
            }
        }

        std::vector<uint32_t> load_textures(aiMaterial *ai_material, aiTextureType ai_tex_type, texture_type tex_type, aiScene const *ai_scene)
        {
            std::vector<uint32_t> result{};
            for (auto i : utils::range(ai_material->GetTextureCount(ai_tex_type)))
            {
                aiString str;
                ai_material->GetTexture(ai_tex_type, i, &str);

                // find texture in loaded
                std::string path(str.C_Str());
                auto iter = texture_map_.find(path);

                if (iter == texture_map_.end())
                {
                    auto new_index = static_cast<uint32_t>(textures_.size());
                    texture_map_.emplace(path, new_index);
                    result.push_back(new_index);
                    if (auto texture = ai_scene->GetEmbeddedTexture(str.C_Str()))
                    {
                        if (texture->mHeight != 0)
                        {
                            throw std::runtime_error("Cannot load embeded texture");
                        }
                        textures_.emplace_back(texture->pcData, texture->mWidth);
                    }
                    else
                    {
                        if (tex_type == texture_type::diffuse) {
                            textures_.emplace_back((directory_ / path).string(), true);
                        }
                        else {
                            textures_.emplace_back((directory_ / path).string());
                        }
                    }
                }
                else
                {
                    result.push_back(iter->second);
                }
            }
            return result;
        }
    };

    texture2d &mesh::get_texture(texture_type type)
    {
        auto iter = impl_->textures.lower_bound(type);
        if (iter == impl_->textures.end())
        {
            throw std::runtime_error(std::format("Cannot get {} texture from mesh", to_string(type)));
        }
        if (impl_->parent == nullptr)
        {
            throw std::runtime_error("Mesh has no parent model");
        }
        return impl_->parent->get_texture(iter->second);
    }

    model::model() : impl_{std::make_unique<model_impl>()}
    { }

    model::model(model && other) noexcept : impl_{std::move(other.impl_)}
    {
        for (auto &mesh : impl_->meshes_)
        {
            mesh.impl_->parent = this;
        }
    }

    model &model::operator=(model &&other) noexcept
    {
        impl_ = std::move(other.impl_);
        for (auto &mesh : impl_->meshes_)
        {
            mesh.impl_->parent = this;
        }
        return *this;
    }

    model::~model() = default;

    model model::load_file(std::filesystem::path const &path, texture_type tex_types)
    {
        auto m = model{};
        m.impl_->load_file(path, tex_types);
        for (auto & mesh : m.impl_->meshes_) {
            mesh.impl_->parent = &m;
        }
        return m;
    }

    mesh model::create_mesh()
    {
        return mesh{};
    }

    std::vector<mesh> &model::meshes()
    {
        return impl_->meshes_;
    }

    texture2d &model::get_texture(uint32_t i)
    {
        return impl_->textures_.at(i);
    }
}
