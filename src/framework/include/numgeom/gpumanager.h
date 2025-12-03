#ifndef numgeom_framework_gpumanager_h
#define numgeom_framework_gpumanager_h

class QWindow;

class Application;


/**
Менеджер GPU связывает состояние приложения (Application) с GPU и руководит
отрисовкой данных в окне графического приложения.
*/
class GpuManager
{
public:

    GpuManager(Application*, QWindow*);

    ~GpuManager();

    //! Запрос на обновление изображения в окне приложения.
    bool update();

private:
    bool initialize();

private:
    GpuManager(const GpuManager&) = delete;
    GpuManager& operator=(const GpuManager&) = delete;

private:
    struct Impl;
    Impl* m_pimpl;
};
#endif // !numgeom_framework_gpumanager_h
