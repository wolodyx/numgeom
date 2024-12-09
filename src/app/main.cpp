#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char** argv)
{
    //Q_INIT_RESOURCE(application);
#ifdef Q_OS_ANDROID
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("numgeom");
    QCoreApplication::setApplicationName("NumGeom App");
    QCoreApplication::setApplicationVersion("0.0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    MainWindow mainWin;
    mainWin.resize(900, 600);
    mainWin.show();
    return app.exec();
}
