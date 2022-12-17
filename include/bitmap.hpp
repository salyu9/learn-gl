#pragma once

#include <vector>
#include <filesystem>
#include <utility>
#include <cstdint>

enum class bitmap_channel : int
{
    unspecified = 0,
    grey = 1,
    grey_alpha = 2,
    rgb = 3,
    rgba = 4,
};

inline std::string to_string(bitmap_channel channel)
{
    switch (channel)
    {
    case bitmap_channel::unspecified:
        return "unspecified";
    case bitmap_channel::grey:
        return "grey";
    case bitmap_channel::grey_alpha:
        return "grey_alpha";
    case bitmap_channel::rgb:
        return "rgb";
    case bitmap_channel::rgba:
        return "rgba";
    default:
        throw std::invalid_argument("invalid bitmap_channel value");
    }
}

class bitmap final
{
public:
    bitmap(bitmap const &) = delete;
    bitmap(bitmap &&other) noexcept { swap(other); }

    bitmap &operator=(bitmap const &) = delete;
    bitmap &operator=(bitmap &&other) noexcept
    {
        swap(other);
        return *this;
    }

    bool is_16bit() const noexcept { return is_16bit_; }
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }
    bitmap_channel channels() const noexcept { return static_cast<bitmap_channel>(channels_); }
    int max_mipmap_level() const noexcept { return max_mipmap_level_; }
    std::vector<std::uint8_t> const &pixels() const noexcept { return pixels_; }
    std::vector<std::uint16_t> const &pixels_16bit() const noexcept { return pixels_16bit_; }

    void swap(bitmap &other) noexcept
    {
        std::swap(pixels_, other.pixels_);
        std::swap(pixels_16bit_, other.pixels_16bit_);
        std::swap(is_16bit_, other.is_16bit_);
        std::swap(width_, other.width_);
        std::swap(height_, other.height_);
        std::swap(channels_, other.channels_);
        std::swap(max_mipmap_level_, other.max_mipmap_level_);
    }

    static bitmap from_memory(void const *p, size_t size, bitmap_channel required_channels = bitmap_channel::unspecified, bool flip_vertically = false);

    static bitmap from_file(const char *filename, bitmap_channel required_channels = bitmap_channel::unspecified, bool flip_vertically = false);
    static bitmap from_file(std::filesystem::path const &filename, bitmap_channel required_channels = bitmap_channel::unspecified, bool flip_vertically = false)
    {
        return from_file((const char *)filename.u8string().c_str(), required_channels, flip_vertically);
    }

private:
    bitmap(int width, int height, int channels, std::vector<std::uint8_t> &&data)
        : pixels_(std::move(data)), is_16bit_(false), width_(width), height_(height), channels_(channels), max_mipmap_level_(1)
    {
        while (width > 1 && height > 1)
        {
            width /= 2;
            height /= 2;
            ++max_mipmap_level_;
        }
    }
    bitmap(int width, int height, int channels, std::vector<std::uint16_t> &&data)
        : pixels_16bit_(std::move(data)), is_16bit_(true), width_(width), height_(height), channels_(channels), max_mipmap_level_(1)
    {
        while (width > 1 && height > 1)
        {
            width /= 2;
            height /= 2;
            ++max_mipmap_level_;
        }
    }
    std::vector<std::uint8_t> pixels_{};
    std::vector<std::uint16_t> pixels_16bit_{};
    bool is_16bit_{};
    int width_{};
    int height_{};
    int channels_{};
    int max_mipmap_level_{};
};
