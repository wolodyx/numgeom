#ifndef numgeom_app_mainwindow_h
#define numgeom_app_mainwindow_h

#include <QMainWindow>

class Application;
class VulkanWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

    MainWindow(Application* app);

    ~MainWindow();


private:

    void createActions();


public slots:
    void onScreenshot();
    void onOpenFile();
    void onQuit();
    void onFitScreen();


private:
    VulkanWidget* m_sceneWidget;
    Application* m_app;
};
#endif // !numgeom_app_mainwindow_h
