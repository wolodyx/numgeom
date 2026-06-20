#ifndef NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDTEXT
#define NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDTEXT

#include <string>

#include "qdialog.h"

#include "glm/glm.hpp"

class QLineEdit;
class QSpinBox;

class Dialog_AddForegroundText : public QDialog {
  Q_OBJECT

 public:
  explicit Dialog_AddForegroundText(QWidget* parent = nullptr);

  std::string GetText() const;
  glm::vec3 GetColor() const;
  glm::ivec2 GetPosition() const;

 private:
  QLineEdit* text_edit_;
  QSpinBox* color_r_;
  QSpinBox* color_g_;
  QSpinBox* color_b_;
  QSpinBox* pos_x_;
  QSpinBox* pos_y_;
};

#endif // !NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDTEXT
