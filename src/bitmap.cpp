#include "bitmap.hpp"

//#define STBI_NO_JPEG
//#define STBI_NO_PNG
//#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM (.ppm and.pgm)
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

bitmap bitmap::from_memory(std::byte const * p, size_t size, bitmap_channel required_channels, bool flip_vertically)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flip_vertically);
    auto puc = reinterpret_cast<stbi_uc const *>(p);
    auto len = static_cast<int>(size);
    if (stbi_is_16_bit_from_memory(puc, len))
    {
        auto raw_data = stbi_load_16_from_memory(puc, len, &width, &height, &channels, static_cast<int>(required_channels));
        if (raw_data == nullptr)
            throw std::invalid_argument("bitmap from memory load failed");
        std::vector<std::uint16_t> pixels(raw_data, raw_data + width * height * channels);
        stbi_image_free(raw_data);
        return bitmap(width, height, channels, std::move(pixels));
    }
    else
    {
        auto raw_data = stbi_load_from_memory(puc, len, &width, &height, &channels, static_cast<int>(required_channels));
        if (raw_data == nullptr)
            throw std::invalid_argument("bitmap from memory load failed");
        std::vector<std::uint8_t> pixels(raw_data, raw_data + width * height * channels);
        stbi_image_free(raw_data);
        return bitmap(width, height, channels, std::move(pixels));
    }
}

bitmap bitmap::from_file(const char *filename, bitmap_channel required_channels, bool flip_vertically)
{
    int width, height, channels;
    stbi_set_flip_vertically_on_load(flip_vertically);
    if (stbi_is_16_bit(filename))
    {
        auto raw_data = stbi_load_16(filename, &width, &height, &channels, static_cast<int>(required_channels));
        if (raw_data == nullptr)
            throw std::invalid_argument(std::string("bitmap \"") + filename + "\" load failed");
        std::vector<std::uint16_t> pixels(raw_data, raw_data + width * height * channels);
        stbi_image_free(raw_data);
        return bitmap(width, height, channels, std::move(pixels));
    }
    else
    {
        auto raw_data = stbi_load(filename, &width, &height, &channels, static_cast<int>(required_channels));
        if (raw_data == nullptr)
            throw std::invalid_argument(std::string("bitmap \"") + filename + "\" load failed");
        std::vector<std::uint8_t> pixels(raw_data, raw_data + width * height * channels);
        stbi_image_free(raw_data);
        return bitmap(width, height, channels, std::move(pixels));
    }
}
