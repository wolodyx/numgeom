#include "fgimage.h"

#include <fstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FgImage* FgImage::Make(const std::filesystem::path& image_path) {
  int channels, width, height;
  unsigned char* pixels = stbi_load(image_path.string().c_str(), &width,
                                    &height, &channels, 4);
  if (!pixels)
    return nullptr;
  PixelBuffer pixel_buffer;
  pixel_buffer.width = static_cast<uint32_t>(width);
  pixel_buffer.height = static_cast<uint32_t>(height);
  int pixels_size = 4 * width * height;
  pixel_buffer.pixels.assign(pixels, pixels + pixels_size);
  stbi_image_free(pixels);
  return new FgImage(std::move(pixel_buffer));
}

FgImage* FgImage::Make(const unsigned char* image_data,
                       size_t image_data_size) {
  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(
      image_data, static_cast<int>(image_data_size), &width, &height, &channels,
      4);
  if (!pixels)
    return nullptr;
  PixelBuffer pixel_buffer;
  pixel_buffer.width = static_cast<uint32_t>(width);
  pixel_buffer.height = static_cast<uint32_t>(height);
  int pixels_size = 4 * width * height;
  pixel_buffer.pixels.assign(pixels, pixels + pixels_size);
  stbi_image_free(pixels);
  return new FgImage(std::move(pixel_buffer));
}

FgImage::FgImage(PixelBuffer&& pixel_buffer) : pixel_buffer_(pixel_buffer) {
}

const PixelBuffer& FgImage::GetPixelBuffer() const {
  return pixel_buffer_;
}

uint32_t FgImage::GetWidth() const { return pixel_buffer_.width; }

uint32_t FgImage::GetHeight() const { return pixel_buffer_.height; }
