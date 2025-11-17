#ifndef numgeom_app_mainwindow_h
#define numgeom_app_mainwindow_h

#include <QMainWindow>

class VulkanWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    void createActions();

public slots:
    void onScreenshot();
    void onOpenFile();
    void onQuit();

private:
    VulkanWidget* m_sceneWidget;
};
#endif // !numgeom_app_mainwindow_h
