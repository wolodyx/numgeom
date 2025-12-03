#ifndef numgeom_app_mainwindow_h
#define numgeom_app_mainwindow_h

#include "qmainwindow.h"
#include "qvulkaninstance.h"

class Application;
class SceneWindow;


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
    QVulkanInstance m_vulkanInstance;
    SceneWindow* m_sceneWindow;
    Application* m_app;
};
#endif // !numgeom_app_mainwindow_h
