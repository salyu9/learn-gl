#include <cmath>

#include "examples.hpp"
#include "utils.hpp"

using namespace std::literals;
using namespace glwrap;

struct vert_t
{
    using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2>;
    glm::vec3 postion;
    glm::vec3 normal;
    glm::vec2 tex_coords;
};
class hdr_scene final : public example
{
public:
    hdr_scene(bool hdr)
        : hdr_(hdr)
    {
        hdr_light_program_.uniform("lights[0].Position").set_vec3(glm::vec3(0.0f, 0.0f, 49.5f));
        hdr_light_program_.uniform("lights[1].Position").set_vec3(glm::vec3(-1.4f, -1.9f, 9.0f));
        hdr_light_program_.uniform("lights[2].Position").set_vec3(glm::vec3(0.0f, -1.8f, 4.0f));
        hdr_light_program_.uniform("lights[3].Position").set_vec3(glm::vec3(0.8f, -1.7f, 6.0f));

        hdr_light_program_.uniform("lights[0].Color").set_vec3(glm::vec3(200.0f, 200.0f, 200.0f));
        hdr_light_program_.uniform("lights[1].Color").set_vec3(glm::vec3(0.1f, 0.0f, 0.0f));
        hdr_light_program_.uniform("lights[2].Color").set_vec3(glm::vec3(0.0f, 0.0f, 0.2f));
        hdr_light_program_.uniform("lights[3].Color").set_vec3(glm::vec3(0.0f, 0.1f, 0.0f));

        hdr_light_program_.uniform("diffuseTexture").set_int(0);

        hdr_light_program_.uniform("inverse_normals").set_bool(true);
    }

    std::optional<camera> get_camera() override
    {
        return camera{glm::vec3(1.12f, 0.50f, -2.52f), glm::vec3(0, 1, 0), 100.0f, -7.7f};
    }

    void draw(glm::mat4 const& projection, camera & cam) override
    {
        if (hdr_)
        {
            auto period = 3.0f;
            auto t = std::abs((std::fmod(timer::time(), (2 * period)) / period - 1));
            exposure_.set_float(t * 5);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hdr_light_program_.use();

        projection_.set_mat4(projection);
        view_.set_mat4(cam.view());

        auto model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 25.0));
        model = glm::scale(model, glm::vec3(2.5f, 2.5f, 27.5f));
        model_.set_mat4(model);

        view_pos_.set_vec3(cam.position());

        diffuse_.bind_unit(0);

        varray_.bind();
        varray_.draw(draw_mode::triangles);
    }

    shader_program * postprocessor_ptr() override
    {
        return hdr_ ? &postprocess_ : nullptr;
    }

    bool is_hdr() override { return true; }

    bool hdr_;

    vertex_array varray_{vertex_array::load_simple_json("resources/simple_vertices/hdr_scene.jsonc")};

    texture2d diffuse_{"resources/textures/wood.png"sv, true};

    shader_program hdr_light_program_{
        shader::compile_file("shaders/hdr_light_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/hdr_light_fs.glsl", shader_type::fragment),
    };

    shader_uniform projection_{hdr_light_program_.uniform("projection")};
    shader_uniform view_{hdr_light_program_.uniform("view")};
    shader_uniform model_{hdr_light_program_.uniform("model")};
    shader_uniform view_pos_{hdr_light_program_.uniform("viewPos")};

    shader_program postprocess_{
        shader::compile_file("shaders/base/fbuffer_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/hdr_exposure_fs.glsl", shader_type::fragment),
    };
    shader_uniform exposure_{postprocess_.uniform("exposure")};
};

std::unique_ptr<example> create_ldr()
{
    return std::make_unique<hdr_scene>(false);
}

std::unique_ptr<example> create_hdr()
{
    return std::make_unique<hdr_scene>(true);
}
