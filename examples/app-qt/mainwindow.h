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

    void initVulkan();


private:
    void createActions();

public slots:
    void onScreenshot();
    void onOpenFile();
    void onQuit();
    void onFitScene();

private:
    SceneWindow* m_sceneWindow;
    Application* m_app;
    QVulkanInstance m_vulkanInstance;
};
#endif // !numgeom_app_mainwindow_h
