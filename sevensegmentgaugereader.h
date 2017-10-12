#ifndef SEVENSEGMENTGAUGEREADER_H
#define SEVENSEGMENTGAUGEREADER_H

#include <cstdlib>
#include <QObject>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "igaugereader.h"
#include "imageanalizer.h"

using namespace cv;
using namespace std;

class SevenSegmentGaugeReader : public IGaugeReader
{
public:
    int adaptiveThresholdBlockSize = 35;
    int adaptivethresholdC = -25;
    int gaussianBlurSize = 3;

    int cannyThreshold1 = 200;
    int cannyThreshold2 = 200;
    int dilateKernelSize = 3;

    double houghDistanceResolution = 1;
    double houghAngleResolutionDegrees = 1;
    int houghVotesThreshold = 100;
    double houghMinLineLengthFactor = 0.15; // relative to image width
    double houghMaxLineGap = 20;

    SevenSegmentGaugeReader();
    void EnhanceImage(Mat src, OutputArray dst);
    void SegmentImage(Mat src, OutputArray dst);
    ImageObject* ExtractFeatures(Mat src);
    ReaderResult Classify(ImageObject* features);
    ReaderResult ReadGaugeImage(Mat src);

    //TODO: in separate class
    double median(vector<double> collection);
private:
    //TODO: verhouding van source image behouden
    const int X_RESOLUTION = 480;
    const int Y_RESOLUTION = 640;
    const Size IMG_SIZE = Size(X_RESOLUTION, Y_RESOLUTION);

    ImageAnalizer imageAnalizer;
};

#endif // SEVENSEGMENTGAUGEREADER_H
