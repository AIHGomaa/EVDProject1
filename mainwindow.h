#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <mutex>    // lock
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "igaugereader.h"
#include "sevensegmentgaugereader.h"

namespace Ui {
class MainWindow;
}

using namespace cv;
using namespace std;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnReadImageValue_clicked();

    void on_spnEnhancementadaptiveThresholdBlockSize_valueChanged();

    void on_spnEnhancementadaptivethresholdC_valueChanged();

    void on_spnEnhancementGaussianKernelSize_valueChanged();

    void on_spnSegmentationCannyThreshold1_valueChanged();

    void on_spnSegmentationCannyThreshold2_valueChanged();

    void on_spnSegmentationDilateKernelSize_valueChanged();

    void on_spnFeatureExtractionHoughDistanceResolution_valueChanged();

    void on_spnFeatureExtractionHoughAngleResolution_valueChanged();

    void on_spnFeatureExtractionHoughVotesThreshold_valueChanged();

    void on_spnFeatureExtractionHoughMaxLineGap_valueChanged();

    void on_spnFeatureExtractionHoughMinLineLength_valueChanged();

    void on_spnSegmentationCannyAppertureSize_valueChanged();

    void on_spnDigitDilateKernelWidth_valueChanged();

    void on_spnDigitDilateKernelHeight_valueChanged();

    void on_spnTemplateMatchThreshold_valueChanged();

    void on_btnReadCameraImage_clicked();

    void on_chkShowMainImages_clicked();

    void on_chkShowEnhancementImages_clicked();

    void on_chkShowSegmentationImages_clicked();

    void on_chkShowFeatureExtractionImages_clicked();

    void on_chkShowFeatureExtractReferenceDigitImages_clicked();

    void on_chkShowClassificationImages_clicked();

    void on_chkShowFeatureExtractKnnTraining_clicked();

    void on_chkShowFeatureExtractRotationCorrection_clicked();

    void on_chkShowAllContours_clicked();

private:
    Ui::MainWindow *ui;
    // image folders in project working directory
    QString referenceImageDir = "referenceImages\\";
    QString testImageDir = "testImages\\";
    IGaugeReader *gaugeReader;
    Mat srcImage;
    bool cameraRunning = false;
    mutex camRunningMutex;

    void configGaugeReader();
    void processImage();
};

#endif // MAINWINDOW_H
