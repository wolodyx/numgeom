#include "numgeom/fgtext.h"

#include <algorithm>
#include <fstream>
#include <format>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static unsigned char default_font_data_[] = {
  #include "UbuntuSans[wdth,wght].ttf.h"
};

//! Glyph information for rendering
struct GlyphInfo {
  glm::vec2 uv_min;      //!< UV coordinates in atlas (normalized)
  glm::vec2 uv_max;
  glm::ivec2 size;       //!< Glyph size in pixels
  glm::ivec2 bearing;    //!< Offset from baseline to top-left of glyph
  int advance;           //!< Horizontal advance to next glyph (in 1/64 pixels)
};

class FgTextImpl {
 public:
  std::string text;
  glm::vec4 color = glm::vec4(0.0, 0.0, 0.0, 1.0);
  int font_size = 32; //!< Font size in pixels
  std::vector<unsigned char> font_data; //!< Content of `*.ttf` font file.
  PixelBuffer pixel_buffer;
  std::unordered_map<uint32_t, GlyphInfo> glyph_cache;
};

FgText::FgText(const std::string& text, const glm::vec3& color) {
  impl_ = new FgTextImpl();
  impl_->text = text;
  RegeneratePixelBuffer();
}

FgText::~FgText() {
  delete impl_;
}

void FgText::SetText(const std::string& text) {
  if (impl_->text != text) {
    impl_->text = text;
    RegeneratePixelBuffer();
    SetDirty();
  }
}

void FgText::SetColor(const glm::vec3& color) {
  impl_->color.x = color.x;
  impl_->color.y = color.y;
  impl_->color.z = color.z;
  RegeneratePixelBuffer();
  SetDirty();
}

void FgText::SetOpacity(float opacity) {
  impl_->color.a = opacity;
  RegeneratePixelBuffer();
  SetDirty();
}

FgText* FgText::Make(const std::string& text, const glm::vec3& color) {
  if (text.empty())
    return nullptr;
  return new FgText(text, color);
}

const PixelBuffer& FgText::GetPixelBuffer() const {
  return impl_->pixel_buffer;
}

uint32_t FgText::GetWidth() const {
  return impl_->pixel_buffer.width;
}

uint32_t FgText::GetHeight() const {
  return impl_->pixel_buffer.height;
}

void FgText::RegeneratePixelBuffer() {
  // Use embedded font data if no custom font is loaded.
  const unsigned char* font_data = impl_->font_data.empty()
                                       ? default_font_data_
                                       : impl_->font_data.data();
  if (!font_data) {
    return;
  }

  // Initialize stb_truetype font.
  stbtt_fontinfo font_info;
  if (!stbtt_InitFont(&font_info, font_data, 0)) {
    return;
  }

  const std::string& text = impl_->text;
  if (text.empty()) {
    impl_->pixel_buffer.pixels.clear();
    impl_->pixel_buffer.width = 0;
    impl_->pixel_buffer.height = 0;
    SetDirty();
    return;
  }

  const int font_size = impl_->font_size;
  const float scale = stbtt_ScaleForPixelHeight(&font_info,
                                                static_cast<float>(font_size));

  // --- First pass: measure each glyph and compute total atlas width ---
  struct GlyphMetrics {
    int width;
    int height;
    int bearing_x;
    int bearing_y;
    int advance;
    int origin_x;  // x-origin in atlas (top-left of glyph bitmap)
  };
  std::vector<GlyphMetrics> metrics;
  metrics.reserve(text.size());

  int ascent = 0;
  int descent = 0;
  int line_gap = 0;
  stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);

  const int baseline_y = static_cast<int>((ascent * scale) + 0.5f);

  int atlas_x = 0;
  int atlas_height = 0;
  impl_->glyph_cache.clear();

  for (size_t i = 0; i < text.size(); ++i) {
    const uint32_t codepoint = static_cast<unsigned char>(text[i]);

    int advance_width = 0;
    int left_side_bearing = 0;
    stbtt_GetCodepointHMetrics(&font_info, codepoint, &advance_width,
                               &left_side_bearing);

    int glyph_width = 0;
    int glyph_height = 0;
    int offset_x = 0;
    int offset_y = 0;
    const unsigned char* glyph_bitmap = stbtt_GetCodepointBitmap(
        &font_info, scale, scale, codepoint, &glyph_width, &glyph_height,
        &offset_x, &offset_y);

    const int origin_x = atlas_x + offset_x + static_cast<int>(left_side_bearing * scale);
    const int origin_y = baseline_y + offset_y;

    metrics.push_back({glyph_width, glyph_height, offset_x, offset_y,
                       static_cast<int>(advance_width * scale + 0.5f),
                       origin_x});

    atlas_x += static_cast<int>(advance_width * scale + 0.5f) + 1;

    if (origin_y + glyph_height > atlas_height) {
      atlas_height = origin_y + glyph_height;
    }

    if (glyph_bitmap) {
      stbtt_FreeBitmap(const_cast<unsigned char*>(glyph_bitmap), nullptr);
    }
  }

  // Add some padding.
  atlas_x += 2;
  atlas_height += 2 + baseline_y;

  if (atlas_x <= 0 || atlas_height <= 0) {
    impl_->pixel_buffer.pixels.clear();
    impl_->pixel_buffer.width = 0;
    impl_->pixel_buffer.height = 0;
    SetDirty();
    return;
  }

  const uint32_t atlas_width = static_cast<uint32_t>(atlas_x);
  const uint32_t final_height = static_cast<uint32_t>(atlas_height);

  // --- Second pass: render glyphs into the atlas ---
  // VK_FORMAT_R8G8B8A8_UNORM: 4 bytes per pixel (R, G, B, A).
  std::vector<unsigned char> atlas_pixels(atlas_width * final_height * 4, 0);

  atlas_x = 0;
  for (size_t i = 0; i < text.size(); ++i) {
    const uint32_t codepoint = static_cast<unsigned char>(text[i]);

    int advance_width = 0;
    int left_side_bearing = 0;
    stbtt_GetCodepointHMetrics(&font_info, codepoint, &advance_width,
                               &left_side_bearing);

    int glyph_width = 0;
    int glyph_height = 0;
    int offset_x = 0;
    int offset_y = 0;
    const unsigned char* glyph_bitmap = stbtt_GetCodepointBitmap(
        &font_info, scale, scale, codepoint, &glyph_width, &glyph_height,
        &offset_x, &offset_y);

    const int origin_x = atlas_x + offset_x + static_cast<int>(left_side_bearing * scale);
    const int origin_y = baseline_y + offset_y;

    // Copy the glyph bitmap (single-channel alpha) into the RGBA atlas.
    if (glyph_bitmap && glyph_width > 0 && glyph_height > 0) {
      for (int gy = 0; gy < glyph_height; ++gy) {
        for (int gx = 0; gx < glyph_width; ++gx) {
          const int atlas_px = origin_x + gx;
          const int atlas_py = origin_y + gy;
          if (atlas_px >= 0 && atlas_px < static_cast<int>(atlas_width) &&
              atlas_py >= 0 && atlas_py < static_cast<int>(final_height)) {
            const unsigned char alpha =
                glyph_bitmap[gy * glyph_width + gx];
            const size_t pixel_idx =
                (static_cast<size_t>(atlas_py) * atlas_width + atlas_px) * 4;
            // Premultiply color with alpha for correct blending.
            atlas_pixels[pixel_idx + 0] = static_cast<unsigned char>(
                (impl_->color.x * 255.0f) * (alpha / 255.0f));
            atlas_pixels[pixel_idx + 1] = static_cast<unsigned char>(
                (impl_->color.y * 255.0f) * (alpha / 255.0f));
            atlas_pixels[pixel_idx + 2] = static_cast<unsigned char>(
                (impl_->color.z * 255.0f) * (alpha / 255.0f));
            atlas_pixels[pixel_idx + 3] = alpha;
          }
        }
      }
    }

    if (glyph_bitmap) {
      stbtt_FreeBitmap(const_cast<unsigned char*>(glyph_bitmap), nullptr);
    }

    // Cache glyph info with normalized UV coordinates.
    GlyphInfo& info = impl_->glyph_cache[codepoint];
    info.uv_min = glm::vec2(static_cast<float>(origin_x) / atlas_width,
                            static_cast<float>(origin_y) / final_height);
    info.uv_max = glm::vec2(
        static_cast<float>(origin_x + glyph_width) / atlas_width,
        static_cast<float>(origin_y + glyph_height) / final_height);
    info.size = glm::ivec2(glyph_width, glyph_height);
    info.bearing = glm::ivec2(offset_x, offset_y);
    info.advance = static_cast<int>(advance_width * scale + 0.5f);

    atlas_x += static_cast<int>(advance_width * scale + 0.5f) + 1;
  }

  // Store the result.
  impl_->pixel_buffer.width = atlas_width;
  impl_->pixel_buffer.height = final_height;
  impl_->pixel_buffer.pixels = std::move(atlas_pixels);
}
