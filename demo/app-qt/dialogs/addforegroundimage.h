#ifndef NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDIMAGE_H
#define NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDIMAGE_H

#include <string>

#include "qdialog.h"

#include "glm/glm.hpp"

class QLineEdit;
class QSettings;
class QSpinBox;

class Dialog_AddForegroundImage : public QDialog {
  Q_OBJECT

 public:
  Dialog_AddForegroundImage(QWidget* parent, QSettings& settings);
  ~Dialog_AddForegroundImage();

  std::string GetFilePath() const;
  glm::ivec2 GetPosition() const;

 private slots:
  void onBrowse();

 private:
  QSettings& settings_;
  QLineEdit* file_path_edit_;
  QSpinBox* pos_x_;
  QSpinBox* pos_y_;
};

#endif  // !NUMGEOM_FRAMEWORK_DIALOGS_ADDFOREGROUNDIMAGE_H