#include "QApplication"
#include "QLoggingCategory"

#include "mainwindow.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    MainWindow window;
    window.resize(1024, 768);
    window.show();

    return app.exec();
}
