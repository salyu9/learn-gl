#include <iostream>
#include <map>
#include <vector>
#include <cmath>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include "glwrap.hpp"
#include "utils.hpp"
#include "model.hpp"

namespace glwrap
{
    enum class texture_type
    {
        diffuse,
        specular,
    };

    inline std::string to_string(texture_type type)
    {
        switch (type)
        {
        case texture_type::diffuse:
            return "diffuse";
        case texture_type::specular:
            return "specular";
        default:
            return "unknown";
        }
    }

    struct mesh final
    {
        mesh(
            std::string name,
            std::vector<vertex> const &vertices,
            std::vector<uint32_t> const &indices,
            std::vector<uint32_t> diffuse_indices,
            std::vector<uint32_t> specular_indices)
            : name(std::move(name)),
              vbuffer(vertices),
              ibuffer(indices),
              varray(auto_vertex_array(ibuffer, vbuffer)),
              diffuse_indices(std::move(diffuse_indices)),
              specular_indices(std::move(specular_indices))
        {
        }

        void draw()
        {
            varray.bind();
            varray.draw(draw_mode::triangles);
        }

        std::string name;
        vertex_buffer<vertex> vbuffer;
        index_buffer<uint32_t> ibuffer;
        vertex_array varray;

        std::vector<uint32_t> diffuse_indices;
        std::vector<uint32_t> specular_indices;
    };

    inline glm::vec2 to_vec2(aiVector2D const &vec)
    {
        return {vec.x, vec.y};
    }
    inline glm::vec3 to_vec3(aiVector3D const &vec)
    {
        return {vec.x, vec.y, vec.z};
    }

    struct model::model_impl final
    {
        std::filesystem::path directory_;
        std::vector<mesh> meshes_;
        std::vector<texture2d> textures_;
        std::map<std::string, uint32_t> texture_map_;

        shader_program program_{
            shader::compile_file("shaders/straight_vs.glsl", shader_type::vertex),
            shader::compile_file("shaders/straight_fs.glsl", shader_type::fragment),
        };

        // shader_program program_{
        //     shader::compile_file("shaders/geometry/explode_vs.glsl", shader_type::vertex),
        //     shader::compile_file("shaders/geometry/explode_fs.glsl", shader_type::fragment),
        //     shader::compile_file("shaders/geometry/explode_gs.glsl", shader_type::geometry)
        // };

        void load_file(std::filesystem::path const &path)
        {
            Assimp::Importer importer;
            auto ai_scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            if (!ai_scene || ai_scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !ai_scene->mRootNode)
            {
                throw std::invalid_argument(std::string("Load model failed: ") + importer.GetErrorString());
            }
            directory_ = path.parent_path();
            import_node(ai_scene->mRootNode, ai_scene);
        }

        void import_node(aiNode const *ai_node, aiScene const *ai_scene)
        {
            for (auto mesh_index : utils::ptr_range(ai_node->mMeshes, ai_node->mNumMeshes))
            {
                import_mesh(ai_scene->mMeshes[mesh_index], ai_scene);
            }
            for (auto *child : utils::ptr_range(ai_node->mChildren, ai_node->mNumChildren))
            {
                import_node(child, ai_scene);
            }
        }

        void import_mesh(aiMesh const *ai_mesh, aiScene const *ai_scene)
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
            auto diffuse_maps = load_textures(ai_material, aiTextureType_DIFFUSE, texture_type::diffuse, ai_scene);
            // textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
            auto specular_maps = load_textures(ai_material, aiTextureType_SPECULAR, texture_type::specular, ai_scene);
            // textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
            // auto normalMaps = load_textures(ai_material, aiTextureType_HEIGHT, "texture_normal");
            // textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
            // auto heightMaps = load_textures(ai_material, aiTextureType_AMBIENT, "texture_height");
            // textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

            meshes_.emplace_back(ai_mesh->mName.C_Str(), std::move(vertices), std::move(indices), std::move(diffuse_maps), std::move(specular_maps));
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
                        textures_.emplace_back((directory_ / path).string());
                    }
                }
                else
                {
                    result.push_back(iter->second);
                }
            }
            return result;
        }

        void draw(glm::mat4 const &projection, glm::mat4 const &model_view)
        {
            program_.use();
            program_.uniform("projection").set_mat4(projection);
            program_.uniform("modelView").set_mat4(model_view);

            // program_.uniform("explodeDistance").set_float(2);
            // auto time_ratio = std::fmod(timer::time(), 3.0f) / 3.0f;
            // auto ratio = std::sqrt(time_ratio);
            // program_.uniform("explodeRatio").set_float(ratio);

            for (auto &mesh : meshes_)
            {
                mesh.varray.bind();
                if (!mesh.diffuse_indices.empty())
                {
                    auto &texture = textures_[mesh.diffuse_indices.front()];
                    texture.bind_unit(0);
                    program_.uniform("textureDiffuse0").set_int(0);
                }
                mesh.varray.draw(draw_mode::triangles);
            }
        }
    };

    model::model() : impl_{std::make_shared<model_impl>()}
    { }

    model model::load_file(std::filesystem::path const &path)
    {
        auto m = model{};
        m.impl_->load_file(path);
        return m;
    }

    void model::draw(const glm::mat4 &projection, const glm::mat4 &model_view)
    {
        impl_->draw(projection, model_view);
    }
}
