#include "bitmap.hpp"

//#define STBI_NO_JPEG
//#define STBI_NO_PNG
//#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
//#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM (.ppm and.pgm)
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>

struct bitmap::bitmap_impl final
{
    bitmap_impl(int width, int height, int channels, bitmap_internal_format internal_format, void *ptr, size_t size_in_bytes)
        : width_{width}, height_{height}, channels_{channels}, internal_format_{internal_format}, ptr_{ptr}, size_in_bytes_{size_in_bytes}
    {
        while (width > 1 && height > 1)
        {
            width /= 2;
            height /= 2;
            ++max_mipmap_level_;
        }
    }
    ~bitmap_impl()
    {
        stbi_image_free(ptr_);
    }

    bitmap_impl(bitmap_impl const &) = delete;
    bitmap_impl(bitmap_impl &&) = delete;
    bitmap_impl &operator=(bitmap_impl const &) = delete;
    bitmap_impl &operator=(bitmap_impl &&) = delete;

    void *ptr_;
    size_t size_in_bytes_;
    int width_;
    int height_;
    int channels_;
    bitmap_internal_format internal_format_;
    int max_mipmap_level_{0};
};

bitmap::bitmap()
{ }

bitmap::~bitmap() = default;

bitmap::bitmap(bitmap && other) noexcept
{
    swap(other);
}

bitmap &bitmap::operator=(bitmap && other) noexcept
{
    swap(other);
    return *this;
}

int bitmap::width() const noexcept { return impl_->width_; }

int bitmap::height() const noexcept { return impl_->height_; }

bitmap_channel bitmap::channels() const noexcept
{
    return static_cast<bitmap_channel>(impl_->channels_);
}

bitmap_internal_format bitmap::internal_format() const noexcept
{
    return impl_->internal_format_;
}

int bitmap::max_mipmap_level() const noexcept
{
    return impl_->max_mipmap_level_;
}

std::byte *bitmap::pixels() noexcept
{
    return reinterpret_cast<std::byte*>(impl_->ptr_);
}

void bitmap::swap(bitmap &other) noexcept
{
    std::swap(impl_, other.impl_);
}

bitmap bitmap::from_memory(std::byte const *p, size_t size, bitmap_channel required_channels, bool flip_vertically)
{
    int width, height, channels;
    auto puc = reinterpret_cast<stbi_uc const *>(p);
    auto len = static_cast<int>(size);
    void *raw_data;
    bitmap_internal_format internal_format;
    size_t size_in_bytes;
    stbi_set_flip_vertically_on_load_thread(flip_vertically);
    if (stbi_is_hdr_from_memory(puc, len))
    {
        raw_data = stbi_loadf_from_memory(puc, len, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::f32;
        size_in_bytes = width * height * channels * 4;
    }
    else if (stbi_is_16_bit_from_memory(puc, len))
    {
        raw_data = stbi_load_16_from_memory(puc, len, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::u16;
        size_in_bytes = width * height * channels * 2;
    }
    else
    {
        raw_data = stbi_load_from_memory(puc, len, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::u8;
        size_in_bytes = width * height * channels;
    }
    if (raw_data == nullptr)
    {
        throw std::invalid_argument("bitmap from memory load failed");
    }
    bitmap bmp;
    bmp.impl_ = std::make_unique<bitmap_impl>(width, height, channels, internal_format, raw_data, size_in_bytes);
    return bmp;
}

bitmap bitmap::from_file(std::filesystem::path const& path, bitmap_channel required_channels, bool flip_vertically)
{
    auto fp = std::fopen(path.string().c_str(), "rb");
    if (!fp)
    {
        throw std::runtime_error(std::format("Cannot open file: {}", path.string()));
    }
    int width, height, channels;
    void *raw_data;
    bitmap_internal_format internal_format;
    size_t size_in_bytes;
    stbi_set_flip_vertically_on_load_thread(flip_vertically);
    if (stbi_is_hdr_from_file(fp))
    {
        raw_data = stbi_loadf_from_file(fp, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::f32;
        size_in_bytes = width * height * channels * 4;
    }
    else if (stbi_is_16_bit_from_file(fp))
    {
        raw_data = stbi_load_from_file_16(fp, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::u16;
        size_in_bytes = width * height * channels * 2;
    }
    else
    {
        raw_data = stbi_load_from_file(fp, &width, &height, &channels, static_cast<int>(required_channels));
        internal_format = bitmap_internal_format::u8;
        size_in_bytes = width * height * channels;
    }
    if (raw_data == nullptr)
    {
        throw std::invalid_argument(std::format("Bitmap file load failed: {}", path.string()));
    }
    bitmap bmp;
    bmp.impl_ = std::make_unique<bitmap_impl>(width, height, channels, internal_format, raw_data, size_in_bytes);
    return bmp;
}
