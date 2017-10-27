#ifndef SEVENSEGMENTGAUGEREADER_H
#define SEVENSEGMENTGAUGEREADER_H

#include <cstdlib>
#include <QObject>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "igaugereader.h"
#include "imageanalizer.h"
#include "digitfeatures.h"

using namespace cv;
using namespace std;

class SevenSegmentGaugeReader : public IGaugeReader
{
public:
    enum ShowImageFlags {
        SHOW_NONE = 0x00,
        SHOW_IMAGE_ENHANCEMENT_FLAG = 0x01,
        SHOW_SEGMENTATION_FLAG = 0x02,
        SHOW_FEATURE_EXTRACTION_FLAG = 0x04,
        SHOW_CLASSIFICATION_FLAG = 0x08,
        SHOW_ROTATION_CORRECTION_FLAG = 0x10,
        SHOW_KNN_TRAINING_FLAG = 0x20,
        SHOW_FEATURE_EXTRACT_REFERENCE_DIGIT_FLAG = 0x40,
        SHOW_MAIN_RESULTS_FLAG = 0x100
    };

    uint showImageFlags = SHOW_FEATURE_EXTRACTION_FLAG | SHOW_MAIN_RESULTS_FLAG;

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
    int digitDilateKernelHeight = 7;

    //TODO: configure in UI
    int templateMatchCannyThreshold1 = 50;
    int templateMatchCannyThreshold2 = 50 * 4;
    bool templayteMatchByEdges = true;
    int templateMatchMethod = TM_CCOEFF; // TM_SQDIFF_NORMED, TM_SQDIFF

    double templateMatchThreshold = 0.95;

    Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();

    SevenSegmentGaugeReader();
    void EnhanceImage(Mat src, OutputArray dst, OutputArray srcScaled);
    void SegmentImage(Mat src, OutputArray dst);
    DigitFeatures* ExtractFeatures(Mat src, Mat srcOriginal);
    DigitFeatures* ExtractFeatures(Mat edges, Mat enhancedImage, Mat colorImage);
    ReaderResult Classify(DigitFeatures *features);
    ReaderResult ReadGaugeImage(Mat src);
    bool loadKNNDataAndTrainKNN();

private:
    const int X_RESOLUTION = 480;
    const int Y_RESOLUTION = 640;
    const Size IMG_SIZE = Size(X_RESOLUTION, Y_RESOLUTION);
    const int DIGIT_TEMPLATE_X_RESOLUTION = 60;
    const int DIGIT_TEMPLATE_Y_RESOLUTION = 80;
    const Size DIGIT_TEMPLATE_SIZE = Size(DIGIT_TEMPLATE_X_RESOLUTION, DIGIT_TEMPLATE_Y_RESOLUTION);

    ImageAnalizer imageAnalizer;
    Mat classifyDigitsBySegmentPositions(Mat src, vector<vector<Point>> contours);
    vector<Point2d> getPoint(Point2d p1 , Point2d p2);
    Mat loadReferenceImage(string fileName);
    DigitFeatures extractDigitFeaturesByCustomTemplateMatching(Mat src);
    DigitFeatures extractReferenceDigitFeaturesByMultiScaleTemplateMatch(Mat src);
    double calculateRotationDegrees(Mat src);
    void correctRotation(double rotationDegrees, Mat srcColor, Mat srcGrayScale, OutputArray dstColor, OutputArray dstGrayScale);
    double classifyDigitsByTemplateMatching(Mat src, DigitFeatures digitFeatures);
    double median(vector<double> collection);
};

#endif // SEVENSEGMENTGAUGEREADER_H
