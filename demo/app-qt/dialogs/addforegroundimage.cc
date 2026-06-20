#include "addforegroundimage.h"

#include "qboxlayout.h"
#include "qfiledialog.h"
#include "qformlayout.h"
#include "qgroupbox.h"
#include "qlineedit.h"
#include "qpushbutton.h"
#include "qsettings.h"
#include "qspinbox.h"

Dialog_AddForegroundImage::Dialog_AddForegroundImage(
    QWidget* parent,
    QSettings& settings) : QDialog(parent), settings_(settings) {
  setWindowTitle(tr("Add Foreground Image"));
  setMinimumWidth(400);

  auto* main_layout = new QVBoxLayout(this);

  // --- File path input ---
  auto* file_group = new QGroupBox(tr("Image file"), this);
  auto* file_layout = new QHBoxLayout(file_group);
  file_path_edit_ = new QLineEdit(this);
  file_path_edit_->setPlaceholderText(tr("Select an image file..."));
  auto* browse_button = new QPushButton(tr("Browse..."), this);
  file_layout->addWidget(file_path_edit_);
  file_layout->addWidget(browse_button);
  main_layout->addWidget(file_group);

  connect(browse_button, &QPushButton::clicked, this, &Dialog_AddForegroundImage::onBrowse);

  // --- Position input ---
  auto* pos_group = new QGroupBox(tr("Position (screen coordinates)"), this);
  auto* pos_layout = new QFormLayout(pos_group);

  pos_x_ = new QSpinBox(this);
  pos_x_->setRange(0, 99999);
  pos_x_->setValue(5);

  pos_y_ = new QSpinBox(this);
  pos_y_->setRange(0, 99999);
  pos_y_->setValue(5);

  pos_layout->addRow(tr("X:"), pos_x_);
  pos_layout->addRow(tr("Y:"), pos_y_);
  main_layout->addWidget(pos_group);

  // --- Buttons ---
  auto* button_layout = new QHBoxLayout();
  auto* ok_button = new QPushButton(tr("OK"), this);
  auto* cancel_button = new QPushButton(tr("Cancel"), this);
  button_layout->addStretch();
  button_layout->addWidget(ok_button);
  button_layout->addWidget(cancel_button);
  main_layout->addLayout(button_layout);

  connect(ok_button, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancel_button, &QPushButton::clicked, this, &QDialog::reject);

  adjustSize();
}

Dialog_AddForegroundImage::~Dialog_AddForegroundImage() {
  QString last_directory = file_path_edit_->text();
  if (!last_directory.isEmpty()) {
    settings_.setValue("MainWindow/lastFgImageDirectory",
                        QFileInfo(last_directory).absolutePath());
  }
}

void Dialog_AddForegroundImage::onBrowse() {
  QString last_directory = settings_.value("MainWindow/lastFgImageDirectory",
                                           QDir::homePath()).toString();
  QString filename = QFileDialog::getOpenFileName(
      this, tr("Select foreground image file"),
      file_path_edit_->text().isEmpty() ? last_directory : file_path_edit_->text());
  if (!filename.isEmpty()) {
    file_path_edit_->setText(filename);
  }
}

std::string Dialog_AddForegroundImage::GetFilePath() const {
  return file_path_edit_->text().toStdString();
}

glm::ivec2 Dialog_AddForegroundImage::GetPosition() const {
  return glm::ivec2{pos_x_->value(), pos_y_->value()};
}
