#include "examples.hpp"
#include "skybox.hpp"
#include "utils.hpp"

using namespace glwrap;

class basic_pbr : public example
{
public:
    basic_pbr()
    {
        program_.uniform("albedoTexture").set_int(0);
        program_.uniform("normalTexture").set_int(1);
        program_.uniform("metallicTexture").set_int(2);
        program_.uniform("roughnessTexture").set_int(3);
        program_.uniform("aoTexture").set_int(4);

    }

    bool is_hdr() override { return true; }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(-5.0f, 2.0f, 10.0f));
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        auto view = cam.view();

        skybox_.draw(projection, view);

        program_.use();
        projection_.set_mat4(projection);
        view_uniform_.set_mat4(view);
        model_uniform_.set_mat4(glm::mat4(1));
        for (auto &mesh : model_.meshes())
        {
            auto &varray = mesh.get_varray();
            varray.bind();
            if (mesh.has_texture(texture_type::diffuse))
            {
                auto &texture = mesh.get_texture(texture_type::diffuse);
                texture.bind_unit(0);
            }
            varray.draw(draw_mode::triangles);
        }
    }

private:
    skybox skybox_{glwrap::cubemap{"resources/cubemaps/skybox", ".jpg"}};
    model model_{model::load_file("resources/models/backpack_modified/backpack.obj", texture_type::diffuse | texture_type::specular)};

    shader_program program_{
        shader::compile_file("shaders/basic_pbr.glsl", shader_type::vertex),
        shader::compile_file("shaders/basic_pbr.glsl", shader_type::fragment),
    };

    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform view_uniform_{program_.uniform("view")};
    shader_uniform model_uniform_{program_.uniform("model")};
};

std::unique_ptr<example> create_basic_pbr()
{
    return std::make_unique<basic_pbr>();
}