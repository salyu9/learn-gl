#pragma once

/*
    Require OpenGL 4.5 or higher (for DSA).
    Require glm
    Require bitmap class with definition:
        class bitmap
        {
        public:
            int width() const noexcept { return width_; }
            int height() const noexcept { return height_; }
            int channels() const noexcept { return channels_; }
            int max_mipmap_level() const noexcept { return max_mipmap_level_; }
            std::vector<std::uint8_t> const& pixels() const noexcept { return pixels_; }

            static bitmap from_file(const char * filename, bool flip_vertically = false);
            static bitmap from_file(std::string const& filename, bool flip_vertically = false) { return from_file(filename.c_str(), flip_vertically); }
        };


    * Create buffers
    Use: vertex_buffer<Vertex> and index_buffer<Index>
    Vertex can be any type, Index must be integral type.
    VBO will be deleted when lifetime ends.
    Examples:
    - auto vbuffer = vertex_buffer(box_vertices);
    - auto ibuffer = index_buffer(indices);

    * Create vertex array
    Use:   vertex_array(index_buffer<Index>&, vertex_buffer<Vertex>&...)
        or vertex_array(vertex_buffer<Vertex>&...)
    If index_buffer<Index> is set then vbo.draw() will call glDrawElements, otherwise glDrawArrays.
    Vertex buffers indices will be in sequence (0, 1, 2, ...).
    vertex_array will not keep any of the buffers. Please make sure that all the buffers are alive when using vertex_array.
    Examples:
    - auto varray = vertex_array(ibuffer, vbuffer);

    * Set vertex array attrib format:
    Use: vbo.enable_attrib(index) and vbo.attrib_format(attrib_index, vbo_index, size, type, normalizing, offset)
    Just like using glVertexArrayAttribFormat
    Examples:
    - varray.enable_attrib(0);
    - varray.attrib_format(0, 0, decltype(vertex_t::pos)::length(), GL_FLOAT, GL_FALSE, offsetof(vertex_t, pos));
    - varray.enable_attrib(1);
    - varray.attrib_format(1, 0, decltype(vertex_t::tex)::length(), GL_FLOAT, GL_FALSE, offsetof(vertex_t, tex));

    * Auto set vertex array attrib format:
    Use:   auto_vertex_array(index_buffer<Index>&, vertex_buffer<Vertex>&...)
        or auto_vertex_array(vertex_buffer<Vertex>&...)
    Vertex type must be one of:
        1. arithmetic type
        2. glm::vec<> type
        3. std::tuple<> type, with elements of Vertex types
        4. type that defines type vertex_desc_t
    Each element will leads to a attrib_format invocation in sequence.
    Examples:
    - Vertex type: int
            attrib_format(attrib_index: 0, vbo_index: 0, size: 1, type: GL_INT, normalizing: GL_FALSE, offset: 0)

    - Vertex type: glm::vec3 (glm::vec<3, float, ...>)
            attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)

    - Vertex type: struct vertex_t {
                       using vertex_desc_t = std::tuple<glm::vec3, glm::vec2>;
                       glm::vec3 position;
                       glm::vec2 tex_coord;
                   };
            attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)
            attrib_format(attrib_index: 1, vbo_index: 0, size: 2, type: GL_FLOAT, normalizing: GL_FALSE, offset: 3 * sizeof(float))
            ** Cannot use std::tuple<glm::vec3, glm::vec2> as vertex type directly, since std::tuple is not standard-layout.

    - Vertex type: glm::vec3 (buffer 0), glm::vec2 (buffer 1)
            attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)
            attrib_format(attrib_index: 1, vbo_index: 1, size: 2, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)

    * Shaders and program
    Use:  shader::compile(code, type) or shader::compile_file(filename, type)
          shader_program(shaders...)
    If shader_program get a rvalue referenced shader, it will keep that shader alive.
    For other shaders passed by lvalue reference, please keep them alive when using shader_program.
    Examples:
    - auto fs = shader::compile_file("my_fs.glsl", shader_type::fragment);
      auto program = shader_program(shader::compile("#version 330 core\n uniform mat4 transform; ...", shader_type::vertex), fs);
      auto trams_uniform = program.uniform("transform");
      trans_uniform.set_mat4(...);

    * Render
    program.use();
    vertarray.bind();
    vertarray.draw(draw_mode::triangles);

 */

#include <utility>
#include <string_view>
#include <string>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <array>
#include <tuple>
#include <optional>
#include <format>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bitmap.hpp"
#include "utils.hpp"

#define GLWRAP_ITER_ARITH_TYPES(X)    \
    X(std::int32_t, GL_INT)           \
    X(std::int16_t, GL_SHORT)         \
    X(std::int8_t, GL_BYTE)           \
    X(std::uint32_t, GL_UNSIGNED_INT) \
    X(std::uint16_t, GL_UNSIGNED_INT) \
    X(std::uint8_t, GL_UNSIGNED_INT)  \
    X(float, GL_FLOAT)                \
    X(double, GL_DOUBLE)

namespace glwrap
{
    using std::int8_t, std::int16_t, std::int32_t, std::int64_t,
        std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t;

    namespace details
    {
        template <typename T>
        struct gl_type_traits final
        {
        };

#define GLWRAP_GL_TYPE_TRAITS(ori_type, gl_type)                      \
    template <>                                                       \
    struct gl_type_traits<ori_type>                                   \
    {                                                                 \
        constexpr inline const static size_t size = sizeof(ori_type); \
        constexpr inline const static GLenum type = gl_type;          \
    };

        GLWRAP_ITER_ARITH_TYPES(GLWRAP_GL_TYPE_TRAITS)
        /*
        template <>
        struct gl_type_traits<std::int32_t> final : gl_type_traits_base<std::int32_t>
        {
            constexpr inline const static GLenum type = GL_INT;
        };
        */

    }

    class gl_error final : public std::runtime_error
    {
    public:
        explicit gl_error(std::string const &error_msg) : std::runtime_error(error_msg) {}
    };

    //----------- enums ---------------------
    enum class draw_mode : GLenum
    {
        points = GL_POINTS,
        line_strip = GL_LINE_STRIP,
        line_loop = GL_LINE_LOOP,
        lines = GL_LINES,
        line_strip_adjacency = GL_LINE_STRIP_ADJACENCY,
        lines_adjacency = GL_LINES_ADJACENCY,
        triangle_strip = GL_TRIANGLE_STRIP,
        triangle_fan = GL_TRIANGLE_FAN,
        triangles = GL_TRIANGLES,
        triangle_strip_adjacency = GL_TRIANGLE_STRIP_ADJACENCY,
        triangles_adjacency = GL_TRIANGLES_ADJACENCY,
        patches = GL_PATCHES,
    };

    //----------- buffers -------------------

    template <typename T>
    class buffer abstract
    {
    public:
        buffer(T const *data, size_t length)
        {
            if (length > std::numeric_limits<GLsizei>::max())
                throw std::invalid_argument(std::string("buffer data too large (") + std::to_string(length) + ")");
            count_ = static_cast<GLsizei>(length);
            ::glCreateBuffers(1, &this->handle_);
            ::glNamedBufferStorage(this->handle_, sizeof(T) * length, data, GL_DYNAMIC_STORAGE_BIT);
        }

        buffer(std::initializer_list<T> data) : buffer(data.begin(), data.size()) {}

        template <size_t N>
        explicit buffer(T const (&data)[N]) : buffer(data, N) {}

        template <typename Allocator>
        explicit buffer(std::vector<T, Allocator> const &data) : buffer(data.data(), data.size()) {}

        buffer(buffer const &) = delete;
        buffer(buffer &&other) noexcept
        {
            this->swap(other);
        }
        buffer &operator=(buffer const &other) = delete;
        buffer &operator=(buffer &&other) noexcept
        {
            this->swap(other);
            return *this;
        }

        void swap(buffer &other) noexcept
        {
            std::swap(handle_, other.handle_);
        }

        ~buffer()
        {
            if (handle_ != 0)
            {
                ::glDeleteBuffers(1, &handle_);
            }
        }

        GLuint handle() const noexcept { return handle_; }
        GLsizei size() const noexcept { return count_; }

    protected:
        buffer() = default;

    private:
        GLuint handle_{};
        GLsizei count_{};
    };

    template <typename Vertex>
    class vertex_buffer final : public buffer<Vertex>
    {
        static_assert(std::is_standard_layout_v<Vertex>, "User-defined vertex type must be standard layout");

    public:
        using vertex_type = Vertex;
        using base = buffer<Vertex>;

        vertex_buffer(Vertex const *data, size_t length) : base(data, length) {}
        vertex_buffer(std::initializer_list<Vertex> data) : base(data) {}
        template <size_t N>
        explicit vertex_buffer(Vertex const (&data)[N]) : base(data) {}
        template <typename Allocator>
        explicit vertex_buffer(std::vector<Vertex, Allocator> const &data) : base(data) {}

    private:
        vertex_buffer() = default;
    };

    template <typename Index>
    class index_buffer final : public buffer<Index>
    {
        static_assert(std::is_integral_v<Index>, "index must be integral type");

    public:
        using index_type = Index;
        using base = buffer<Index>;

        index_buffer(Index const *data, size_t length) : base(data, length) {}
        index_buffer(std::initializer_list<Index> data) : base(data) {}
        template <size_t N>
        explicit index_buffer(Index const (&data)[N]) : base(data) {}
        template <typename Allocator>
        explicit index_buffer(std::vector<Index, Allocator> const &data) : base(data) {}

    private:
        index_buffer() = default;
    };

    template <typename T>
    class typed_uniform_buffer final
    {
    public:
        typed_uniform_buffer()
        {
            glCreateBuffers(1, &handle_);
            // glBufferStorage()
        }

    private:
        GLuint handle_;
    };

    class vertex_array final
    {
    public:
        template <typename... Vertex>
        explicit vertex_array(vertex_buffer<Vertex> &...buffers) : vertex_array()
        {
            (void)std::initializer_list<int>{(this->attach_vbuffer(buffers), 0)...};
        }

        template <typename Index, typename... Vertex>
        explicit vertex_array(index_buffer<Index> &ibuffer, vertex_buffer<Vertex> &...buffers) : vertex_array(buffers...)
        {
            this->set_ibuffer(ibuffer);
        }

        vertex_array(vertex_array &) = delete;
        vertex_array(vertex_array &&other) noexcept { this->swap(other); }

        vertex_array &operator=(vertex_array const &) = delete;
        vertex_array &operator=(vertex_array &&other) noexcept
        {
            this->swap(other);
            return *this;
        }

        ~vertex_array()
        {
            if (handle_ != 0)
            {
                glDeleteVertexArrays(1, &handle_);
            }
        }

        void swap(vertex_array &other) noexcept
        {
            std::swap(handle_, other.handle_);
            std::swap(vbuffers_, other.vbuffers_);
            std::swap(count_, other.count_);
            std::swap(ibuffer_, other.ibuffer_);
            std::swap(index_type_, other.index_type_);
            std::swap(index_size_, other.index_size_);
        }

        void attrib_format(GLuint attrib_index, GLuint vbuffer_index, GLint size, GLenum type, GLboolean normalizing, GLuint relative_offset)
        {
            glVertexArrayAttribFormat(handle_, attrib_index, size, type, normalizing, relative_offset);
            glVertexArrayAttribBinding(handle_, attrib_index, vbuffer_index);
        }

        void enable_attrib(GLuint index)
        {
            glEnableVertexArrayAttrib(handle_, index);
        }

        void disable_attrib(GLuint index)
        {
            glDisableVertexArrayAttrib(handle_, index);
        }

        void bind()
        {
            glBindVertexArray(handle_);
        }

        static void unbind_all()
        {
            glBindVertexArray(0);
        }

        void draw(draw_mode mode)
        {
            draw(mode, 0, count_);
        }

        void draw(draw_mode mode, GLint start, GLsizei count)
        {
            if (ibuffer_.has_value())
            {
                intptr_t indices = start * index_size_;
                glDrawElements(static_cast<GLenum>(mode), count, index_type_, reinterpret_cast<const void *>(indices));
            }
            else
            {
                glDrawArrays(static_cast<GLenum>(mode), start, count);
            }
        }

        GLuint handle() const noexcept { return handle_; }

    private:
        vertex_array()
        {
            ::glCreateVertexArrays(1, &handle_);
        }

        template <typename Vertex>
        void attach_vbuffer(vertex_buffer<Vertex> &vbuffer)
        {
            count_ = std::min(count_, vbuffer.size());
            ::glVertexArrayVertexBuffer(this->handle_, static_cast<GLuint>(vbuffers_.size()), vbuffer.handle(), 0, sizeof(Vertex));
            vbuffers_.push_back(vbuffer.handle());
        }

        template <typename Index>
        void set_ibuffer(index_buffer<Index> &ibuffer)
        {
            assert(!ibuffer_.has_value());
            ibuffer_ = ibuffer.handle();
            index_type_ = details::gl_type_traits<Index>::type;
            index_size_ = details::gl_type_traits<Index>::size;
            count_ = ibuffer.size();
            ::glVertexArrayElementBuffer(handle_, ibuffer.handle());
        }

        GLuint handle_{};
        std::vector<GLuint> vbuffers_{};
        GLsizei count_{std::numeric_limits<GLsizei>::max()};
        std::optional<GLuint> ibuffer_{};
        GLenum index_type_{};
        size_t index_size_{};
    };

    // ----------------- auto VAO for single vertex/index buffer ------------------------

    namespace details
    {
        template <typename T, typename V = bool>
        struct has_vertex_desc : std::false_type
        {
        };

        template <typename T>
        struct has_vertex_desc<T, std::enable_if_t<!std::is_same_v<decltype(T::vertex_desc_t), void>, bool>> : std::true_type
        {
        };

        template <typename T>
        struct attrib_format_helper
        {
            static int run(vertex_array &varray, int attrib_index, int buffer_index, int offset)
            {
                // static_assert(has_vertex_desc<T>::value, "Vertex type must be basic types, vec[x] types or define 'vertex_desc_t' like: using vertex_desc_t = std::tuple<glm::vec3, glm::vec2>");
                return attrib_format_helper<typename T::vertex_desc_t>::run(varray, attrib_index, buffer_index, offset);
            }
        };

        template <typename T>
        struct arithmetic_attrib_format_helper
        {
            static int run(vertex_array &varray, int attrib_index, int buffer_index, int offset)
            {
                varray.enable_attrib(attrib_index);
                varray.attrib_format(attrib_index, buffer_index, 1, gl_type_traits<T>::type, GL_FALSE, offset);
                return attrib_index + 1;
            }
        };
#define GLWRAP_ARITH_FORMAT_HELPER(ori, gltype)                             \
    template <>                                                             \
    struct attrib_format_helper<ori> : arithmetic_attrib_format_helper<ori> \
    {                                                                       \
    };

        GLWRAP_ITER_ARITH_TYPES(GLWRAP_ARITH_FORMAT_HELPER)

        template <glm::length_t L, typename T, glm::qualifier Q>
        struct attrib_format_helper<glm::vec<L, T, Q>>
        {
            static int run(vertex_array &varray, int attrib_index, int buffer_index, int offset)
            {
                varray.enable_attrib(attrib_index);
                varray.attrib_format(attrib_index, buffer_index, L, gl_type_traits<T>::type, GL_FALSE, offset);
                return attrib_index + 1;
            }
        };

        inline int tuple_attrib_format_iter(vertex_array &, int attrib_index, int, int) { return attrib_index; }
        template <typename T, typename... Rest>
        int tuple_attrib_format_iter(vertex_array &varray, int attrib_index, int buffer_index, int offset, attrib_format_helper<T> helper, Rest &&...rest)
        {
            attrib_index = helper.run(varray, attrib_index, buffer_index, offset);
            return tuple_attrib_format_iter(varray, attrib_index, buffer_index, offset + sizeof(T), std::forward<Rest &&>(rest)...);
        }

        template <typename... T>
        struct attrib_format_helper<std::tuple<T...>>
        {
            static int run(vertex_array &varray, int attrib_index, int buffer_index, int offset)
            {
                return tuple_attrib_format_iter(varray, attrib_index, buffer_index, offset, attrib_format_helper<T>{}...);
            }
        };

        inline void attrib_format_iter(vertex_array &, int, int) {}
        template <typename T, typename... Rest>
        void attrib_format_iter(vertex_array &varray, int attrib_index, int buffer_index, attrib_format_helper<T> helper, Rest &&...rest)
        {
            attrib_index = helper.run(varray, attrib_index, buffer_index, 0);
            attrib_format_iter(varray, attrib_index, buffer_index + 1, std::forward<Rest &&>(rest)...);
        }

        template <typename... T>
        void auto_attrib_formats(vertex_array &varray)
        {
            attrib_format_iter(varray, 0, 0, attrib_format_helper<T>{}...);
        }

    }

    template <typename... Vertex>
    auto auto_vertex_array(vertex_buffer<Vertex> &...vbuffer)
    {
        auto varray = vertex_array(vbuffer...);
        details::auto_attrib_formats<Vertex...>(varray);
        return varray;
    }

    template <typename Index, typename... Vertex>
    auto auto_vertex_array(index_buffer<Index> &ibuffer, vertex_buffer<Vertex> &...vbuffer)
    {
        auto varray = vertex_array(ibuffer, vbuffer...);
        details::auto_attrib_formats<Vertex...>(varray);
        return varray;
    }

    // ----------------- shaders -------------------------

    enum class shader_type : GLenum
    {
        vertex = GL_VERTEX_SHADER,
        fragment = GL_FRAGMENT_SHADER,
        geometry = GL_GEOMETRY_SHADER,
    };

    class shader final
    {
    public:
        shader(shader const &) = delete;
        shader(shader &&other) noexcept : shader()
        {
            swap(other);
        }
        shader &operator=(shader const &) = delete;
        shader &operator=(shader &&other) noexcept
        {
            swap(other);
            return *this;
        }
        ~shader()
        {
            if (handle_ != 0)
            {
                glDeleteShader(handle_);
            }
        }

        void swap(shader &other) noexcept
        {
            std::swap(handle_, other.handle_);
        }

        static shader compile(std::string const &str, shader_type type)
        {
            GLuint handle = glCreateShader(static_cast<GLenum>(type));
            const char *const p_char = str.data();
            glShaderSource(handle, 1, &p_char, nullptr);
            glCompileShader(handle);
            int success;
            glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                char log[512];
                glGetShaderInfoLog(handle, 512, nullptr, log);
                glDeleteShader(handle);
                throw gl_error("Shader compile failed: " + std::string(log));
            }
            return shader(handle);
        }

        static shader compile_file(const std::filesystem::path &path, shader_type type)
        {
            try
            {
                return compile(file::read_all_text(path), type);
            }
            catch (std::exception &e)
            {
                throw gl_error(std::string(e.what()) + "\n   file: " + path.string());
            }
        }

        GLuint handle() const noexcept { return handle_; }

    private:
        explicit shader(GLuint handle = 0) noexcept : handle_(handle) {}
        GLuint handle_{};
    };

    class shader_uniform final
    {
    public:
        GLfloat get_float() const
        {
            GLfloat v;
            glGetUniformfv(program_handle_, location_, &v);
            return v;
        }

        void set_float(GLfloat v)
        {
            glProgramUniform1f(program_handle_, location_, v);
        }

        GLboolean get_bool() const
        {
            GLint v;
            glGetUniformiv(program_handle_, location_, &v);
            return v;
        }

        void set_bool(GLboolean v)
        {
            glProgramUniform1i(program_handle_, location_, v);
        }

        GLint get_int() const
        {
            GLint v;
            glGetUniformiv(program_handle_, location_, &v);
            return v;
        }

        void set_int(GLint v)
        {
            glProgramUniform1i(program_handle_, location_, v);
        }

        GLuint get_uint() const
        {
            GLuint v;
            glGetUniformuiv(program_handle_, location_, &v);
            return v;
        }

        void set_uint(GLuint v)
        {
            glProgramUniform1ui(program_handle_, location_, v);
        }

        glm::vec3 get_vec3() const
        {
            glm::vec3 res;
            glGetnUniformfv(program_handle_, location_, sizeof(res), value_ptr(res));
            return res;
        }

        void set_vec3(glm::vec3 v)
        {
            glProgramUniform3fv(program_handle_, location_, 1, value_ptr(v));
        }

        glm::vec4 get_vec4() const
        {
            glm::vec4 res;
            glGetnUniformfv(program_handle_, location_, sizeof(res), value_ptr(res));
            return res;
        }

        void set_vec4(glm::vec4 v)
        {
            glProgramUniform4fv(program_handle_, location_, 1, value_ptr(v));
        }

        glm::mat3 get_mat3() const
        {
            glm::mat3 res;
            glGetnUniformfv(program_handle_, location_, sizeof(res), value_ptr(res));
            return res;
        }

        void set_mat3(glm::mat3 const &v)
        {
            glProgramUniformMatrix3fv(program_handle_, location_, 1, GL_FALSE, value_ptr(v));
        }

        glm::mat4 get_mat4() const
        {
            glm::mat4 res;
            glGetnUniformfv(program_handle_, location_, sizeof(res), value_ptr(res));
            return res;
        }

        void set_mat4(glm::mat4 const &v)
        {
            glProgramUniformMatrix4fv(program_handle_, location_, 1, GL_FALSE, value_ptr(v));
        }

    private:
        friend class shader_program;
        shader_uniform(GLuint program_handle, std::string const &name, bool throw_if_not_found)
            : program_handle_(program_handle),
              location_(glGetUniformLocation(program_handle, name.data()))
        {
            if (throw_if_not_found && this->location_ < 0)
            {
                throw gl_error(std::string("Cannot find uniform \"") + std::string(name) + "\"");
            }
        }
        GLuint program_handle_;
        GLint location_;
    };

    class shader_program final
    {
    public:
        template <typename... T>
        explicit shader_program(T &&...args)
        {
            handle_ = glCreateProgram();
            this->attach_multiple(std::forward<T>(args)...);
            glLinkProgram(handle_);
            int success;
            glGetProgramiv(handle_, GL_LINK_STATUS, &success);
            if (!success)
            {
                char log[512];
                glGetProgramInfoLog(handle_, 512, nullptr, log);
                throw gl_error("Shader link failed: " + std::string(log));
            }
        }

        shader_program(shader_program const &) = delete;
        shader_program(shader_program &&other) noexcept
        {
            swap(other);
        }

        shader_program &operator=(shader_program const &) = delete;
        shader_program &operator=(shader_program &&other) noexcept
        {
            swap(other);
            return *this;
        }

        ~shader_program()
        {
            for (auto &&shader : shaders_)
                glDetachShader(handle_, shader.handle());
            for (auto &&shader_handle : ref_shaders_)
                glDetachShader(handle_, shader_handle);
        }

        void use() const
        {
            glUseProgram(handle_);
        }

        void swap(shader_program &other) noexcept
        {
            std::swap(handle_, other.handle_);
            std::swap(shaders_, other.shaders_);
            std::swap(ref_shaders_, other.ref_shaders_);
        }

        shader_uniform uniform(std::string const &name, bool throw_if_not_found = false) const
        {
            return shader_uniform(handle_, name, throw_if_not_found);
        }

        GLuint handle() const noexcept { return handle_; }

    private:
        void attach(shader &ref)
        {
            glAttachShader(handle_, ref.handle());
            ref_shaders_.push_back(ref.handle());
        }
        void attach(shader &&rref)
        {
            glAttachShader(handle_, rref.handle());
            shaders_.emplace_back(std::move(rref));
        }

        template <typename... T>
        void attach_multiple(T &&...s)
        {
            (void)std::initializer_list<int>{(this->attach(std::forward<T>(s)), 0)...};
        }

        std::vector<shader> shaders_;
        std::vector<GLuint> ref_shaders_;

        GLuint handle_ = 0;
    };

    enum class texture2d_format : GLenum
    {
        unspecified = 0,
        rgb = GL_RGB,
        rgba = GL_RGBA,
    };

    class texture2d final
    {
    public:
        explicit texture2d(GLsizei width, GLsizei height, GLsizei multisamples = 0, texture2d_format format = texture2d_format::rgb)
        {
            GLenum internal_format = format == texture2d_format::rgb ? GL_RGB8 : GL_RGBA8;
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
        }

        explicit texture2d(void const * p, size_t size, texture2d_format format = texture2d_format::unspecified)
        {
            auto required_channel = bitmap_channel::unspecified;
            if (format == texture2d_format::rgb)
                required_channel = bitmap_channel::rgb;
            else if (format == texture2d_format::rgba)
                required_channel = bitmap_channel::rgba;
            auto bmp = bitmap::from_memory(p, size, required_channel, true);
            if (format == texture2d_format::unspecified)
            {
                if (bmp.channels() == bitmap_channel::rgb)
                    format = texture2d_format::rgb;
                else if (bmp.channels() == bitmap_channel::rgba)
                    format = texture2d_format::rgba;
                else
                    throw std::invalid_argument("bitmap from memory has " + to_string(bmp.channels()) + "channels, unknown format");
            }
            else
            {
                if (format == texture2d_format::rgb && bmp.channels() != bitmap_channel::rgb)
                {
                    throw std::invalid_argument("bitmap from memory has " + to_string(bmp.channels()) + "channels, 3 needed");
                }
                if (format == texture2d_format::rgba && bmp.channels() != bitmap_channel::rgba)
                {
                    throw std::invalid_argument("bitmap from memory has " + to_string(bmp.channels()) + "channels, 4 needed");
                }
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &handle_);
            glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLenum internal_format = format == texture2d_format::rgb ? GL_RGB8 : GL_RGBA8;

            glTextureStorage2D(handle_, bmp.max_mipmap_level(), internal_format, bmp.width(), bmp.height());
            if (bmp.is_16bit())
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), static_cast<GLenum>(format), GL_UNSIGNED_SHORT, bmp.pixels_16bit().data());
            }
            else
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), static_cast<GLenum>(format), GL_UNSIGNED_BYTE, bmp.pixels().data());
            }
            glGenerateTextureMipmap(handle_);
        }

        explicit texture2d(std::string const &filename, texture2d_format format = texture2d_format::unspecified)
        {
            auto required_channel = bitmap_channel::unspecified;
            if (format == texture2d_format::rgb)
                required_channel = bitmap_channel::rgb;
            else if (format == texture2d_format::rgba)
                required_channel = bitmap_channel::rgba;
            auto bmp = bitmap::from_file(filename, required_channel, true);
            if (format == texture2d_format::unspecified)
            {
                if (bmp.channels() == bitmap_channel::rgb)
                    format = texture2d_format::rgb;
                else if (bmp.channels() == bitmap_channel::rgba)
                    format = texture2d_format::rgba;
                else
                    throw std::invalid_argument(std::string("bitmap \"") + filename + "\" has " + to_string(bmp.channels()) + "channels, unknown format");
            }
            else
            {
                if (format == texture2d_format::rgb && bmp.channels() != bitmap_channel::rgb)
                {
                    throw std::invalid_argument(std::string("bitmap \"") + filename + "\" has " + to_string(bmp.channels()) + "channels, 3 needed");
                }
                if (format == texture2d_format::rgba && bmp.channels() != bitmap_channel::rgba)
                {
                    throw std::invalid_argument(std::string("bitmap \"") + filename + "\" has " + to_string(bmp.channels()) + "channels, 4 needed");
                }
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &handle_);
            glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            GLenum internal_format = format == texture2d_format::rgb ? GL_RGB8 : GL_RGBA8;

            glTextureStorage2D(handle_, bmp.max_mipmap_level(), internal_format, bmp.width(), bmp.height());
            if (bmp.is_16bit())
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), static_cast<GLenum>(format), GL_UNSIGNED_SHORT, bmp.pixels_16bit().data());
            }
            else
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), static_cast<GLenum>(format), GL_UNSIGNED_BYTE, bmp.pixels().data());
            }
            glGenerateTextureMipmap(handle_);
        }

        texture2d(texture2d const &) = delete;
        texture2d(texture2d &&other) noexcept { swap(other); }
        texture2d &operator=(texture2d const &) = delete;
        texture2d &operator=(texture2d &&other) noexcept
        {
            swap(other);
            return *this;
        }

        ~texture2d()
        {
            if (handle_ != 0)
            {
                glDeleteTextures(1, &handle_);
            }
        }

        void swap(texture2d &other) noexcept
        {
            std::swap(handle_, other.handle_);
            std::swap(width_, other.width_);
            std::swap(height_, other.height_);
        }

        void bind()
        {
            glBindTexture(GL_TEXTURE_2D, handle_);
        }

        void bind_unit(GLuint unit)
        {
            glBindTextureUnit(unit, handle_);
        }

        GLuint handle() const noexcept
        {
            return handle_;
        }

    private:
        texture2d() {}

        GLuint handle_{0};
        GLsizei width_{0};
        GLsizei height_{0};

        friend class frame_buffer;
    };

    class cubemap final
    {
    public:
        cubemap(
            std::string const &right,
            std::string const &left,
            std::string const &top,
            std::string const &bottom,
            std::string const &back,
            std::string const &front)
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
            for (auto &[bmp, _1, _2] : faces)
            {
                if (bmp.is_16bit())
                    throw gl_error("16-bit cubemap not supported currently.");
                if (bmp.width() != width || bmp.height() != height)
                    throw gl_error("Cubemap texture size must be same");
            }

            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle_);
            glTextureStorage2D(handle_, 1, GL_RGB8, width, height);
            for (auto &[bmp, _, i] : faces)
            {
                glTextureSubImage3D(handle_, 0, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, bmp.pixels().data());
            }
            glGenerateTextureMipmap(handle_);
            glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(handle_, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(handle_, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(handle_, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTextureParameteri(handle_, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
        }

        void bind() { glBindTexture(GL_TEXTURE_CUBE_MAP, handle_); }

        void bind_unit(GLuint unit) { glBindTextureUnit(unit, handle_); }

        GLuint handle() { return handle_; }

    private:
        GLuint handle_{0};
    };
    // ----------------- frame buffer --------------------

    class frame_buffer final
    {
    public:
        frame_buffer(GLsizei width, GLsizei height, GLsizei multisamples = 0)
            : width_(width), height_(height), multisamples_(multisamples),
              color_tex_(texture2d(width, height, multisamples))
        {
            glCreateFramebuffers(1, &handle_);
            glNamedFramebufferTexture(handle_, GL_COLOR_ATTACHMENT0, color_tex_.handle(), 0);

            glCreateRenderbuffers(1, &render_buffer_handle_);
            if (multisamples != 0)
            {
                glNamedRenderbufferStorageMultisample(render_buffer_handle_, multisamples, GL_DEPTH24_STENCIL8, width_, height_);
            }
            else
            {
                glNamedRenderbufferStorage(render_buffer_handle_, GL_DEPTH24_STENCIL8, width_, height_);
            }
            glNamedFramebufferRenderbuffer(handle_, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer_handle_);

            if (glCheckNamedFramebufferStatus(handle_, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                throw gl_error("Frame buffer uncomplete.");
            }
        }

        frame_buffer(frame_buffer const &) = delete;
        frame_buffer(frame_buffer &&other) noexcept { swap(other); }

        frame_buffer &operator=(frame_buffer const &) = delete;
        frame_buffer &operator=(frame_buffer &&other) noexcept
        {
            swap(other);
            return *this;
        }

        ~frame_buffer()
        {
            glDeleteRenderbuffers(1, &render_buffer_handle_);
            glDeleteFramebuffers(1, &handle_);
        }

        void swap(frame_buffer &other) noexcept
        {
            std::swap(width_, other.width_);
            std::swap(height_, other.height_);
            std::swap(multisamples_, other.multisamples_);
            std::swap(handle_, other.handle_);
            std::swap(color_tex_, other.color_tex_);
            std::swap(render_buffer_handle_, other.render_buffer_handle_);
        }

        void bind()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, handle_);
        }

        static void unbind_all()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void clear(glm::vec4 color = glm::vec4(0), float depth = 1)
        {
            clear_color(color);
            clear_depth(depth);
        }

        void clear_color(glm::vec4 color = glm::vec4(0))
        {
            glClearNamedFramebufferfv(handle_, GL_COLOR, 0, value_ptr(color));
        }

        void clear_depth(float depth = 1)
        {
            glClearNamedFramebufferfv(handle_, GL_DEPTH, 0, &depth);
        }

        texture2d &color_texture()
        {
            return color_tex_;
        }

        GLuint handle()
        {
            return handle_;
        }

        GLsizei width() const noexcept { return width_; }
        GLsizei height() const noexcept { return height_; }

    private:
        GLsizei width_{0}, height_{0};
        GLsizei multisamples_{0};
        GLuint handle_{0};
        texture2d color_tex_;
        GLuint render_buffer_handle_{0};
    };

}
