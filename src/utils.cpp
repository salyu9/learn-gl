#include "glwrap.hpp"
#include "utils.hpp"

namespace utils
{
    glwrap::vertex_array get_quad_varray()
    {
        return glwrap::auto_vertex_array(glwrap::vertex_buffer<quad_vertex_t>{
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, 0.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f},
            {1.0f, -1.0f, 1.0f, 0.0f},
            {1.0f, 1.0f, 1.0f, 1.0f},
        });
    }

    glwrap::vertex_array create_uv_sphere(int slices, int stacks, bool full_information)
    {
        using namespace glwrap;
        if (slices < 4 || stacks < 3)
        {
            throw std::invalid_argument(std::format("Too less slices or stacks: {}, {}", slices, stacks));
        }
        constexpr auto pi = std::numbers::pi_v<float>;

        std::vector<glm::vec3> vertices{};
        std::vector<glm::vec2> texcoords{};
        std::vector<glm::vec4> tangents{};
        std::vector<GLuint> indices{};

        vertices.emplace_back(0, 1, 0);
        if (full_information)
        {
            texcoords.emplace_back(0, 1);
            tangents.emplace_back(1, 0, 0, 1);
        }

        for (int i = 0; i < stacks - 1; ++i)
        {
            auto theta = pi * (i + 1) / stacks;
            for (int j = 0; j < slices; ++j)
            {
                auto phi = -2.0f * pi * j / slices;
                auto x = std::sin(theta) * std::cos(phi);
                auto y = std::cos(theta);
                auto z = std::sin(theta) * std::sin(phi);
                vertices.emplace_back(x, y, z);
                if (full_information)
                {
                    texcoords.emplace_back(static_cast<float>(j) / slices, 1 - static_cast<float>(i + 1) / stacks);
                    tangents.emplace_back(std::sin(phi), 0, -std::cos(phi), 1);
                }
            }
        }

        vertices.emplace_back(0, -1, 0);
        if (full_information)
        {
            texcoords.emplace_back(0, 0);
            tangents.emplace_back(-1, 0, 0, 1);
        }

        for (int j = 0; j < slices; ++j)
        {
            indices.push_back(0);
            indices.push_back(1 + j);
            indices.push_back(1 + (j + 1) % slices);
        }

        for (int i = 0; i < stacks - 2; ++i)
        {
            auto base0 = i * slices + 1;
            auto base1 = (i + 1) * slices + 1;
            for (int j = 0; j < slices; ++j)
            {
                indices.push_back(base0 + j);
                indices.push_back(base1 + j);
                indices.push_back(base1 + (j + 1) % slices);
                indices.push_back(base1 + (j + 1) % slices);
                indices.push_back(base0 + (j + 1) % slices);
                indices.push_back(base0 + j);
            }
        }

        auto sec_last = (stacks - 2) * slices + 1;
        auto last = (stacks - 1) * slices + 1;
        for (int j = 0; j < slices; ++j)
        {
            indices.push_back(sec_last + j);
            indices.push_back(last);
            indices.push_back(sec_last + (j + 1) % slices);
        }

        return full_information
                   ? auto_vertex_array(index_buffer<GLuint>(indices),
                                       vertex_buffer<glm::vec3>(vertices), // position
                                       vertex_buffer<glm::vec3>(vertices), // normal
                                       vertex_buffer<glm::vec2>(texcoords),
                                       vertex_buffer<glm::vec4>(tangents) // gltf2-style tangent
                                       )
                   : auto_vertex_array(index_buffer<GLuint>(indices), vertex_buffer<glm::vec3>(vertices));
    }

    glm::vec3 hsv(int h, float s, float v)
    {
        if (h > 360 || h < 0 || s > 100 || s < 0 || v > 100 || v < 0)
        {
            throw std::invalid_argument(std::format("Invalid HSV: {}, {}, {}", h, s, v));
        }
        float c = s * v;
        float x = c * (1 - std::abs(std::fmod(h / 60.0f, 2) - 1));
        float m = v - c;
        float r, g, b;
        if (h >= 0 && h < 60)
        {
            r = c, g = x, b = 0;
        }
        else if (h >= 60 && h < 120)
        {
            r = x, g = c, b = 0;
        }
        else if (h >= 120 && h < 180)
        {
            r = 0, g = c, b = x;
        }
        else if (h >= 180 && h < 240)
        {
            r = 0, g = x, b = c;
        }
        else if (h >= 240 && h < 300)
        {
            r = x, g = 0, b = c;
        }
        else
        {
            r = c, g = 0, b = x;
        }
        return {r + m, g + m, b + m};
    }
}