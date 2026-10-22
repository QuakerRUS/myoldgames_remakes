#pragma once
// Private wrapper around the stb libraries so that engine sources never include their
// implementation headers (which are not /W4 clean). Implemented in stb_impl.cpp.
#include <cstddef>
#include <cstdint>
#include <vector>

namespace vs::stb {

// stb_image: decodes to RGBA8; returns nullptr on failure. Free with free_image().
unsigned char* load_image_rgba(const unsigned char* data, std::size_t size, int* w, int* h, int* channels);
void free_image(unsigned char* pixels);
const char* image_failure_reason();

// stb_image_write: PNG bytes of an RGBA8 image (empty on failure).
std::vector<std::uint8_t> encode_png_rgba(const std::uint8_t* rgba, int w, int h);

// stb_vorbis: decodes a whole OGG Vorbis file to interleaved int16. Returns false on failure.
bool decode_vorbis(const unsigned char* data, std::size_t size, int* channels, int* sample_rate,
                   std::vector<std::int16_t>& samples);

}  // namespace vs::stb
