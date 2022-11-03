#pragma once

#include <string>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <memory>

namespace file
{
    inline std::string read_all_text(std::filesystem::path const& filename)
    {
        std::ifstream f(filename);
        if (!f) throw std::invalid_argument("Open file " + filename.string() + " failed");
        f.seekg(0, std::ios_base::end);
        auto size = f.tellg();
        f.seekg(0, std::ios_base::beg);
        std::string res(size, '\0');
        f.read(res.data(), size);
        return res;
    }
}

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

}
