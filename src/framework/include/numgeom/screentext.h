#ifndef NUMGEOM_FRAMEWORK_SCREENTEXT_H
#define NUMGEOM_FRAMEWORK_SCREENTEXT_H

#include <string>
#include <vector>
#include <unordered_map>

#include "glm/glm.hpp"

/** \class ScreenText
\brief Экранный текст -- текст, отрисовываемый на переднем плане сцены.

С экранным текстом связаны атрибуты: шрифт, размер, позиция на экране, цвет,
прозрачность.
*/
class ScreenText {
 public:
  ScreenText(const std::string& text, const glm::ivec2& position);
  ~ScreenText();

  void SetText(const std::string& text);
  void SetPosition(const glm::ivec2& position);
  void SetColor(const glm::vec3& color);
  void SetOpacity(float opacity);

  //! \brief An extended interface for internal use.
  //! \{
  class Inner;
  Inner* GetInnerInterface();
  const Inner* GetInnerInterface() const;
  //! \}

 private:
  class State;
  State* impl_;

  bool LoadFontData(const std::string& font_path);
};
#endif // !NUMGEOM_FRAMEWORK_SCREENTEXT_H
