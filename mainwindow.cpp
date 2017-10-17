#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDirIterator"
#include "QDebug"
#include "QMessageBox"
#include "sevensegmentgaugereader.h"

using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    QDirIterator it(testImageDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
        QString filename = it.fileName();
        if(filename.length() > 5){
            ui->FotoCombobox->addItem(filename);
        }
    }

    gaugeReader = new SevenSegmentGaugeReader();
    imageAnalizer = ImageAnalizer();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::configGaugeReader()
{
    SevenSegmentGaugeReader* gr = ((SevenSegmentGaugeReader*)gaugeReader);
    gr->adaptiveThresholdBlockSize = ui->spnEnhancementadaptiveThresholdBlockSize->value();
    gr->adaptivethresholdC = ui->spnEnhancementadaptivethresholdC->value();
    gr->gaussianBlurSize = ui->spnEnhancementGaussianKernelSize->value();

    gr->cannyThreshold1 = ui->spnSegmentationCannyThreshold1->value();
    gr->cannyThreshold2 = ui->spnSegmentationCannyThreshold2->value();
    gr->cannyAppertureSize = ui->spnSegmentationCannyAppertureSize->value();
    gr->dilateKernelSize = ui->spnSegmentationDilateKernelSize->value();

    gr->houghDistanceResolution = ui->spnFeatureExtractionHoughDistanceResolution->value();
    gr->houghAngleResolutionDegrees = ui->spnFeatureExtractionHoughAngleResolution->value();
    gr->houghVotesThreshold = ui->spnFeatureExtractionHoughVotesThreshold->value();
    gr->houghMaxLineGap = ui->spnFeatureExtractionHoughMaxLineGap->value();
    gr->houghMinLineLength = ui->spnFeatureExtractionHoughMinLineLength->value();
    gr->referenceImageDir = referenceImageDir.toStdString();

    bool blnKNNTrainingSuccessful =  gr->loadKNNDataAndTrainKNN();
    if (blnKNNTrainingSuccessful == false) {
        std::cout << std::endl << std::endl << "error: error: KNN traning was not successful" << std::endl << std::endl;
    }
}

void MainWindow::on_btnReadImageValue_clicked()
{
    QString filename = ui->FotoCombobox->currentText();
    QString path = QString(testImageDir + filename);
    Mat src = imread(path.toStdString(), CV_LOAD_IMAGE_COLOR);

    if(!src.data){
        ui->statusBar->showMessage(QString("could not open image " + path), 0);
        return;
    }

    Size imgSize(src.cols, src.rows);
    Mat enhanced(imgSize, CV_8UC1);
    Mat segmented(imgSize, CV_8UC3);

    configGaugeReader();

    gaugeReader->EnhanceImage(src, enhanced);

    gaugeReader->SegmentImage(enhanced, segmented);
    gaugeReader->ExtractFeatures(segmented, src);

    imageAnalizer.resetNextWindowPosition();
    //imageAnalizer.showImage("MainWindow: Original image", src);
    imageAnalizer.showImage("MainWindow: Enhanced image", enhanced);
    imageAnalizer.showImage("MainWindow: Segmented image", segmented);
}

void MainWindow::on_spnEnhancementadaptiveThresholdBlockSize_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnEnhancementadaptivethresholdC_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnEnhancementGaussianKernelSize_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnSegmentationCannyThreshold1_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnSegmentationCannyThreshold2_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnSegmentationDilateKernelSize_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnFeatureExtractionHoughDistanceResolution_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnFeatureExtractionHoughAngleResolution_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnFeatureExtractionHoughVotesThreshold_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnFeatureExtractionHoughMaxLineGap_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnFeatureExtractionHoughMinLineLength_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnSegmentationCannyAppertureSize_valueChanged()
{
    on_btnReadImageValue_clicked();
}
