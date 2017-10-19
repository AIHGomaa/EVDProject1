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
    int cannyAppertureSize = 3;
    int dilateKernelSize = 3;
    string referenceImageDir;

    double houghDistanceResolution = 1;
    double houghAngleResolutionDegrees = 1;
    int houghVotesThreshold = 100;
    double houghMinLineLength = 0.15; // relative to image width
    double houghMaxLineGap = 20;
    Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();

    SevenSegmentGaugeReader();
    void EnhanceImage(Mat src, OutputArray dst);
    void SegmentImage(Mat src, OutputArray dst);
    ImageObject* ExtractFeatures(Mat src, Mat srcOriginal);
    ReaderResult Classify(ImageObject* features);
    ReaderResult ReadGaugeImage(Mat src);
    bool loadKNNDataAndTrainKNN();

    //TODO: in separate class
    double median(vector<double> collection);
private:
    //TODO: keep source image aspect ratio
    const int X_RESOLUTION = 480;
    const int Y_RESOLUTION = 640;
    const Size IMG_SIZE = Size(X_RESOLUTION, Y_RESOLUTION);

    ImageAnalizer imageAnalizer;
    Size calculateDigitSize(Mat src);
    Mat berekenDigitAlgorithm(Mat src);
    vector<Point2d> getPoint(Point2d p1 , Point2d p2);
    Mat loadReferenceImage(string fileName);
};

#endif // SEVENSEGMENTGAUGEREADER_H
