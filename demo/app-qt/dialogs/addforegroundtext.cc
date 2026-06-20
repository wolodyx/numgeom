#include "addforegroundtext.h"

#include "qboxlayout.h"
#include "qformlayout.h"
#include "qgroupbox.h"
#include "qlabel.h"
#include "qlineedit.h"
#include "qpushbutton.h"
#include "qspinbox.h"

Dialog_AddForegroundText::Dialog_AddForegroundText(QWidget* parent)
    : QDialog(parent) {
  setWindowTitle(tr("Add Foreground Text"));
  setMinimumWidth(320);

  auto* main_layout = new QVBoxLayout(this);

  // --- Text input ---
  auto* text_group = new QGroupBox(tr("Text"), this);
  auto* text_layout = new QVBoxLayout(text_group);
  text_edit_ = new QLineEdit(this);
  text_edit_->setText(tr("Hello, world!"));
  text_layout->addWidget(text_edit_);
  main_layout->addWidget(text_group);

  // --- Color input ---
  auto* color_group = new QGroupBox(tr("Color (RGB)"), this);
  auto* color_layout = new QFormLayout(color_group);

  color_r_ = new QSpinBox(this);
  color_r_->setRange(0, 255);
  color_r_->setValue(0);

  color_g_ = new QSpinBox(this);
  color_g_->setRange(0, 255);
  color_g_->setValue(0);

  color_b_ = new QSpinBox(this);
  color_b_->setRange(0, 255);
  color_b_->setValue(0);

  color_layout->addRow(tr("Red:"), color_r_);
  color_layout->addRow(tr("Green:"), color_g_);
  color_layout->addRow(tr("Blue:"), color_b_);
  main_layout->addWidget(color_group);

  // --- Position input ---
  auto* pos_group = new QGroupBox(tr("Position (screen coordinates)"), this);
  auto* pos_layout = new QFormLayout(pos_group);

  pos_x_ = new QSpinBox(this);
  pos_x_->setRange(0, 99999);
  pos_x_->setValue(5);

  pos_y_ = new QSpinBox(this);
  pos_y_->setRange(0, 99999);
  pos_y_->setValue(100);

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

std::string Dialog_AddForegroundText::GetText() const {
  return text_edit_->text().toStdString();
}

glm::vec3 Dialog_AddForegroundText::GetColor() const {
  return glm::vec3{static_cast<float>(color_r_->value()) / 255.0f,
                   static_cast<float>(color_g_->value()) / 255.0f,
                   static_cast<float>(color_b_->value()) / 255.0f};
}

glm::ivec2 Dialog_AddForegroundText::GetPosition() const {
  return glm::ivec2{pos_x_->value(), pos_y_->value()};
}
