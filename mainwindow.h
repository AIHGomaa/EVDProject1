#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "igaugereader.h"
#include "sevensegmentgaugereader.h"
#include "imageanalizer.h"

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

private:
    Ui::MainWindow *ui;
    // image folders in project working directory
    QString referenceImageDir = "referenceImages\\";
    QString testImageDir = "testImages\\";
    IGaugeReader *gaugeReader;
    ImageAnalizer imageAnalizer;

    void configGaugeReader();
};

#endif // MAINWINDOW_H
