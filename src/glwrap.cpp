#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>
#include "glwrap.hpp"
#include "utils.hpp"

using namespace glwrap;
using json = nlohmann::json;

// --------------------- texture -------------------------------

GLenum to_image_format(texture2d_format format)
{
    switch (format)
    {
    case texture2d_format::rgb:
        return GL_RGB;
    case texture2d_format::srgb:
        return GL_RGB;
    case texture2d_format::rgba:
        return GL_RGBA;
    case texture2d_format::srgba:
        return GL_RGBA;
    case texture2d_format::grey:
        return GL_RED;
    default:
        throw std::invalid_argument(std::format("Cannot create internal format for {}", format));
    }
}

GLenum to_internal_format(texture2d_format format, texture2d_elem_type elem_type)
{
    static std::map<std::tuple<texture2d_format, texture2d_elem_type>, GLenum> map_{
        {{texture2d_format::rgb, texture2d_elem_type::u8}, GL_RGB8},
        {{texture2d_format::rgb, texture2d_elem_type::f16}, GL_RGB16F},
        {{texture2d_format::rgb, texture2d_elem_type::f32}, GL_RGB32F},

        {{texture2d_format::srgb, texture2d_elem_type::u8}, GL_SRGB8},

        {{texture2d_format::rgba, texture2d_elem_type::u8}, GL_RGBA8},
        {{texture2d_format::rgba, texture2d_elem_type::f16}, GL_RGBA16F},
        {{texture2d_format::rgba, texture2d_elem_type::f32}, GL_RGBA32F},

        {{texture2d_format::srgba, texture2d_elem_type::u8}, GL_SRGB8_ALPHA8},

        {{texture2d_format::grey, texture2d_elem_type::u8}, GL_R8},
        {{texture2d_format::grey, texture2d_elem_type::f16}, GL_R16F},
        {{texture2d_format::grey, texture2d_elem_type::f32}, GL_R32F},
    };

    auto iter = map_.find({format, elem_type});
    if (iter == map_.end())
    {
        throw std::invalid_argument(std::format("Cannot create internal format for {}, {}", format, elem_type));
    }
    return iter->second;
}

void create_texture_resources(GLuint & handle, bool srgb, texture2d_format format, texture2d_elem_type elem_type, bitmap &bmp, GLsizei &width, GLsizei &height)
{
    if (format == texture2d_format::unspecified)
    {
        if (bmp.channels() == bitmap_channel::rgb)
            format = srgb ? texture2d_format::srgb : texture2d_format::rgb;
        else if (bmp.channels() == bitmap_channel::rgba)
            format = srgb ? texture2d_format::srgba : texture2d_format::rgba;
        else if (bmp.channels() == bitmap_channel::grey)
            format = texture2d_format::grey;
        else
            throw std::invalid_argument(std::format("bitmap from memory has {} channels, unknown format.", bmp.channels()));
    }
    else
    {
        if ((format == texture2d_format::rgb || format == texture2d_format::srgb) && bmp.channels() != bitmap_channel::rgb)
        {
            throw std::invalid_argument(std::format("bitmap from memory has {} channels, 3 needed.", bmp.channels()));
        }
        if ((format == texture2d_format::rgba || format == texture2d_format::srgba) && bmp.channels() != bitmap_channel::rgba)
        {
            throw std::invalid_argument(std::format("bitmap from memory has {} channels, 4 needed.", bmp.channels()));
        }
        if (format == texture2d_format::grey && bmp.channels() != bitmap_channel::grey)
        {
            throw std::invalid_argument(std::format("bitmap from memory has {} channels, 1 needed.", bmp.channels()));
        }
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTextureStorage2D(handle, bmp.max_mipmap_level(), to_internal_format(format, elem_type), bmp.width(), bmp.height());
    glTextureSubImage2D(handle, 0, 0, 0, bmp.width(), bmp.height(), to_image_format(format), get_bitmap_texture_type(bmp), bmp.pixels());
    glGenerateTextureMipmap(handle);
    width = bmp.width();
    height = bmp.height();
}

texture2d::texture2d(GLsizei width, GLsizei height, GLsizei multisamples, GLenum internal_format)
    : width_{width}, height_{height}
{
    if (multisamples == 0)
    {
        glCreateTextures(GL_TEXTURE_2D, 1, &handle_);
        glTextureStorage2D(handle_, 1, internal_format, width, height);
        glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }
    else
    {
        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &handle_);
        glTextureStorage2DMultisample(handle_, multisamples, internal_format, width, height, GL_TRUE);
    }
    auto err = glGetError();
    if (err != GL_NO_ERROR)
    {
        throw gl_error(std::format("Create texture2d failed: {}", err));
    }
}

texture2d::texture2d(GLsizei width, GLsizei height, GLsizei multisamples, texture2d_format format, texture2d_elem_type elem_type)
    : texture2d(width, height, multisamples, to_internal_format(format, elem_type))
{
}

texture2d::texture2d(std::byte const *p, size_t size, bool srgb, texture2d_elem_type elem_type, texture2d_format format)
{
    auto required_channel = bitmap_channel::unspecified;
    if (format == texture2d_format::rgb)
        required_channel = bitmap_channel::rgb;
    else if (format == texture2d_format::rgba)
        required_channel = bitmap_channel::rgba;
    auto bmp = bitmap::from_memory(p, size, required_channel, true);
    create_texture_resources(handle_, srgb, format, elem_type, bmp, width_, height_);
}

texture2d::texture2d(std::filesystem::path const &filename, bool srgb, texture2d_elem_type elem_type, texture2d_format format)
{
    auto required_channel = bitmap_channel::unspecified;
    if (format == texture2d_format::rgb)
        required_channel = bitmap_channel::rgb;
    else if (format == texture2d_format::rgba)
        required_channel = bitmap_channel::rgba;
    auto bmp = bitmap::from_file(filename, required_channel, true);
    create_texture_resources(handle_, srgb, format, elem_type, bmp, width_, height_);
}

// ----------------------- cubemap ------------------------------------

cubemap::cubemap(std::filesystem::path const &right,
                 std::filesystem::path const &left,
                 std::filesystem::path const &top,
                 std::filesystem::path const &bottom,
                 std::filesystem::path const &back,
                 std::filesystem::path const &front)
{
    std::tuple<bitmap, GLenum, GLsizei> faces[] = {
        {bitmap::from_file(right, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0},
        {bitmap::from_file(left, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 1},
        {bitmap::from_file(top, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 2},
        {bitmap::from_file(bottom, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 3},
        {bitmap::from_file(back, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 4},
        {bitmap::from_file(front, bitmap_channel::rgb), GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 5},
    };
    auto &first_bmp = std::get<bitmap>(faces[0]);
    GLsizei width = first_bmp.width(), height = first_bmp.height(), levels = first_bmp.max_mipmap_level();
    auto internal_format = first_bmp.internal_format();
    for (auto &[bmp, _1, _2] : faces)
    {
        if (bmp.width() != width || bmp.height() != height)
            throw gl_error("Cubemap texture size must be same");
    }

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle_);
    glTextureStorage2D(handle_, 1, GL_SRGB8, width, height);
    for (auto &[bmp, _, i] : faces)
    {
        glTextureSubImage3D(handle_, 0, 0, 0, i, width, height, 1, GL_RGB, get_bitmap_texture_type(bmp), bmp.pixels());
    }
    glGenerateTextureMipmap(handle_);
    glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    auto err = glGetError();
    if (err != GL_NO_ERROR)
    {
        throw gl_error(std::format("Create cube map failed, gl error = {}", err));
    }
}

cubemap::cubemap(GLsizei size)
{
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle_);
    glTextureStorage2D(handle_, 1, GL_RGBA32F, size, size);
    glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(handle_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    auto err = glGetError();
    if (err != GL_NO_ERROR)
    {
        throw gl_error(std::format("Create cube map failed, gl error = {}", err));
    }
}

cubemap::cubemap() = default;

cubemap cubemap::from_single_texture(texture2d & texture, GLsizei size)
{
    static auto prog = shader_program{
        shader::compile_file("shaders/compute/cubemap_mapping.glsl", shader_type::compute),
    };

    texture.bind_unit(0);
    cubemap c{size};
    c.bind_write_image(1);

    prog.use();

    glDispatchCompute(size, size, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    return c;
}

// -------------------- vertex array --------------------------

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

vertex_array vertex_array::load_simple_json(std::filesystem::path const &path)
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

// ------------------------ frame buffer ---------------------------------

struct frame_buffer::frame_buffer_impl final
{
    frame_buffer_impl(std::vector<texture2d> &&textures, GLsizei width, GLsizei height, GLsizei multisamples)
        : width_(width), height_(height), multisamples_(multisamples), color_textures_(std::move(textures)),
          depth_texture_(width, height, multisamples, GL_DEPTH_COMPONENT32F)
    {
        glCreateFramebuffers(1, &handle_);

        for (size_t i = 0; i < color_textures_.size(); ++i)
        {
            glNamedFramebufferTexture(handle_, GL_COLOR_ATTACHMENT0 + i, color_textures_[i].handle(), 0);
        }

        glNamedFramebufferTexture(handle_, GL_DEPTH_ATTACHMENT, depth_texture_.handle(), 0);

        auto status = glCheckNamedFramebufferStatus(handle_, GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            throw gl_error(std::format("Frame buffer uncomplete, status = {:x}", status));
        }
    }

    ~frame_buffer_impl()
    {
        glDeleteFramebuffers(1, &handle_);
    }

    GLsizei width_{0}, height_{0};
    GLsizei multisamples_{0};
    GLuint handle_{0};
    std::vector<texture2d> color_textures_;
    texture2d depth_texture_;
};

std::vector<texture2d> create_frame_buffer_textures(size_t target_count, GLsizei width, GLsizei height, GLsizei multisamples, bool is_hdr)
{
    std::vector<texture2d> result;
    for (size_t i = 0; i < target_count; ++i)
    {
        result.emplace_back(width, height, multisamples, texture2d_format::rgba, is_hdr ? texture2d_elem_type::f16 : texture2d_elem_type::u8);
    }
    return result;
}

frame_buffer::frame_buffer(std::vector<texture2d> &&textures, GLsizei width, GLsizei height, GLsizei multisamples)
    : impl_{std::make_unique<frame_buffer_impl>(std::move(textures), width, height, multisamples)}
{ }

frame_buffer::frame_buffer(GLsizei width, GLsizei height, GLsizei multisamples, bool is_hdr)
    : frame_buffer(1, width, height, multisamples, is_hdr)
{ }

frame_buffer::frame_buffer(size_t target_count, GLsizei width, GLsizei height, GLsizei multisamples, bool is_hdr)
    : frame_buffer(create_frame_buffer_textures(target_count, width, height, multisamples, is_hdr), width, height, multisamples)
{ }

frame_buffer::frame_buffer(frame_buffer &&other) noexcept { swap(other); }

frame_buffer &frame_buffer::operator = (frame_buffer && other) noexcept
{
    swap(other);
    return *this;
}

frame_buffer::~frame_buffer() = default;

void frame_buffer::swap(frame_buffer &other) noexcept
{
    std::swap(impl_, other.impl_);
}

void frame_buffer::bind()
{
    glBindFramebuffer(GL_FRAMEBUFFER, impl_->handle_);
}

void frame_buffer::unbind_all()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void frame_buffer::clear(glm::vec4 color, float depth, size_t draw_buffer)
{
    clear_color(color, draw_buffer);
    clear_depth(depth);
}

void frame_buffer::clear_color(glm::vec4 color, size_t draw_buffer)
{
    glClearNamedFramebufferfv(impl_->handle_, GL_COLOR, draw_buffer, value_ptr(color));
}

void frame_buffer::clear_depth(float depth)
{
    glClearNamedFramebufferfv(impl_->handle_, GL_DEPTH, 0, &depth);
}

texture2d &frame_buffer::color_texture()
{
    return impl_->color_textures_.at(0);
}

texture2d &frame_buffer::color_texture_at(size_t index)
{
    return impl_->color_textures_.at(index);
}

texture2d &frame_buffer::depth_texture()
{
    return impl_->depth_texture_;
}

void frame_buffer::draw_buffers(std::span<size_t> indexes)
{
    std::vector<GLenum> dst{};
    for (auto i : indexes)
    {
        dst.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(impl_->handle_, dst.size(), dst.data());
}

void frame_buffer::draw_buffers(std::initializer_list<size_t> indexes)
{
    std::vector<GLenum> dst{};
    for (auto i : indexes)
    {
        dst.push_back(GL_COLOR_ATTACHMENT0 + i);
    }
    glNamedFramebufferDrawBuffers(impl_->handle_, dst.size(), dst.data());
}

GLuint frame_buffer::handle()
{
    return impl_->handle_;
}

GLsizei frame_buffer::width() const noexcept { return impl_->width_; }
GLsizei frame_buffer::height() const noexcept { return impl_->height_; }

void frame_buffer::blit_to(frame_buffer &other, GLbitfield mask, GLenum filter)
{
    glBlitNamedFramebuffer(impl_->handle_, other.impl_->handle_, 0, 0, impl_->width_, impl_->height_, 0, 0, other.impl_->width_, other.impl_->height_, mask, filter);
}