#include "glwrap.hpp"
#include "utils.hpp"
#include "examples.hpp"

#include "model.hpp"
#include "skybox.hpp"
#include "camera.hpp"

using namespace glwrap;

class backpack final : public example
{
public:
    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(-5.0f, 2.0f, 10.0f));
    }

    void draw(glm::mat4 const &projection, camera & cam) override
    {
        auto view = cam.view();

        skybox_.draw(projection, view);

        program_.use();
        projection_.set_mat4(projection);
        model_view_.set_mat4(view);
        for (auto & mesh : model_.meshes()) {
            auto & varray = mesh.get_varray();
            if (mesh.has_texture(texture_type::diffuse))
            {
                auto &texture = mesh.get_texture(texture_type::diffuse);
                texture.bind_unit(0);
                diffuse0_.set_int(0);
            }
            varray.draw(draw_mode::triangles);
        }
    }

private:
    skybox skybox_{glwrap::cubemap{"resources/cubemaps/skybox", ".jpg"}};
    model model_{model::load_file("resources/models/backpack_modified/backpack.obj", texture_type::diffuse | texture_type::specular)};

    shader_program program_{
        shader::compile_file("shaders/straight_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/straight_fs.glsl", shader_type::fragment),
    };

    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform model_view_{program_.uniform("modelView")};
    shader_uniform diffuse0_{program_.uniform("textureDiffuse0")};
};

std::unique_ptr<example> create_backpack()
{
    return std::make_unique<backpack>();
}