#include <ranges>
#include <random>

#include "glwrap.hpp" 
#include "model.hpp"
#include "examples.hpp"
#include "utils.hpp"

using namespace glwrap;

class asteroids final : public example
{
public:

    asteroids(int amount, bool draw_instanced)
        : amount_(amount), draw_instanced_(draw_instanced)
        , planet_projection_{planet_program_.uniform("projection")}
        , planet_model_view_{planet_program_.uniform("modelView")}
        , planet_diffuse0_{planet_program_.uniform("textureDiffuse0")}
        , asteroid_projection_{asteroid_program_.uniform("projection")}
        , asteroid_model_view_{asteroid_program_.uniform("modelView")}
        , asteroid_diffuse0_{asteroid_program_.uniform("textureDiffuse0")}
        , asteroid_instanced_projection_{asteroid_instanced_program_.uniform("projection")}
        , asteroid_instanced_view_{asteroid_instanced_program_.uniform("view")}
        , asteroid_instanced_diffuse0_{asteroid_instanced_program_.uniform("textureDiffuse0")}
    {
        auto mt = std::mt19937(std::random_device()());
        auto rng = [&mt, d = std::uniform_real_distribution(0.0f, 1.0f)]() mutable { return d(mt); };
        auto poisson_rng = [&mt, d = std::poisson_distribution()]() mutable { return d(mt); };

        auto base_r = 20.0f;
        auto range_r = amount > 10000.0f ? 10.0f * std::log(amount / 10000.0f) : 10.0f;
        auto size_factor = std::log(amount / 10000.0f) + 1;
        auto base_scale = amount > 10000.0f ? 0.05f / (size_factor * size_factor) : 0.05f;
        std::cout << std::format("range: {}, scale: {}", range_r, base_scale) << std::endl;
        for (auto i = 0; i < amount; ++i)
        {
            auto mat = glm::mat4(1.0f);
            auto angle = static_cast<float>(i) / amount * 360.0f;
            auto r = base_r + rng() * range_r;
            auto x = r * std::sin(angle);
            auto z = r * std::cos(angle);
            auto y = rng() * 0.5f - 1.0f;

            mat = glm::translate(mat, glm::vec3(x, y, z));

            auto scale = base_scale * poisson_rng() * 2.5f;
            mat = glm::scale(mat, glm::vec3(scale));

            auto rot = rng() * 360.0f;
            mat = glm::rotate(mat, rot, glm::vec3(0.4f, 0.6f, 0.8f));

            mats_.push_back(mat);
        }

        attach_to_varray2(mats_);
    }

    void attach_to_varray2(std::vector<glm::mat4> const &mats)
    {
        auto models = vertex_buffer(mats);

        for (auto &m : asteroid_model_.meshes())
        {
            auto &varray = m.get_varray();
            auto binding_index = varray.attach_vbuffer(models);

            varray.enable_attrib(3);
            varray.attrib_format(3, binding_index, 4, GL_FLOAT, GL_FALSE, 0);

            varray.enable_attrib(4);
            varray.attrib_format(4, binding_index, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4));

            varray.enable_attrib(5);
            varray.attrib_format(5, binding_index, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4));

            varray.enable_attrib(6);
            varray.attrib_format(6, binding_index, 4, GL_FLOAT, GL_FALSE, 3 * sizeof(glm::vec4));

            varray.binding_divisor(binding_index, 1);
        }
    }

    void init() override
    {
        glClearColor(0, 0, 0, 0);
    }

    std::optional<camera> get_camera() override
    {
        return camera::look_at_camera(glm::vec3(0, 3.0f, 50.0f), glm::vec3(-5, 0, 0));
        //return camera(glm::vec3(0, 0, 50.0f), glm::vec3(0, 1, 0), -100);
    }

    void draw(glm::mat4 const &projection, glm::mat4 const &view) override
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        draw_planet(projection, view);
        draw_asteroids(projection, view);
    }

    void draw_planet(glm::mat4 const &projection, glm::mat4 const &view)
    {
        planet_program_.use();
        planet_projection_.set_mat4(projection);
        planet_model_view_.set_mat4(view);
        for (auto & m : planet_model_.meshes()) {
            m.get_texture(texture_type::diffuse).bind_unit(0);
            planet_diffuse0_.set_int(0);
            auto &varray = m.get_varray();
            varray.bind();
            varray.draw(draw_mode::triangles);
        }
    }

    void draw_asteroids(glm::mat4 const &projection, glm::mat4 const &view)
    {
        if (draw_instanced_) {
            asteroid_instanced_program_.use();
            asteroid_instanced_projection_.set_mat4(projection);
            asteroid_instanced_view_.set_mat4(view);
            for (auto &m : asteroid_model_.meshes())
            {
                m.get_texture(texture_type::diffuse).bind_unit(0);
                asteroid_diffuse0_.set_int(0);
                auto &varray = m.get_varray();
                varray.bind();
                varray.draw_instanced(draw_mode::triangles, amount_);
            }
        }
        else {
            asteroid_program_.use();
            asteroid_projection_.set_mat4(projection);
            for (auto i = 0; i < amount_; ++i) {
                auto &mat = mats_[i];
                asteroid_model_view_.set_mat4(view * mat);
                for (auto &m : asteroid_model_.meshes())
                {
                    m.get_texture(texture_type::diffuse).bind_unit(0);
                    asteroid_diffuse0_.set_int(0);
                    auto &varray = m.get_varray();
                    varray.bind();
                    varray.draw(draw_mode::triangles);
                }
            }
        }
    }

    int amount_;
    bool draw_instanced_;

    model planet_model_{model::load_file("resources/models/planet/planet.obj", texture_type::diffuse)};

    shader_program planet_program_{
        shader::compile_file("shaders/straight_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/straight_fs.glsl", shader_type::fragment),
    };

    shader_uniform planet_projection_;
    shader_uniform planet_model_view_;
    shader_uniform planet_diffuse0_;

    model asteroid_model_{model::load_file("resources/models/rock/rock.obj", texture_type::diffuse)};

    shader_program asteroid_program_{
        shader::compile_file("shaders/straight_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/straight_fs.glsl", shader_type::fragment),
    };

    shader_uniform asteroid_projection_;
    shader_uniform asteroid_model_view_;
    shader_uniform asteroid_diffuse0_;

    shader_program asteroid_instanced_program_{
        shader::compile_file("shaders/asteroid_vs.glsl", shader_type::vertex),
        shader::compile_file("shaders/straight_fs.glsl", shader_type::fragment),
    };

    shader_uniform asteroid_instanced_projection_;
    shader_uniform asteroid_instanced_view_;
    shader_uniform asteroid_instanced_diffuse0_;

    std::vector<glm::mat4> mats_;
};

std::unique_ptr<example> create_asteroids()
{
    return std::make_unique<asteroids>(1000, false);
}

std::unique_ptr<example> create_asteroids_instanced()
{
    return std::make_unique<asteroids>(100000, true);
}