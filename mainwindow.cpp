#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

using namespace cv;

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showImage(const char *winname, const Mat img, int x, int y)
{
    namedWindow(winname, CV_WINDOW_AUTOSIZE);
    moveWindow(winname, x, y);
    imshow(winname, img);
}

void MainWindow::showImage(const char *winname, const Mat img, int position)
{
    showImage(winname, img, (position % 4) * 200, (position / 4) * 100);
}
