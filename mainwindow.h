#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    void showImage(const char *winname, const Mat tex, int x, int y);
    void showImage(const char *winname, const Mat tex, int position);
};

#endif // MAINWINDOW_H
