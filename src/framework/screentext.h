#ifndef NUMGEOM_FRAMEWORK_SCREENTEXT_H
#define NUMGEOM_FRAMEWORK_SCREENTEXT_H

#include <string>
#include <vector>
#include <unordered_map>

#include "glm/glm.hpp"

class ScreenText {
 public:
  ScreenText();
  ScreenText(const std::string& text, const glm::ivec2& position);
  ScreenText(const std::string& text, const glm::ivec2& position,
             const glm::vec4& color, const std::string& font_path = "",
             int font_size = 32);
  ~ScreenText();

  bool operator!() const;
  bool IsEmpty() const;

  // Font and text management
  bool LoadFont(const std::string& font_path, int font_size);
  void SetText(const std::string& text);
  void SetPosition(const glm::ivec2& position);
  void SetColor(const glm::vec4& color);
  
  // Regenerate the glyph atlas and texture data
  void RegenerateAtlas();

  // Accessors
  const std::string& GetText() const { return text_; }
  const glm::ivec2& GetPosition() const { return position_; }
  const glm::vec4& GetColor() const { return color_; }
  const std::string& GetFontPath() const { return font_path_; }
  int GetFontSize() const { return font_size_; }
  
  // Texture data for GPU upload
  const std::vector<unsigned char>& GetAtlasPixels() const { return atlas_pixels_; }
  int GetAtlasWidth() const { return atlas_width_; }
  int GetAtlasHeight() const { return atlas_height_; }
  
  // Glyph information for rendering
  struct GlyphInfo {
    glm::vec2 uv_min;      // UV coordinates in atlas (normalized)
    glm::vec2 uv_max;
    glm::ivec2 size;       // Glyph size in pixels
    glm::ivec2 bearing;    // Offset from baseline to top-left of glyph
    int advance;           // Horizontal advance to next glyph (in 1/64 pixels)
  };
  
  const GlyphInfo* GetGlyphInfo(uint32_t codepoint) const;
  
  // Calculate text dimensions in pixels
  glm::ivec2 GetTextDimensions() const;

 private:
  std::string text_;
  glm::ivec2 position_;
  glm::vec4 color_;           // RGBA color
  std::string font_path_;     // Path to TTF font file
  int font_size_;             // Font size in pixels
  
  // Font data loaded from file
  std::vector<unsigned char> font_data_;
  
  // Glyph atlas data
  std::vector<unsigned char> atlas_pixels_;
  int atlas_width_;
  int atlas_height_;
  
  // Glyph cache
  std::unordered_map<uint32_t, GlyphInfo> glyph_cache_;
  
  // Private methods
  bool LoadFontData(const std::string& font_path);
  void CreateGlyphAtlas();
  void RasterizeGlyph(uint32_t codepoint, int x, int y);
};
#endif // !NUMGEOM_FRAMEWORK_SCREENTEXT_H
