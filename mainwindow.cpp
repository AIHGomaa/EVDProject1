#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDirIterator"
#include "QDebug"
#include "QMessageBox"


//Plaats de foto map in dropbox ergens local om te verwerken
QString directory = "F:\\QT\\Project1\\afbeeldingen\\";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    QDirIterator it(directory, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
        QString filename = it.fileName();
        if(filename.length() > 5){
            ui->FotoCombobox->addItem(filename);
        }
    }
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

void MainWindow::on_FotoVerwerk_clicked()
{
    QString filename = ui->FotoCombobox->currentText();
    //QMessageBox::information(this, "Item Selection", filename);
    QString path = QString(directory + filename);
   // QMessageBox::information(this, "Path is", path);
    Mat src;
    src = imread(path.toStdString(), CV_LOAD_IMAGE_COLOR);
    if(!src.data){
        ui->statusBar->showMessage(QString("could not open image"), 0);
    }
    else
    {
        showImage("Original image", src, 200, 200);

    }
}
