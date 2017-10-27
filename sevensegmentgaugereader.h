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

    int cannyThreshold1 = 10;
    int cannyThreshold2 = 100;
    int cannyAppertureSize = 3;
    int dilateKernelSize = 3;

    string referenceImageDir;

    double houghDistanceResolution = 1;
    double houghAngleResolutionDegrees = 1;
    int houghVotesThreshold = 100;
    double houghMinLineLength = 0.15; // relative to image width
    double houghMaxLineGap = 20;

    int digitDilateKernelWidth = 1;
    int digitDilateKernelHeight = 5;

    //TODO: configure in UI
    int templateMatchCannyThreshold1 = 50;
    int templateMatchCannyThreshold2 = 50 * 4;
    bool templayteMatchByEdges = true;
    int templateMatchMethod = TM_CCOEFF;   // TM_SQDIFF_NORMED, TM_SQDIFF

    double templateMatchThreshold = 0.95;

    Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();

    SevenSegmentGaugeReader();
    void EnhanceImage(Mat src, OutputArray dst, OutputArray srcScaled);
    void SegmentImage(Mat src, OutputArray dst);
    ImageObject* ExtractFeatures(Mat src, Mat srcOriginal);
    ImageObject* ExtractFeatures(Mat edges, Mat enhancedImage, Mat srcScaled);
    ReaderResult Classify(ImageObject *features);
    ReaderResult ReadGaugeImage(Mat src);
    bool loadKNNDataAndTrainKNN();

    //TODO: in separate class
    double median(vector<double> collection);
private:
    typedef struct DigitInfo {
        int width, height, bottomY, leftX, value;
        double imageScale;
        DigitInfo(int width, int height, int bottomY, int leftX, int value = -1, double imageScale = 1)
        {
            this->width = width;
            this->height = height;
            this->bottomY = bottomY;
            this->leftX = leftX;
            this->value = value;
            this->imageScale = imageScale;
        }

    } DigitInfo;

    //TODO: keep source image aspect ratio
    const int X_RESOLUTION = 480;
    const int Y_RESOLUTION = 640;
    const Size IMG_SIZE = Size(X_RESOLUTION, Y_RESOLUTION);
    const int DIGIT_TEMPLATE_X_RESOLUTION = 60;
    const int DIGIT_TEMPLATE_Y_RESOLUTION = 80;
    const Size DIGIT_TEMPLATE_SIZE = Size(DIGIT_TEMPLATE_X_RESOLUTION, DIGIT_TEMPLATE_Y_RESOLUTION);

    ImageAnalizer imageAnalizer;
    Mat berekenDigitAlgorithm(Mat src, vector<vector<Point>> contours);
    vector<Point2d> getPoint(Point2d p1 , Point2d p2);
    Mat loadReferenceImage(string fileName);
    DigitInfo calculateDigitInfo(Mat src);
    DigitInfo calculateDigitInfoByMultiScaleTemplateMatch(Mat src);
    double calculateRotationDegrees(Mat src);
    void correctRotation(double rotationDegrees, Mat srcColor, Mat srcGrayScale, OutputArray dstColor, OutputArray dstGrayScale);
    double readDigits(Mat src, DigitInfo digitInfo);
};

#endif // SEVENSEGMENTGAUGEREADER_H
