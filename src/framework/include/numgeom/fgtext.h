#ifndef NUMGEOM_FRAMEWORK_FGTEXT_H
#define NUMGEOM_FRAMEWORK_FGTEXT_H

#include <string>
#include <vector>
#include <unordered_map>

#include "glm/glm.hpp"

#include "numgeom/fgobject.h"

class FgTextImpl;
class GlyphInfo;

/** \class FgText
\brief Экранный текст -- текст, отрисовываемый на переднем плане сцены.

С экранным текстом связаны атрибуты: шрифт, размер, цвет, прозрачность.
*/
class FgText : public FgObject {
 public:
  static FgText* Make(const std::string& text,
                      const glm::vec3& color = glm::vec3(0.0f));

 public:
  virtual ~FgText();

  const PixelBuffer& GetPixelBuffer() const override;
  uint32_t GetWidth() const override;
  uint32_t GetHeight() const override;

  void SetText(const std::string& text);
  void SetColor(const glm::vec3& color);
  void SetOpacity(float opacity);

 private:
  FgText(const std::string&, const glm::vec3&);
  void RegeneratePixelBuffer();

 private:
  FgTextImpl* impl_;
};
#endif // !NUMGEOM_FRAMEWORK_FGTEXT_H
