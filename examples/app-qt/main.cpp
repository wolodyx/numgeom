#include "QApplication"
#include "QLoggingCategory"

#include "numgeom/application.h"

#include "mainwindow.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char* argv[])
{
    Application caxApp(argc, argv);
    QApplication guiApp(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    MainWindow window(&caxApp);
    window.resize(1024, 768);
    window.show();

    return guiApp.exec();
}
