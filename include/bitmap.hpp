#pragma once

#include <filesystem>
#include <format>
#include <memory>

enum class bitmap_channel : int
{
    unspecified = 0,
    grey = 1,
    grey_alpha = 2,
    rgb = 3,
    rgba = 4,
};

template <>
struct std::formatter<bitmap_channel> : std::formatter<std::string>
{
    auto format(bitmap_channel channel, std::format_context &ctx) const
    {
        auto &&out = ctx.out();
        switch (channel)
        {
        case bitmap_channel::unspecified:
            return std::format_to(out, "unspecified");
        case bitmap_channel::grey:
            return std::format_to(out, "grey");
        case bitmap_channel::grey_alpha:
            return std::format_to(out, "grey_alpha");
        case bitmap_channel::rgb:
            return std::format_to(out, "rgb");
        case bitmap_channel::rgba:
            return std::format_to(out, "rgba");
        default:
            throw std::invalid_argument(std::format("invalid bitmap_channel value: {}", channel));
        }
    }
};

enum class bitmap_internal_format : int
{
    u8,
    u16,
    f32,
};

template <>
struct std::formatter<bitmap_internal_format> : std::formatter<std::string>
{
    auto format(bitmap_internal_format fmt, std::format_context &ctx) const
    {
        auto &&out = ctx.out();
        switch (fmt)
        {
        case bitmap_internal_format::u8:
            return std::format_to(out, "u8");
        case bitmap_internal_format::u16:
            return std::format_to(out, "u16");
        case bitmap_internal_format::f32:
            return std::format_to(out, "f32");
        default:
            throw std::invalid_argument(std::format("invalid bitmap_internal_format value: {}", fmt));
        }
    }
};

class bitmap final
{
public:
    bitmap(bitmap const &) = delete;
    bitmap(bitmap &&other) noexcept;
    ~bitmap();
    bitmap &operator=(bitmap const &) = delete;
    bitmap &operator=(bitmap &&other) noexcept;

    int width() const noexcept;
    int height() const noexcept;
    bitmap_channel channels() const noexcept;
    bitmap_internal_format internal_format() const noexcept;
    int max_mipmap_level() const noexcept;
    std::byte *pixels() noexcept;

    void swap(bitmap &other) noexcept;

    static bitmap from_memory(std::byte const *p, size_t size, bitmap_channel required_channels = bitmap_channel::unspecified, bool flip_vertically = false);

    static bitmap from_file(std::filesystem::path const &filename, bitmap_channel required_channels = bitmap_channel::unspecified, bool flip_vertically = false);

private:
    bitmap();
    struct bitmap_impl;
    std::unique_ptr<bitmap_impl> impl_;
};
