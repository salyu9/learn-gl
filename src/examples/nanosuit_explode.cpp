#include "glwrap.hpp"
#include "utils.hpp"
#include "examples.hpp"

#include "model.hpp"
#include "skybox.hpp"

using namespace glwrap;

class nanosuit_explode final : public example
{
public:
    nanosuit_explode()
        : explode_{program_.uniform("explodeRatio")},
          projection_{program_.uniform("projection")},
          model_view_{program_.uniform("modelView")},
          diffuse0_{program_.uniform("textureDiffuse0")}
    {
        program_.uniform("explodeDistance").set_float(2);
    }

    std::optional<camera> get_camera() override
    {
        return camera(glm::vec3(-5.0f, 15.0f, 10.0f), glm::vec3(0, 1, 0), -60, -10);
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        auto view = cam.view();

        skybox_.draw(projection, view);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        program_.use();

        auto time_ratio = std::fmod(timer::time(), 3.0f) / 3.0f;
        auto ratio = std::sqrt(time_ratio);

        explode_.set_float(ratio);
        projection_.set_mat4(projection);
        model_view_.set_mat4(view);
        for (auto &mesh : model_.meshes())
        {
            auto &varray = mesh.get_varray();
            varray.bind();
            if (mesh.has_texture(texture_type::diffuse))
            {
                auto &texture = mesh.get_texture(texture_type::diffuse);
                texture.bind_unit(0);
                diffuse0_.set_int(0);
            }
            varray.draw(draw_mode::triangles);
        }

        glDisable(GL_BLEND);
    }

private:
    glwrap::skybox skybox_{glwrap::cubemap{"resources/cubemaps/skybox", ".jpg"}};
    glwrap::model model_{model::load_file("resources/models/crysis_nano_suit_2/scene.gltf", texture_type::diffuse)};

    shader_program program_{
        shader::compile_file("shaders/geometry/explode_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/geometry/explode_fs.glsl", shader_type::fragment),
        shader::compile_file("shaders/geometry/explode_gs.glsl", shader_type::geometry),
    };

    shader_uniform explode_;
    shader_uniform projection_;
    shader_uniform model_view_;
    shader_uniform diffuse0_;
};

std::unique_ptr<example> create_nanosuit_explode()
{
    return std::make_unique<nanosuit_explode>();
}