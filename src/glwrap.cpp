#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include "glwrap.hpp"

using namespace glwrap;
using json = nlohmann::json;

static std::vector<glm::vec2> json_to_vec2(json const &j)
{
    auto size = j.size();
    size_t i = 0;
    std::vector<glm::vec2> v;
    while (i + 2 <= size)
    {
        v.emplace_back(j[i], j[i + 1]);
        i += 2;
    }
    return v;
}

static std::vector<glm::vec3> json_to_vec3(json const &j)
{
    auto size = j.size();
    size_t i = 0;
    std::vector<glm::vec3> v;
    while (i + 3 <= size)
    {
        v.emplace_back(j[i], j[i + 1], j[i + 2]);
        i += 3;
    }
    return v;
}

vertex_array glwrap::vertex_array::load_simple_json(std::filesystem::path const &path)
{
    std::ifstream f(path);
    auto j = json::parse(f,
                         /*parser_callback*/ nullptr,
                         /* allow_exceptions */ true,
                         /* ignore_comments */ true);

    vertex_array result;

    GLuint attrib_index = 0;
    GLuint buffer_index = 0;

    if (j.contains("position"))
    {
        result.attach_vbuffer(vertex_buffer<glm::vec3>(json_to_vec3(j["position"])));
        result.enable_attrib(attrib_index);
        result.attrib_format(attrib_index++, buffer_index++, 3, GL_FLOAT, GL_FALSE, 0);
    }
    if (j.contains("normal"))
    {
        result.attach_vbuffer(vertex_buffer<glm::vec3>(json_to_vec3(j["normal"])));
        result.enable_attrib(attrib_index);
        result.attrib_format(attrib_index++, buffer_index++, 3, GL_FLOAT, GL_TRUE, 0);
    }
    if (j.contains("texcoords"))
    {
        result.attach_vbuffer(vertex_buffer<glm::vec2>(json_to_vec2(j["texcoords"])));
        result.enable_attrib(attrib_index);
        result.attrib_format(attrib_index++, buffer_index++, 2, GL_FLOAT, GL_FALSE, 0);
    }
    if (j.contains("tangent"))
    {
        result.attach_vbuffer(vertex_buffer<glm::vec3>(json_to_vec3(j["tangent"])));
        result.enable_attrib(attrib_index);
        result.attrib_format(attrib_index++, buffer_index++, 3, GL_FLOAT, GL_TRUE, 0);
    }
    if (j.contains("bitangent"))
    {
        result.attach_vbuffer(vertex_buffer<glm::vec3>(json_to_vec3(j["bitangent"])));
        result.enable_attrib(attrib_index);
        result.attrib_format(attrib_index++, buffer_index++, 3, GL_FLOAT, GL_TRUE, 0);
    }
    if (j.contains("index"))
    {
        auto ids = j["index"].get<std::vector<GLuint>>();
        result.set_ibuffer(index_buffer<GLuint>(ids));
    }
    return result;
}