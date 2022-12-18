#include "examples.hpp"
#include "glwrap.hpp"
#include "model.hpp"
#include "common_obj.hpp"

using namespace glwrap;

struct vert_t
{
    using vertex_desc_t = std::tuple<glm::vec3, glm::vec3, glm::vec2, glm::vec3, glm::vec3>;
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec2 tex;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

class parallax_map final : public example
{
public:
    parallax_map()
    {
        light_position_.set_vec3(box_.get_position());

        glEnable(GL_CULL_FACE);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(10, 0, 10));
    }

    void draw(glm::mat4 const &projection, glm::mat4 const &view) override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        rotation_ += timer::delta_time() * 1;
        auto model = glm::rotate(glm::mat4(1), rotation_, glm::vec3(0, 1, 0));
        auto normal_mat = glm::inverse(glm::transpose(model));

        program_.use();
        projection_.set_mat4(projection);
        view_mat_.set_mat4(view);
        model_mat_.set_mat4(model);
        normal_mat_.set_mat4(normal_mat);
        view_position_.set_vec3(camera::active->position());
        diffuse_sampler_.set_int(0);
        normal_sampler_.set_int(1);
        depth_sampler_.set_int(2);

        diffuse_map1_.bind_unit(0);
        normal_map1_.bind_unit(1);
        depth_map1_.bind_unit(2);
        varray1_.bind();
        varray1_.draw(draw_mode::triangles);

        diffuse_map2_.bind_unit(0);
        normal_map2_.bind_unit(1);
        depth_map2_.bind_unit(2);
        varray2_.bind();
        varray2_.draw(draw_mode::triangles);

        box_.draw(projection, view);
    }

    shader_program program_{
        shader::compile_file("shaders/parallax_map_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/parallax_map_fs.glsl", shader_type::fragment),
    };
    shader_uniform projection_{program_.uniform("projection")};
    shader_uniform model_mat_{program_.uniform("model")};
    shader_uniform view_mat_{program_.uniform("view")};
    shader_uniform normal_mat_{program_.uniform("normalMat")};
    shader_uniform diffuse_sampler_{program_.uniform("diffuseTexture")};
    shader_uniform normal_sampler_{program_.uniform("normalTexture")};
    shader_uniform depth_sampler_{program_.uniform("depthTexture")};
    shader_uniform light_position_{program_.uniform("lightPosition")};
    shader_uniform view_position_{program_.uniform("viewPosition")};

    texture2d diffuse_map1_{"resources/textures/bricks2.jpg"};
    texture2d normal_map1_{"resources/textures/bricks2_normal.jpg"};
    texture2d depth_map1_{"resources/textures/bricks2_disp.jpg"};

    texture2d diffuse_map2_{"resources/textures/wooden_toy.png"};
    texture2d normal_map2_{"resources/textures/wooden_toy_normal.png"};
    texture2d depth_map2_{"resources/textures/wooden_toy_disp.png"};

    vertex_buffer<vert_t> vbuffer1_{
        {glm::vec3(0, -5, +5), glm::vec3(+1, 0, 0), glm::vec2(0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, -5, -5), glm::vec3(+1, 0, 0), glm::vec2(1, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, +5, -5), glm::vec3(+1, 0, 0), glm::vec2(1, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, +5, +5), glm::vec3(+1, 0, 0), glm::vec2(0, 1), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)},
    };
    vertex_buffer<vert_t> vbuffer2_{
        {glm::vec3(0, -5, -5), glm::vec3(-1, 0, 0), glm::vec2(0, 0), glm::vec3(0, 0, +1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, -5, +5), glm::vec3(-1, 0, 0), glm::vec2(1, 0), glm::vec3(0, 0, +1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, +5, +5), glm::vec3(-1, 0, 0), glm::vec2(1, 1), glm::vec3(0, 0, +1), glm::vec3(0, 1, 0)},
        {glm::vec3(0, +5, -5), glm::vec3(-1, 0, 0), glm::vec2(0, 1), glm::vec3(0, 0, +1), glm::vec3(0, 1, 0)},
    };

    index_buffer<GLuint> ibuffer_{0, 1, 2, 2, 3, 0};

    vertex_array varray1_{auto_vertex_array(ibuffer_, vbuffer1_)};
    vertex_array varray2_{auto_vertex_array(ibuffer_, vbuffer2_)};

    box box_{glm::vec3(2, 3, -5), glm::vec3(0.2f, 0.2f, 0.2f)};

    float rotation_ = 0;
};

std::unique_ptr<example> create_parallax_map()
{
    return std::make_unique<parallax_map>();
}
