#include <format>

#include "examples.hpp"
#include "glwrap.hpp"
#include "model.hpp"
#include "common_obj.hpp"

using namespace glwrap;
using namespace std::literals;

struct vert_t
{
    using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2, glm::vec3, glm::vec3>;
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

class normal_map final : public example
{
public:

    normal_map()
    {
        glEnable(GL_CULL_FACE);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(10, 0, 10));
    }

    void draw(glm::mat4 const &projection, camera &cam) override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto rotation = timer::time() * 1;
        
        auto view = cam.view();

        program_.use();
        projection_.set_mat4(projection);
        view_mat_.set_mat4(view);
        diffuse_map_.bind_unit(0);
        normal_map_.bind_unit(1);
        diffuse_sampler_.set_int(0);
        normal_sampler_.set_int(1);
        view_position_.set_vec3(cam.position());
        varray_.bind();

        auto model = glm::rotate(glm::mat4(1), rotation, glm::vec3(0, 1, 0));
        auto normal_mat = glm::inverse(glm::transpose(model));
        model_mat_.set_mat4(model);
        normal_mat_.set_mat4(normal_mat);
        varray_.draw(draw_mode::triangles);

        model = glm::rotate(glm::mat4(1), rotation + glm::radians(180.0f), glm::vec3(0, 1, 0));
        normal_mat = glm::inverse(glm::transpose(model));
        model_mat_.set_mat4(model);
        normal_mat_.set_mat4(normal_mat);
        varray_.draw(draw_mode::triangles);

        light_box_.draw(projection, view);
    }

private:

    box light_box_{glm::vec3(2, 3, -5), glm::vec3(0.2f, 0.2f, 0.2f)};

    shader_program program_{make_vf_program(
        "shaders/normal_map_vs.glsl"_path,
        "shaders/normal_map_fs.glsl"_path,
        "lightPosition", light_box_.get_position()
    )};
    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform model_mat_{program_.uniform("model")};
    shader_uniform view_mat_{program_.uniform("view")};
    shader_uniform normal_mat_{program_.uniform("normalMat")};
    shader_uniform diffuse_sampler_{program_.uniform("diffuseTexture")};
    shader_uniform normal_sampler_{program_.uniform("normalTexture")};
    shader_uniform view_position_{program_.uniform("viewPosition")};

    texture2d diffuse_map_{"resources/textures/brickwall.jpg"sv, true};
    texture2d normal_map_{"resources/textures/brickwall_normal.jpg"sv};

    vertex_array varray_{auto_vertex_array(
        index_buffer<GLuint>{0, 1, 2, 2, 3, 0},
        vertex_buffer<vert_t>{
            {glm::vec3(0, -5, +5), glm::vec3(1, 0, 0), glm::vec2(0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
            {glm::vec3(0, -5, -5), glm::vec3(1, 0, 0), glm::vec2(1, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
            {glm::vec3(0, +5, -5), glm::vec3(1, 0, 0), glm::vec2(1, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
            {glm::vec3(0, +5, +5), glm::vec3(1, 0, 0), glm::vec2(0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
        }
    )};
};

std::unique_ptr<example> create_normal_map()
{
    return std::make_unique<normal_map>();
}
