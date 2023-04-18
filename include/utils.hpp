#pragma once

#include <string>
#include <string_view>
#include <format>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <memory>
#include <numbers>

#include "glwrap.hpp"

namespace utils
{
    namespace details
    {
        template <typename T>
        struct range_iterator final
        {
            T value;

            bool operator== (range_iterator const& other) { return value == other.value; }
            bool operator!= (range_iterator const& other) { return value != other.value; }
            T operator* () { return value; }
            range_iterator& operator++() { ++value; return *this; }
        };

        template <typename T>
        struct range_t final
        {
            range_t(T first, T last) noexcept : first(first), last(last) {}

            T first, last;
            range_iterator<T> begin() const noexcept { return { first }; }
            range_iterator<T> end() const noexcept { return { last }; }
            range_iterator<T> cbegin() const noexcept { return { first }; }
            range_iterator<T> cend() const noexcept { return { last }; }
        };

        template <typename T, typename Size>
        struct ptr_range_t final
        {
            ptr_range_t(T* p, Size size) noexcept : first(p), last(p + size) {}

            T* first, * last;
            T* begin() const noexcept { return first; }
            T* end() const noexcept { return last; }
            T const* cbegin() const noexcept { return first; }
            T const* cend() const noexcept { return last; }
        };
    }

    template <typename T>
    auto range(T first, T last) noexcept { return details::range_t(first, last); }
    template <typename T>
    auto range(T last) noexcept { return details::range_t(static_cast<T>(0), last); }

    template <typename T, typename Size>
    auto ptr_range(T* p, Size size) noexcept { return details::ptr_range_t(p, size); }


    template <typename T, size_t N>
    constexpr size_t array_size(T(&)[N])
    {
        return N;
    }

    // https://stackoverflow.com/questions/63115900
    template <std::ranges::range R>
    auto to_vector(R &&r)
    {
        std::vector<std::ranges::range_value_t<R>> v;

        // if we can get a size, reserve that much
        if constexpr (requires { std::ranges::size(r); })
        {
            v.reserve(std::ranges::size(r));
        }

        // push all the elements
        for (auto &&e : r)
        {
            v.push_back(static_cast<decltype(e) &&>(e));
        }

        return v;
    }

    //
    template <size_t N>
    class fps_counter final
    {
        static_assert(N > 0);
    public:
        explicit fps_counter(float init_time)
            : first_(0), last_(0), count_(1)
        {
            data_[0] = init_time;
        }
        void count(float time) noexcept
        {
            if (count_ == N) {
                first_ = (first_ + 1) % N;
            }
            else {
                ++count_;
            }
            last_ = (last_ + 1) % N;
            data_[last_] = time;
        }
        float fps() const noexcept
        {
            return count_ / (data_[last_] - data_[first_]);
        }

        static constexpr size_t capacity() noexcept
        {
            return N;
        }
    private:
        std::array<float, N> data_{};
        size_t first_;
        size_t last_;
        size_t count_;
    };

    template <typename Func>
    auto benchmark(Func&& func, std::string_view title = "")
    {
        struct timer_guard
        {
            using clock_t = std::chrono::high_resolution_clock;

            clock_t::time_point start;
            std::string_view title;

            timer_guard(std::string_view title) : start(clock_t::now()), title(title) {}

            ~timer_guard()
            {
                auto end = clock_t::now();
                std::cout << title << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(end - start).count() << " ms" << std::endl;
            }
        } guard(title);

        return func();
    }

    struct quad_vertex_t
    {
        using vertex_desc_t = std::tuple<glm::vec2, glm::vec2>;
        glm::vec2 pos;
        glm::vec2 tex;
        quad_vertex_t(float x, float y, float u, float v) : pos(x, y), tex(u, v) {}
    };

    glwrap::vertex_array& get_quad_varray();

    glwrap::vertex_array& get_skybox();

    /*! \brief Create UV sphere vertex_array
     *         If full_information specified, also create normal/texcoords/tangent(GLTF2-style) buffer.
     *         Otherwise only create positions.
     */
    glwrap::vertex_array create_uv_sphere(int slices, int stacks, bool full_information = false);

    struct rotate_by_axis
    {
        rotate_by_axis(float radian, glm::vec3 const axis)
            : radians{radians}, axis{glm::normalize(axis)}
        { }
        glm::quat to_quat() const
        {
            auto cos = std::cos(radians);
            auto sin = std::sin(radians);
            return glm::quat(cos, sin * axis);
        }
        glm::mat4 to_mat4() const
        {
            return glm::rotate(glm::mat4(1), radians, axis);
        }

    private:
        float radians;
        glm::vec3 axis;
    };

    inline glm::mat4 make_transform(
        glm::vec3 const &position,
        glm::vec3 const &scale,
        rotate_by_axis const& rotation
    )
    {
        return glm::scale(rotation.to_mat4() * glm::translate(glm::mat4(1), position), scale);
    }

    inline glm::mat4 make_transform(
        glm::vec3 const &position = glm::vec3(0, 0, 0),
        glm::vec3 const &scale = glm::vec3(1, 1, 1)
    )
    {
        return glm::scale(glm::translate(glm::mat4(1), position), scale);
    }

    glm::vec3 hsv(int h, float s, float v);

    namespace details
    {
        template <typename T, size_t N, typename Func, size_t ...I>
        constexpr auto make_array_impl(Func && func, std::index_sequence<I...>)
        {
            return std::array<T, N>{func(I)...};
        }

        template <size_t N, size_t ...I>
        auto make_uniform_array_impl(glwrap::shader_program & program, std::string const& name, std::index_sequence<I...>)
        {
            std::string formatter = name + std::string("[{}]");
            return std::array<glwrap::shader_uniform, N>{program.uniform(std::vformat(formatter, std::make_format_args(I)))...};
        }
    }

    template <typename T, size_t N, typename Func>
    constexpr std::array<T, N> make_array(Func&& func)
    {
        return details::make_array_impl<T, N>(std::forward<Func>(func), std::make_index_sequence<N>());
    }

    template <size_t N>
    std::array<glwrap::shader_uniform, N> make_uniform_array(glwrap::shader_program & program, std::string const& name)
    {
        return details::make_uniform_array_impl<N>(program, name, std::make_index_sequence<N>());
    }
}

namespace timer
{
    namespace details
    {
        inline auto startup_tp = std::chrono::high_resolution_clock::now();
        inline auto get_time()
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::duration<float>>(now - startup_tp);
            return duration.count();
        };
        inline float current_time = get_time();
        inline float delta_time = 0.0f;
    }
    inline auto time()
    {
        return details::current_time;
    }
    inline auto delta_time()
    {
        return details::delta_time;
    }
    inline void update()
    {
        auto new_time = details::get_time();
        details::delta_time = new_time - details::current_time;
        details::current_time = new_time;
    }
}

inline std::filesystem::path operator""_path(const char *str, size_t)
{
    return std::filesystem::path(str);
}