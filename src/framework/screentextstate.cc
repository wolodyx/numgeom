#include "screentextstate.h"

#include <cassert>

#include "boost/log/trivial.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

static unsigned char default_font_data_[] = {
  #include "UbuntuSans[wdth,wght].ttf.h"
};

ScreenText::State::State() {}

void ScreenText::State::CreateGlyphAtlas() {
  const unsigned char* font_data_ptr = default_font_data_;
  if (!font_data_.empty())
    font_data_ptr = font_data_.data();

  // Initialize stb_truetype font
  stbtt_fontinfo font;
  if (!stbtt_InitFont(&font, font_data_ptr, 0)) {
    BOOST_LOG_TRIVIAL(trace) << "ScreenText::CreateGlyphAtlas: stbtt_InitFont failed";
    return;
  }

  // Calculate scale for desired font size
  float scale = stbtt_ScaleForPixelHeight(&font, font_size_);

  glyph_cache_.clear();

  // For simplicity, we'll create an atlas that contains only the ASCII characters in the text
  // First pass: calculate total dimensions needed
  int total_width = 0;
  int max_height = 0;
  // int ascent, descent, lineGap;
  // stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

  std::vector<std::pair<uint32_t, glm::ivec2>> glyph_sizes;

  for (char c : text_) {
    uint32_t codepoint = static_cast<uint32_t>(c);
    if (glyph_cache_.find(codepoint) != glyph_cache_.end()) {
      continue;  // Already processed
    }

    int glyph_index = stbtt_FindGlyphIndex(&font, codepoint);
    if (glyph_index == 0) {
      continue;  // Glyph not found
    }

    int advance, lsb;
    stbtt_GetGlyphHMetrics(&font, glyph_index, &advance, &lsb);

    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(&font, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

    int width = x1 - x0;
    int height = y1 - y0;

    if (width > 0 && height > 0) {
      glyph_sizes.emplace_back(codepoint, glm::ivec2(width, height));
      total_width += width + 1;  // +1 for padding
      max_height = std::max(max_height, height);
    }

    // Store glyph metrics
    GlyphInfo info;
    info.size = glm::ivec2(width, height);
    info.bearing = glm::ivec2(x0, -y0);  // y0 is negative for most glyphs
    info.advance = static_cast<int>(advance * scale * 64.0f);

    glyph_cache_[codepoint] = info;
  }

  if (glyph_sizes.empty()) {
    glyph_atlas_.size = glm::ivec2(0, 0);
    glyph_atlas_.pixels.clear();
    return;
  }

  // Simple atlas layout: horizontal strip
  glyph_atlas_.size = glm::ivec2(total_width, max_height);
  glyph_atlas_.pixels.resize(total_width * max_height, 0);

  // Second pass: rasterize glyphs into atlas
  int current_x = 0;
  for (const auto& [codepoint, size] : glyph_sizes) {
    int glyph_index = stbtt_FindGlyphIndex(&font, codepoint);

    // Rasterize glyph
    std::vector<unsigned char> bitmap(size.x * size.y);
    stbtt_MakeGlyphBitmap(&font, bitmap.data(), size.x, size.y, size.x,
                          scale, scale, glyph_index);

    // Copy to atlas
    for (int y = 0; y < size.y; ++y) {
      for (int x = 0; x < size.x; ++x) {
        int atlas_idx = (y * glyph_atlas_.size.x) + (current_x + x);
        int bitmap_idx = (y * size.x) + x;
        glyph_atlas_.pixels[atlas_idx] = bitmap[bitmap_idx];
      }
    }

    // Update UV coordinates in glyph cache
    GlyphInfo& info = glyph_cache_[codepoint];
    info.uv_min = glm::vec2(static_cast<float>(current_x) / glyph_atlas_.size.x,
                            0.0f);
    info.uv_max = glm::vec2(static_cast<float>(current_x + size.x) / glyph_atlas_.size.x,
                            static_cast<float>(size.y) / glyph_atlas_.size.y);

    current_x += size.x + 1;  // Move to next position with padding
  }
}
