#ifndef numgeom_app_scenewindow_h
#define numgeom_app_scenewindow_h

#include "qvulkanwindow.h"

#include "numgeom/trimesh.h"

class Application;
class UserInputController;
class VulkanWindowRenderer;


/** \class SceneWindow
\brief Окно графического приложения, в котором отображается сцена.
*/
class SceneWindow : public QVulkanWindow
{
public:
    SceneWindow(Application*);
    ~SceneWindow();
    QVulkanWindowRenderer* createRenderer() override;

    void updateGeometry(CTriMesh::Ptr);

private:
    void keyPressEvent(QKeyEvent*) override;      //!< Событие о нажатии клавиши.
    void keyReleaseEvent(QKeyEvent*) override;    //!< Событие об отжатии клавиши.
    void mousePressEvent(QMouseEvent*) override;  //!< Событие о нажатии кнопки мыши.
    void mouseReleaseEvent(QMouseEvent*) override;//!< Событие об отжатии кнопки мыши.
    void mouseMoveEvent(QMouseEvent*) override;   //!< Событие о перемещении указателя мыши.
    void wheelEvent(QWheelEvent*) override;       //!< Событие о прокрутке колесика мыши.

private:
    Application* m_app;
    UserInputController* m_userInputController;
    VulkanWindowRenderer* m_renderer = nullptr;
};
#endif // !numgeom_app_scenewindow_h
