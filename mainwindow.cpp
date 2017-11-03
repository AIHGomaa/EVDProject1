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

    gr->templateMatchThreshold = ui->spnTemplateMatchThreshold->value();

    gr->initialize();
}

void MainWindow::processImage()
{
    if(!srcImage.data) {
        return;
    }

    Mat enhanced, segmented, srcScaled;

    configGaugeReader();

    gaugeReader->EnhanceImage(srcImage, enhanced, srcScaled);
    gaugeReader->SegmentImage(enhanced, segmented);
    gaugeReader->ExtractFeatures(segmented, enhanced, srcScaled);

    if (ui->chkShowMainImages->checkState())
    {
        imageAnalizer.resetNextWindowPosition();
        imageAnalizer.showImage("MainWindow: Original image", srcImage);
        imageAnalizer.showImage("MainWindow srcScaled", srcScaled);
        imageAnalizer.showImage("MainWindow: Enhanced image", enhanced);
        //        imageAnalizer.showImage("MainWindow: Enhanced after feature extract", enhancedAfterFeatureExtract);
        imageAnalizer.showImage("MainWindow: Segmented image", segmented);
        //        imageAnalizer.showImage("MainWindow: Segmented image", dst);
    }
}

void MainWindow::on_btnReadImageValue_clicked()
{
    auto childList = findChildren<QMainWindow*>();
    for (auto child : childList)
    {
        child->close();
    }

    QString filename = ui->cmbTestImages->currentText();
    QString path = QString(testImageDir + filename);
    srcImage = imread(path.toStdString(), CV_LOAD_IMAGE_COLOR);

    if(!srcImage.data){
        ui->statusBar->showMessage(QString("could not open image " + path), 0);
        return;
    }

    // Simulate camera image roi
    int blurredRowHeight = (int)(srcImage.rows / 3.5 + 0.5);
    Mat blurredTopRange = srcImage(Range(0, blurredRowHeight), Range::all());
    Mat blurredBottomRange = srcImage(Range(srcImage.rows - blurredRowHeight, srcImage.rows-1), Range::all());
    blur(blurredTopRange, blurredTopRange, Size(25, 25), Point(-1, -1), BORDER_DEFAULT);
    blur(blurredBottomRange, blurredBottomRange, Size(25, 25), Point(-1, -1), BORDER_DEFAULT);

   processImage();
}

void MainWindow::on_btnReadCameraImage_clicked()
{
    QString info;
    VideoCapture vc;
    Mat rawCameraInput(400, 300, CV_8SC4);
    Mat cameraImage(400, 300, CV_8SC4), blurredTopRange, blurredBottomRange;

    imageAnalizer.showImage("Camera input", cameraImage, 0, 0);

    vc.open(0);
    if (!vc.isOpened()) {
        info.append("Could not take a snapshot, probably no camera connected.");
        ui->statusBar->showMessage(info, 0);
        return;
    }

    vc >> rawCameraInput;

    int cameraImageCols = rawCameraInput.cols < rawCameraInput.rows ?
                rawCameraInput.cols : (int)(rawCameraInput.rows * 3 / 4.0 + 0.5);
    int cameraImageLeft = rawCameraInput.cols < rawCameraInput.rows ?
                0 : (rawCameraInput.cols - cameraImageCols) / 2.0 + 0.5;

    Rect cameraImageRoiRect(cameraImageLeft, 0, cameraImageCols, rawCameraInput.rows);
    int blurredRowHeight = (int)(cameraImage.rows / 3.5 + 0.5);

    cameraImage = rawCameraInput(cameraImageRoiRect);
    blurredTopRange = cameraImage(Range(0, blurredRowHeight), Range::all());
    blurredBottomRange = cameraImage(Range(cameraImage.rows - blurredRowHeight, cameraImage.rows-1), Range::all());

    Mat cameraPreview;
    do {
        blur(blurredTopRange, blurredTopRange, Size(25, 25), Point(-1, -1), BORDER_DEFAULT);
        blur(blurredBottomRange, blurredBottomRange, Size(25, 25), Point(-1, -1), BORDER_DEFAULT);

        cameraImage.copyTo(cameraPreview);
        line(cameraPreview, Point(0, blurredRowHeight), Point(cameraPreview.cols - 1, blurredRowHeight), Scalar(0, 255, 0), 1);
        line(cameraPreview, Point(0, cameraPreview.rows - blurredRowHeight), Point(cameraPreview.cols - 1, cameraPreview.rows - blurredRowHeight), Scalar(0, 255, 0), 1);
        imageAnalizer.showImage("Camera input", cameraPreview, 0, 0);

        int key = waitKey(200);
        if (key == 13 || key == 27)
            break;

        vc >> rawCameraInput;
        cameraImage = rawCameraInput(cameraImageRoiRect);
    } while(true);

    vc.release();
    destroyWindow("Camera input");

    if (!cameraImage.data) {
        info.append(QString("Snapshot taken but could not be converted to image."));
        ui->statusBar->showMessage(info, 0);
        return;
    }
    cameraImage.copyTo(srcImage);

    processImage();
}

void MainWindow::on_spnEnhancementadaptiveThresholdBlockSize_valueChanged()
{
    processImage();
}

void MainWindow::on_spnEnhancementadaptivethresholdC_valueChanged()
{
    processImage();
}

void MainWindow::on_spnEnhancementGaussianKernelSize_valueChanged()
{
    processImage();
}

void MainWindow::on_spnSegmentationCannyThreshold1_valueChanged()
{
    processImage();
}

void MainWindow::on_spnSegmentationCannyThreshold2_valueChanged()
{
    processImage();
}

void MainWindow::on_spnSegmentationDilateKernelSize_valueChanged()
{
    processImage();
}

void MainWindow::on_spnFeatureExtractionHoughDistanceResolution_valueChanged()
{
    processImage();
}

void MainWindow::on_spnFeatureExtractionHoughAngleResolution_valueChanged()
{
    processImage();
}

void MainWindow::on_spnFeatureExtractionHoughVotesThreshold_valueChanged()
{
    processImage();
}

void MainWindow::on_spnFeatureExtractionHoughMaxLineGap_valueChanged()
{
    processImage();
}

void MainWindow::on_spnFeatureExtractionHoughMinLineLength_valueChanged()
{
    processImage();
}

void MainWindow::on_spnSegmentationCannyAppertureSize_valueChanged()
{
    processImage();
}

void MainWindow::on_spnDigitDilateKernelWidth_valueChanged()
{
    processImage();
}

void MainWindow::on_spnDigitDilateKernelHeight_valueChanged()
{
    processImage();
}

void MainWindow::on_spnTemplateMatchThreshold_valueChanged()
{
    processImage();
}

