#include "fgimage.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

FgImage::FgImage(const std::filesystem::path& image_path) {
  int channels;
  unsigned char* pixels = stbi_load(image_path.string().c_str(), &this->width,
                                    &this->height, &channels, 4);
  if (!pixels)
    return;
  int pixels_size = 4 * this->width * this->height;
  this->pixels.assign(pixels, pixels + pixels_size);
  stbi_image_free(pixels);
}

FgImage::FgImage(const unsigned char* image_data, size_t image_data_size) {
  int width, height, channels;
  unsigned char* pixels = stbi_load_from_memory(
      image_data, static_cast<int>(image_data_size), &width, &height, &channels,
      4);
  if (!pixels)
    return;
  int pixels_size = width * height * 4;
  this->pixels.assign(pixels, pixels + pixels_size);
  this->width = width;
  this->height = height;
  stbi_image_free(pixels);
}

bool FgImage::operator!() const { return pixels.empty(); }
