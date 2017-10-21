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
    qDebug() << "ctor MainWindow";

    ui->setupUi(this);

    QDirIterator it(testImageDir, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
        QString filename = it.fileName();
        if(filename.length() > 5){
            ui->cmbTestImages->addItem(filename);
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
    gr->digitDilateKernelWidth = ui->spnDigitDilateKernelWidth->value();
    gr->digitDilateKernelHeight = ui->spnDigitDilateKernelHeight->value();

    gr->houghDistanceResolution = ui->spnFeatureExtractionHoughDistanceResolution->value();
    gr->houghAngleResolutionDegrees = ui->spnFeatureExtractionHoughAngleResolution->value();
    gr->houghVotesThreshold = ui->spnFeatureExtractionHoughVotesThreshold->value();
    gr->houghMaxLineGap = ui->spnFeatureExtractionHoughMaxLineGap->value();
    gr->houghMinLineLength = ui->spnFeatureExtractionHoughMinLineLength->value();
    gr->referenceImageDir = referenceImageDir.toStdString();

    //TODO: training not in main read proces, separate button action, store training data, load training data on start reader.
    bool blnKNNTrainingSuccessful =  gr->loadKNNDataAndTrainKNN();
    if (blnKNNTrainingSuccessful == false) {
        std::cout << std::endl << std::endl << "error: error: KNN traning was not successful" << std::endl << std::endl;
    }
}

void MainWindow::on_btnReadImageValue_clicked()
{
    QString filename = ui->cmbTestImages->currentText();
    QString path = QString(testImageDir + filename);
    Mat src = imread(path.toStdString(), CV_LOAD_IMAGE_COLOR);

    if(!src.data){
        ui->statusBar->showMessage(QString("could not open image " + path), 0);
        return;
    }
    //    imageAnalizer.resetNextWindowPosition();
    //    imageAnalizer.showImage("MainWindow: Original image", src);

    Mat enhanced, segmented;

    configGaugeReader();

    gaugeReader->EnhanceImage(src, enhanced);

    //TODO? na rotatiecorrectie: pre-roi voor contours

    gaugeReader->SegmentImage(enhanced, segmented);
    gaugeReader->ExtractFeatures(segmented, enhanced, src);

    //    imageAnalizer.resetNextWindowPosition();
    //imageAnalizer.showImage("MainWindow: Original image", src);
    //    imageAnalizer.showImage("MainWindow: Enhanced image", enhanced);
    //    imageAnalizer.showImage("MainWindow: Segmented image", segmented);
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

void MainWindow::on_spnDigitDilateKernelWidth_valueChanged()
{
    on_btnReadImageValue_clicked();
}

void MainWindow::on_spnDigitDilateKernelHeight_valueChanged()
{
    on_btnReadImageValue_clicked();
}
