#ifndef numgeom_app_mainwindow_h
#define numgeom_app_mainwindow_h

#include <QMainWindow>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

private:
    void createActions();
};
#endif // !numgeom_app_mainwindow_h
