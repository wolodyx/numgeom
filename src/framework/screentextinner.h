#ifndef NUMGEOM_FRAMEWORK_SCREENTEXTINNER_H
#define NUMGEOM_FRAMEWORK_SCREENTEXTINNER_H

#include <vector>

#include "numgeom/screentext.h"

struct GlyphInfo;

class ScreenText::Inner {
 public:
  Inner(State*);
  const std::string& GetText() const;
  glm::ivec2 GetPosition() const;
  glm::vec4 GetColor() const;
  int GetFontSize() const;
  const std::vector<unsigned char>& GetAtlasPixels() const;
  glm::ivec2 GetAtlasSize() const;
  const GlyphInfo* GetGlyphInfo(uint32_t codepoint) const;

 private:
  State* impl_;
};

#endif // !NUMGEOM_FRAMEWORK_SCREENTEXTINNER_H
