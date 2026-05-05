#include "logo.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Logo::Logo() : width(0), height(0) {}

Logo::Logo(const std::filesystem::path& image_path,
           const glm::ivec2& screen_position) {
  int channels;
  unsigned char* pixels = stbi_load(image_path.string().c_str(), &this->width,
                                    &this->height, &channels, 4);
  if (!pixels)
    return;
  int pixels_size = 4 * this->width * this->height;
  this->pixels.assign(pixels, pixels + pixels_size);
  stbi_image_free(pixels);
  this->position = screen_position;
}

Logo::Logo(const unsigned char* image_data,
           size_t image_data_size,
           const glm::ivec2& screen_position) {
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
  this->position = screen_position;
}

bool Logo::operator!() const { return pixels.empty(); }
