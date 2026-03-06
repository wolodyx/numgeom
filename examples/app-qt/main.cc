#include "QApplication"
#include "QLoggingCategory"

#include "numgeom/application.h"

#include "mainwindow.h"

#ifndef NDEBUG
Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")
#endif

int main(int argc, char* argv[]) {
  Application caxApp(argc, argv);
  QApplication guiApp(argc, argv);

#ifndef NDEBUG
  QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));
#endif

MainWindow window(&caxApp);
  window.resize(1024, 768);
  window.show();
  window.initVulkan();

  return guiApp.exec();
}
