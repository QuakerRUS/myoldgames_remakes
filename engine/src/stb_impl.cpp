// Implementation unit of the stb single-header libraries (compiled without warnings in vs_stb).
#define _CRT_SECURE_NO_WARNINGS
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_VORBIS_NO_STDIO
#include <stb_image.h>
#include <stb_image_write.h>
#include <stb_truetype.h>
#include <stb_vorbis.c>

#include <cstdlib>

#include "stb.hpp"

namespace vs::stb {

unsigned char* load_image_rgba(const unsigned char* data, std::size_t size, int* w, int* h, int* channels) {
    return stbi_load_from_memory(data, static_cast<int>(size), w, h, channels, 4);
}

void free_image(unsigned char* pixels) { stbi_image_free(pixels); }

const char* image_failure_reason() {
    const char* r = stbi_failure_reason();
    return r ? r : "unknown error";
}

std::vector<std::uint8_t> encode_png_rgba(const std::uint8_t* rgba, int w, int h) {
    int len = 0;
    unsigned char* png = stbi_write_png_to_mem(rgba, w * 4, w, h, 4, &len);
    if (!png) return {};
    std::vector<std::uint8_t> out(png, png + len);
    STBIW_FREE(png);
    return out;
}

bool decode_vorbis(const unsigned char* data, std::size_t size, int* channels, int* sample_rate,
                   std::vector<std::int16_t>& samples) {
    short* output = nullptr;
    const int frames = stb_vorbis_decode_memory(data, static_cast<int>(size), channels, sample_rate, &output);
    if (frames < 0 || !output) return false;
    samples.assign(output, output + static_cast<std::size_t>(frames) * static_cast<std::size_t>(*channels));
    std::free(output);
    return true;
}

}  // namespace vs::stb
