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
    Vertex can be any type, Index must be unsigned integral type.
    VBO will be deleted when lifetime ends.
    Examples:
    - auto vbuffer = vertex_buffer(box_vertices);
    - auto ibuffer = index_buffer(indices);

    * Create vertex array
    Use:   vertex_array(index_buffer<Index>&, vertex_buffer<Vertex>&...)
        or vertex_array(vertex_buffer<Vertex>&...)
    Rvalue-referenced (moved) buffers are also allowed, in this situation, the moved buffer will be kept by the vertex array.
    If index_buffer<Index> is set then vbo.draw() will call glDrawElements, otherwise glDrawArrays.
    Vertex buffers indices will be in sequence (0, 1, 2, ...).
    vertex_array will not keep any of the lvalue-referenced buffers. Please make sure that all the buffers are alive when using vertex_array.
    Examples:
    - auto varray = vertex_array(ibuffer, vbuffer);
    - auto varray = vertex_array(ibuffer<GLuint>{0, 1, 2, 1, 2, 3}, vertex_buffer<vert_t>{ ... });

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
        ==> attrib_format(attrib_index: 0, vbo_index: 0, size: 1, type: GL_INT, normalizing: GL_FALSE, offset: 0)

    - Vertex type: glm::vec3 (glm::vec<3, float, ...>)
        ==> attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)

    - Vertex type: struct vertex_t {
                       using vertex_desc_t = std::tuple<glm::vec3, glm::vec2>;
                       glm::vec3 position;
                       glm::vec2 tex_coord;
                   };
        ==> attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)
            attrib_format(attrib_index: 1, vbo_index: 0, size: 2, type: GL_FLOAT, normalizing: GL_FALSE, offset: 3 * sizeof(float))
            ** Cannot use std::tuple<glm::vec3, glm::vec2> as vertex type directly, since std::tuple is not standard-layout.

    - Vertex type: glm::vec3 (buffer 0), glm::vec2 (buffer 1)
        ==> attrib_format(attrib_index: 0, vbo_index: 0, size: 3, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)
            attrib_format(attrib_index: 1, vbo_index: 1, size: 2, type: GL_FLOAT, normalizing: GL_FALSE, offset: 0)

    * Shaders and program
    Use:  shader::compile(code, type) or shader::compile_file(filename, type)
          shader_program(shaders...)
    If shader_program receives an rvalue-referenced shader, it will keep that shader alive.
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
#include <iostream>
#include <fstream>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <cstdint>
#include <array>
#include <map>
#include <tuple>
#include <optional>
#include <format>
#include <span>
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "bitmap.hpp"

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

    class buffer_base abstract
    {
    public:
        virtual ~buffer_base() = 0 {}
    };

    template <typename T>
    class buffer abstract : public buffer_base
    {
    public:
        buffer(T const *data, size_t length)
        {
            if (length > std::numeric_limits<GLsizei>::max())
                throw std::invalid_argument(std::format("buffer data too large ({})", length));
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
            std::swap(count_, other.count_);
        }

        virtual ~buffer() override
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
        static_assert(std::is_integral_v<Index> && std::is_unsigned_v<Index>, "index must be unsigned integral type");

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
        // ----------- load from simple json -----------
        // file content:
        //     {
        //         "position": [0,0,0,1,1,1,...], // group-by-3
        //         "normal": [0,0,1,...] // group-by-3
        //         "tex": [0.5,0.5,...]  // group-by-2
        //         "index": [0,1,3,...]
        //     }
        static vertex_array load_simple_json(std::filesystem::path const &path);

        vertex_array()
        {
            ::glCreateVertexArrays(1, &handle_);
        }

        template <typename... VertexBuffers>
        explicit vertex_array(VertexBuffers &&... buffers) : vertex_array()
        {
            this->attach_vbuffers(std::forward<VertexBuffers>(buffers)...);
        }

        template <typename Index, typename... VertexBuffers>
        explicit vertex_array(index_buffer<Index> &ibuffer, VertexBuffers &&...vbuffers) : vertex_array(std::forward<VertexBuffers>(vbuffers)...)
        {
            this->set_ibuffer(ibuffer);
        }

        template <typename Index, typename... VertexBuffers>
        explicit vertex_array(index_buffer<Index> &&moved_ibuffer, VertexBuffers &&...vbuffers) : vertex_array(std::forward<VertexBuffers>(vbuffers)...)
        {
            this->set_ibuffer(std::move(moved_ibuffer));
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
            std::swap(vcount_, other.vcount_);
            std::swap(icount_, other.icount_);
            std::swap(ibuffer_, other.ibuffer_);
            std::swap(index_type_, other.index_type_);
            std::swap(index_size_, other.index_size_);
        }

        void attrib_format(GLuint attrib_index, GLuint vbuffer_index, GLint size, GLenum type, GLboolean normalizing, GLuint relative_offset)
        {
            glVertexArrayAttribFormat(handle_, attrib_index, size, type, normalizing, relative_offset);
            glVertexArrayAttribBinding(handle_, attrib_index, vbuffer_index);
        }

        void binding_divisor(GLuint vbuffer_index, GLuint divisor)
        {
            glVertexArrayBindingDivisor(handle_, vbuffer_index, divisor);
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
            if (ibuffer_.has_value())
            {
                draw(mode, 0, icount_);
            }
            else 
            {
                if (!vcount_.has_value())
                {
                    throw std::runtime_error("Cannot draw empty vertex_array");
                }
                draw(mode, 0, vcount_.value());
            }
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

        void draw_instanced(draw_mode mode, GLsizei instance_count)
        {
            if (ibuffer_.has_value())
            {
                draw_instanced(mode, 0, icount_, instance_count);
            }
            else
            {
                if (!vcount_.has_value())
                {
                    throw std::runtime_error("Cannot draw empty vertex_array");
                }
                draw_instanced(mode, 0, vcount_.value(), instance_count);
            }
        }

        void draw_instanced(draw_mode mode, GLint start, GLsizei count, GLsizei instance_count)
        {
            if (ibuffer_.has_value())
            {
                intptr_t indices = start * index_size_;
                glDrawElementsInstanced(static_cast<GLenum>(mode), count, index_type_, reinterpret_cast<const void *>(indices), instance_count);
            }
            else
            {
                glDrawArraysInstanced(static_cast<GLenum>(mode), start, count, instance_count);
            }
        }

        GLuint handle() const noexcept { return handle_; }

        template <typename Vertex>
        size_t attach_vbuffer(vertex_buffer<Vertex> & vbuffer)
        {
            auto index = vbuffers_.size();
            vcount_ = vcount_.has_value() ? std::min(vcount_.value(), vbuffer.size()) : vbuffer.size();
            ::glVertexArrayVertexBuffer(this->handle_, static_cast<GLuint>(index), vbuffer.handle(), 0, sizeof(Vertex));
            vbuffers_.push_back(vbuffer.handle());
            return index;
        }

        template <typename Vertex>
        size_t attach_vbuffer(vertex_buffer<Vertex> && moved_vbuffer)
        {
            auto holded_ptr = std::make_unique<vertex_buffer<Vertex>>(std::move(moved_vbuffer));
            auto index = vbuffers_.size();
            vcount_ = vcount_.has_value() ? std::min(vcount_.value(), holded_ptr->size()) : holded_ptr->size();
            ::glVertexArrayVertexBuffer(this->handle_, static_cast<GLuint>(index), holded_ptr->handle(), 0, sizeof(Vertex));
            vbuffers_.push_back(holded_ptr->handle());
            holded_buffers_.push_back(std::move(holded_ptr));
            return index;
        }

        template <typename ...VertexBuffers>
        void attach_vbuffers(VertexBuffers &&...vbuffers)
        {
            (attach_vbuffer(std::forward<VertexBuffers>(vbuffers)), ...);
        }

        template <typename Index>
        void set_ibuffer(index_buffer<Index> &ibuffer)
        {
            if (ibuffer_.has_value())
            {
                throw std::runtime_error("Vertex array already has index buffer");
            }
            ibuffer_ = ibuffer.handle();
            index_type_ = details::gl_type_traits<Index>::type;
            index_size_ = details::gl_type_traits<Index>::size;
            icount_ = ibuffer.size();
            ::glVertexArrayElementBuffer(handle_, ibuffer.handle());
        }

        template <typename Index>
        void set_ibuffer(index_buffer<Index> &&moved_ibuffer)
        {
            if (ibuffer_.has_value())
            {
                throw std::runtime_error("Vertex array already has index buffer");
            }
            auto holded_ptr = std::make_unique<index_buffer<Index>>(std::move(moved_ibuffer));
            ibuffer_ = holded_ptr->handle();
            index_type_ = details::gl_type_traits<Index>::type;
            index_size_ = details::gl_type_traits<Index>::size;
            icount_ = holded_ptr->size();
            ::glVertexArrayElementBuffer(handle_, holded_ptr->handle());
            holded_buffers_.push_back(std::move(holded_ptr));
        }

    private:

        GLuint handle_{};
        std::vector<GLuint> vbuffers_{};
        std::optional<GLsizei> vcount_{};
        GLsizei icount_{};
        std::optional<GLuint> ibuffer_{};
        GLenum index_type_{};
        size_t index_size_{};
        std::vector<std::unique_ptr<buffer_base>> holded_buffers_{};
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

        template <typename T>
        struct extract_vertex_type{
        };

        template <typename T>
        struct extract_vertex_type<vertex_buffer<T>>
        {
            using type = T;
        };
        template <typename T>
        using extract_vertex_type_t = typename extract_vertex_type<T>::type;

        template <typename... VertexBuffers>
        void auto_attrib_formats(vertex_array &varray)
        {
            attrib_format_iter(varray, 0, 0, attrib_format_helper<extract_vertex_type_t<std::remove_cvref_t<VertexBuffers>>>{}...);
        }

    }

    template <typename... VertexBuffers>
    auto auto_vertex_array(VertexBuffers &&...vbuffers)
    {
        auto varray = vertex_array(std::forward<VertexBuffers>(vbuffers)...);
        details::auto_attrib_formats<VertexBuffers...>(varray);
        return varray;
    }

    template <typename Index, typename... VertexBuffers>
    auto auto_vertex_array(index_buffer<Index> &ibuffer, VertexBuffers &&...vbuffers)
    {
        auto varray = vertex_array(ibuffer, std::forward<VertexBuffers>(vbuffers)...);
        details::auto_attrib_formats<VertexBuffers...>(varray);
        return varray;
    }

    template <typename Index, typename... VertexBuffers>
    auto auto_vertex_array(index_buffer<Index> &&moved_ibuffer, VertexBuffers &&...vbuffers)
    {
        auto varray = vertex_array(std::move(moved_ibuffer), std::forward<VertexBuffers>(vbuffers)...);
        details::auto_attrib_formats<VertexBuffers...>(varray);
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

        static shader compile_file(std::filesystem::path const& path, shader_type type)
        {
            try
            {
                std::ifstream f(path);
                if (!f)
                    throw std::invalid_argument("Open file " + path.string() + " failed");
                f.seekg(0, std::ios_base::end);
                auto size = f.tellg();
                f.seekg(0, std::ios_base::beg);
                std::string text(size, '\0');
                f.read(text.data(), size);
                return compile(text, type);
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

        glm::vec2 get_vec2() const
        {
            glm::vec2 res;
            glGetnUniformfv(program_handle_, location_, sizeof(res), value_ptr(res));
            return res;
        }

        void set_vec2(glm::vec2 v)
        {
            glProgramUniform2fv(program_handle_, location_, 1, value_ptr(v));
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
        shader_uniform(GLuint program_handle, std::string_view name)
            : program_handle_(program_handle),
              location_(glGetUniformLocation(program_handle, name.data()))
        {
            if (this->location_ < 0)
            {
                std::cout << std::format("Cannot find uniform \"{}\"", name) << std::endl;
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

        shader_uniform uniform(std::string_view name) const
        {
            return shader_uniform(handle_, name);
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
            (this->attach(std::forward<T>(s)), ...);
        }

        std::vector<shader> shaders_;
        std::vector<GLuint> ref_shaders_;

        GLuint handle_ = 0;
    };

    enum class texture2d_format : GLenum
    {
        unspecified = 0,
        rgb,
        rgba,
        srgb,
        srgba,
        grey,
    };

    enum class texture2d_elem_type : GLenum
    {
        u8,
        f16,
        f32,
    };

    class texture2d final
    {
    public:
        texture2d(GLsizei width, GLsizei height, GLsizei multisamples, GLenum internal_format)
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
        }

        texture2d(GLsizei width, GLsizei height, GLsizei multisamples = 0, texture2d_format format = texture2d_format::rgb, texture2d_elem_type elem_type = texture2d_elem_type::u8)
            : texture2d(width, height, multisamples, to_internal_format(format, elem_type))
        { }

        texture2d(void const *p, size_t size, bool srgb = false, texture2d_elem_type elem_type = texture2d_elem_type::u8, texture2d_format format = texture2d_format::unspecified)
        {
            auto required_channel = bitmap_channel::unspecified;
            if (format == texture2d_format::rgb)
                required_channel = bitmap_channel::rgb;
            else if (format == texture2d_format::rgba)
                required_channel = bitmap_channel::rgba;
            auto bmp = bitmap::from_memory(p, size, required_channel, true);
            create_resources(srgb, format, elem_type, bmp);
        }

        explicit texture2d(std::filesystem::path const &filename, bool srgb = false, texture2d_elem_type elem_type = texture2d_elem_type::u8,
          texture2d_format format = texture2d_format::unspecified)
        {
            auto required_channel = bitmap_channel::unspecified;
            if (format == texture2d_format::rgb)
                required_channel = bitmap_channel::rgb;
            else if (format == texture2d_format::rgba)
                required_channel = bitmap_channel::rgba;
            auto bmp = bitmap::from_file(filename, required_channel, true);
            create_resources(srgb, format, elem_type, bmp);
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

        void create_resources(bool srgb, texture2d_format format, texture2d_elem_type elem_type, bitmap & bmp)
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
                if ((format == texture2d_format::rgb || format == texture2d_format::srgb)
                 && bmp.channels() != bitmap_channel::rgb)
                {
                    throw std::invalid_argument(std::format("bitmap from memory has {} channels, 3 needed.", bmp.channels()));
                }
                if ((format == texture2d_format::rgba || format == texture2d_format::srgba)
                 && bmp.channels() != bitmap_channel::rgba)
                {
                    throw std::invalid_argument(std::format("bitmap from memory has {} channels, 4 needed.", bmp.channels()));
                }
                if (format == texture2d_format::grey && bmp.channels() != bitmap_channel::grey)
                {
                    throw std::invalid_argument(std::format("bitmap from memory has {} channels, 1 needed.", bmp.channels()));
                }
            }

            glCreateTextures(GL_TEXTURE_2D, 1, &handle_);
            glTextureParameteri(handle_, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(handle_, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glTextureStorage2D(handle_, bmp.max_mipmap_level(), to_internal_format(format, elem_type), bmp.width(), bmp.height());
            if (bmp.is_16bit())
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), to_image_format(format), GL_UNSIGNED_SHORT, bmp.pixels_16bit().data());
            }
            else
            {
                glTextureSubImage2D(handle_, 0, 0, 0, bmp.width(), bmp.height(), to_image_format(format), GL_UNSIGNED_BYTE, bmp.pixels().data());
            }
            glGenerateTextureMipmap(handle_);
        }

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

        GLuint handle_{0};

        friend class frame_buffer;
    };

    class cubemap final
    {
    public:
        using path = std::filesystem::path;
        cubemap(
            path const &right,
            path const &left,
            path const &top,
            path const &bottom,
            path const &back,
            path const &front)
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
            glTextureStorage2D(handle_, 1, GL_SRGB8, width, height);
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

        explicit cubemap(path const &folder, std::string const& file_ext)
         : cubemap(folder / ("right" + file_ext),
                   folder / ("left" + file_ext),
                   folder / ("top" + file_ext),
                   folder / ("bottom" + file_ext),
                   folder / ("front" + file_ext),
                   folder / ("back" + file_ext))
        { }

        cubemap(cubemap&& other) noexcept
        {
            this->swap(other);
        }

        ~cubemap()
        {
            if (handle_ != 0)
            {
                glDeleteTextures(1, &handle_);
            }
        }

        cubemap& operator= (cubemap&& other) noexcept
        {
            this->swap(other);
            return *this;
        }

        void bind() { glBindTexture(GL_TEXTURE_CUBE_MAP, handle_); }

        void bind_unit(GLuint unit) { glBindTextureUnit(unit, handle_); }

        GLuint handle() { return handle_; }

        void swap(cubemap & other) {
            std::swap(handle_, other.handle_);
        }

    private:
        GLuint handle_{0};
    };
    // ----------------- frame buffer --------------------

    class frame_buffer final
    {
    private:
        static std::vector<texture2d> create_textures(size_t target_count, GLsizei width, GLsizei height, GLsizei multisamples, bool is_hdr)
        {
            std::vector<texture2d> result;
            for (size_t i = 0; i < target_count; ++i)
            {
                result.emplace_back(width, height, multisamples, texture2d_format::rgba, is_hdr ? texture2d_elem_type::f16 : texture2d_elem_type::u8);
            }
            return result;
        }

    public:
        frame_buffer(std::vector<texture2d> &&textures, GLsizei width, GLsizei height, GLsizei multisamples = 0)
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

        frame_buffer(GLsizei width, GLsizei height, GLsizei multisamples = 0, bool is_hdr = false)
            : frame_buffer(1, width, height, multisamples, is_hdr)
        { }

        frame_buffer(size_t target_count, GLsizei width, GLsizei height, GLsizei multisamples = 0, bool is_hdr = false)
            : frame_buffer(create_textures(target_count, width, height, multisamples, is_hdr), width, height, multisamples)
        { }

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
            glDeleteFramebuffers(1, &handle_);
        }

        void swap(frame_buffer &other) noexcept
        {
            std::swap(width_, other.width_);
            std::swap(height_, other.height_);
            std::swap(multisamples_, other.multisamples_);
            std::swap(handle_, other.handle_);
            std::swap(color_textures_, other.color_textures_);
            std::swap(depth_texture_, other.depth_texture_);
        }

        void bind()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, handle_);
        }

        static void unbind_all()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        void clear(glm::vec4 color = glm::vec4(0), float depth = 1, size_t draw_buffer = 0)
        {
            clear_color(color, draw_buffer);
            clear_depth(depth);
        }

        void clear_color(glm::vec4 color = glm::vec4(0), size_t draw_buffer = 0)
        {
            glClearNamedFramebufferfv(handle_, GL_COLOR, draw_buffer, value_ptr(color));
        }

        void clear_depth(float depth = 1)
        {
            glClearNamedFramebufferfv(handle_, GL_DEPTH, 0, &depth);
        }

        texture2d &color_texture()
        {
            return color_textures_.at(0);
        }

        texture2d &color_texture_at(size_t index)
        {
            return color_textures_.at(index);
        }

        texture2d &depth_texture()
        {
            return depth_texture_;
        }

        void draw_buffers(std::span<size_t> indexes)
        {
            std::vector<GLenum> dst{};
            for (auto i : indexes)
            {
                dst.push_back(GL_COLOR_ATTACHMENT0 + i);
            }
            glNamedFramebufferDrawBuffers(handle_, dst.size(), dst.data());
        }

        void draw_buffers(std::initializer_list<size_t> indexes)
        {
            std::vector<GLenum> dst{};
            for (auto i : indexes)
            {
                dst.push_back(GL_COLOR_ATTACHMENT0 + i);
            }
            glNamedFramebufferDrawBuffers(handle_, dst.size(), dst.data());
        }

        GLuint handle()
        {
            return handle_;
        }

        GLsizei width() const noexcept { return width_; }
        GLsizei height() const noexcept { return height_; }

        void blit_to(frame_buffer& other, GLbitfield mask, GLenum filter = GL_NEAREST)
        {
            glBlitNamedFramebuffer(handle_, other.handle_, 0, 0, width_, height_, 0, 0, other.width_, other.height_, mask, filter);
        }

    private:
        GLsizei width_{0}, height_{0};
        GLsizei multisamples_{0};
        GLuint handle_{0};
        std::vector<texture2d> color_textures_;
        texture2d depth_texture_;
    };
}

template <>
struct std::formatter<glwrap::texture2d_format>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glwrap::texture2d_format format, std::format_context &ctx)
    {
        auto &&out = ctx.out();
        switch (format)
        {
        case glwrap::texture2d_format::unspecified:
            return std::format_to(out, "unspecified");
        case glwrap::texture2d_format::rgb:
            return std::format_to(out, "rgb");
        case glwrap::texture2d_format::rgba:
            return std::format_to(out, "rgba");
        case glwrap::texture2d_format::grey:
            return std::format_to(out, "grey");
        default:
            throw std::invalid_argument(std::format("Invalid texture2d_format type: {}", static_cast<int>(format)));
        }
    }
};

template <>
struct std::formatter<glwrap::texture2d_elem_type>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glwrap::texture2d_elem_type format, std::format_context &ctx)
    {
        auto &&out = ctx.out();
        switch (format)
        {
        case glwrap::texture2d_elem_type::u8:
            return std::format_to(out, "u8");
        case glwrap::texture2d_elem_type::f16:
            return std::format_to(out, "f16");
        case glwrap::texture2d_elem_type::f32:
            return std::format_to(out, "f32");
        default:
            throw std::invalid_argument(std::format("Invalid texture2d_format type: {}", static_cast<int>(format)));
        }
    }
};

template <>
struct std::formatter<glm::vec1>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glm::vec1 const &vec, std::format_context &ctx)
    {
        return std::format_to(ctx.out(), "({})", vec.x);
    };
};

template <>
struct std::formatter<glm::vec2>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glm::vec2 const &vec, std::format_context &ctx)
    {
        return std::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    };
};

template <>
struct std::formatter<glm::vec3>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glm::vec3 const &vec, std::format_context &ctx)
    {
        return std::format_to(ctx.out(), "({}, {}, {})", vec.x, vec.y, vec.z);
    };
};

template <>
struct std::formatter<glm::vec4>
{
    constexpr auto parse(std::format_parse_context &ctx)
    {
        return ctx.begin();
    }
    auto format(glm::vec4 const &vec, std::format_context &ctx)
    {
        return std::format_to(ctx.out(), "({}, {}, {}, {})", vec.x, vec.y, vec.z, vec.w);
    };
};
