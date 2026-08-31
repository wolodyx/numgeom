#include "QApplication"
#include "QLoggingCategory"

#include "numgeom/application.h"
#include "numgeom/workbench.h"

#include "mainwindow.h"

#ifndef NDEBUG
Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")
#endif

int main(int argc, char* argv[]) {
  QApplication guiApp(argc, argv);
  Application caxApp(argc, argv);

#ifdef NUMGEOM_HAS_OCC
  // Загружаем разделяемую библиотеку модуля OCC до создания workbench'ей,
  // чтобы выполнилась его саморегистрация (REGISTER_WORKBENCH). По одной лишь
  // ссылке библиотека не загружается, если из неё не вызывается ни одна
  // функция (MSVC не добавляет DLL в import table, ld с --as-needed отбрасывает
  // неиспользуемую библиотеку).
  Workbench::LoadModule(NUMGEOM_OCC_MODULE);
#endif

  QLocale::setDefault(QLocale::system());

  QCoreApplication::setOrganizationName("NumGeom");
  QCoreApplication::setApplicationName("app-qt");

#ifndef NDEBUG
  QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
#endif

MainWindow window(&caxApp);
  window.resize(1024, 768);
  window.show();
  window.initVulkan();

  return guiApp.exec();
}
