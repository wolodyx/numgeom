#include "gizmowidget_filepath.h"

#include "qboxlayout.h"
#include "qfiledialog.h"
#include "qlineedit.h"
#include "qpushbutton.h"

#include "numgeom/gizmo_filepath.h"

class GizmoWidget_FilePath::Ui {
 public:
  Ui(QWidget*);
  QLineEdit* filename_edit;
  QPushButton* open_dialog_button;
};

GizmoWidget_FilePath::Ui::Ui(QWidget* parent) {
  filename_edit = new QLineEdit(parent);
  QIcon icon(":/resources/icons/open-folder.ico");
  open_dialog_button = new QPushButton(icon, QString(""), parent);
  auto ly = new QBoxLayout(QBoxLayout::LeftToRight, parent);
  ly->addWidget(filename_edit);
  ly->addWidget(open_dialog_button);
  parent->setLayout(ly);
}

GizmoWidget_FilePath::GizmoWidget_FilePath(Gizmo_FilePath* gizmo,
                                           QWidget* parent)
    : GizmoWidget(gizmo, parent) {
  ui_ = new Ui(this);
  connect(ui_->open_dialog_button, &QPushButton::clicked, this,
          &GizmoWidget_FilePath::onOpenDialog);
  connect(ui_->filename_edit, &QLineEdit::textChanged, this,
          &GizmoWidget_FilePath::onFilepathChanged);
}

GizmoWidget_FilePath::~GizmoWidget_FilePath() {
  delete ui_;
}

namespace {
QString ToQString(const std::filesystem::path& path) {
#if defined(Q_OS_WIN) || defined(_WIN32)
  // Windows native paths are UTF-16/wchar_t
  return QString::fromStdWString(path.wstring());
#else
  // POSIX paths are typically UTF-8
  #if __cplusplus >= 202002L
    // C++20 u8string() returns std::u8string
    auto u8Str = path.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(u8Str.data()), u8Str.length());
  #else
    // C++17 u8string() returns std::string
    return QString::fromUtf8(path.u8string());
  #endif
#endif
}

std::filesystem::path ToStdPath(const QString& path) {
  return std::filesystem::path(path.toStdU16String());
}
}

void GizmoWidget_FilePath::onOpenDialog() {
  Gizmo_FilePath* gizmo = dynamic_cast<Gizmo_FilePath*>(this->GetGizmo());
  QString filter = QString::fromStdString(gizmo->GetFilter());
  QString dir = ToQString(gizmo->GetCurrentDir());
  QString fileName;
  if(gizmo->IsOpenMode()) {
    fileName = QFileDialog::getOpenFileName(this, tr("Open file"), dir, filter);
  } else {
    fileName = QFileDialog::getSaveFileName(this, tr("Save file"), dir, filter);
  }
  std::filesystem::path path = ToStdPath(fileName);
  gizmo->SetCurrentDir(path.parent_path());
  gizmo->SetFilePath(path);
  ui_->filename_edit->setText(fileName);
}

void GizmoWidget_FilePath::onFilepathChanged(const QString& filepath) {
  Gizmo_FilePath* gizmo = dynamic_cast<Gizmo_FilePath*>(this->GetGizmo());
  gizmo->SetFilePath(ToStdPath(filepath));
}

void GizmoWidget_FilePath::UpdateContent() {
  Gizmo_FilePath* gizmo = dynamic_cast<Gizmo_FilePath*>(this->GetGizmo());
  ui_->filename_edit->setText(ToQString(gizmo->GetFilePath()));
}
