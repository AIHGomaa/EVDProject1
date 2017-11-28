#ifndef SEVENSEGMENTGAUGEREADER_H
#define SEVENSEGMENTGAUGEREADER_H

#include <cstdlib>
#include <QObject>
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "igaugereader.h"
#include "imageTools.h"
#include "templatematchfeatures.h"
#include "sevensegmentdigitfeatures.h"
#include "sevensegmentdigitcriteria.h"

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

    uint showImageFlags = SHOW_FEATURE_EXTRACTION_FLAG | SHOW_CLASSIFICATION_FLAG;

    bool showAllContoursForTest = false;

    int enhanceThreshold = 127;
    double enhancementThreshFactor = 1;
    double gammaCorrectionFactor = 4;

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

    int templateMatchCannyThreshold1 = 50;
    int templateMatchCannyThreshold2 = 50 * 4;
    bool templayteMatchByEdges = true;
    int templateMatchMethod = TM_CCOEFF; // TM_SQDIFF_NORMED, TM_SQDIFF
    double templateMatchThreshold = 0.95;

    SevenSegmentGaugeReader();
    void initialize(int cols, int rows);
    ReaderResult ReadGaugeImage(Mat src);
    Mat getMarkedImage();
    Mat getSourceImage();
private:
    const int X_RESOLUTION = 480;
    //    const int Y_RESOLUTION = 640;
    int yResolution;
    Size imgSize;
    const int DIGIT_TEMPLATE_X_RESOLUTION = 60;
    const int DIGIT_TEMPLATE_Y_RESOLUTION = 80;
    const Size DIGIT_TEMPLATE_SIZE = Size(DIGIT_TEMPLATE_X_RESOLUTION, DIGIT_TEMPLATE_Y_RESOLUTION);
    SevenSegmentDigitCriteria digitCriteria;
    Mat srcImage, markedDigits;
    Ptr<cv::ml::KNearest> kNearest = cv::ml::KNearest::create();

    void EnhanceImage(Mat src, OutputArray dst, OutputArray srcScaled);
    void SegmentImage(Mat src, OutputArray dst);
    vector<SevenSegmentDigitFeatures> ExtractFeatures(Mat edges, Mat enhancedImage, Mat colorImage, OutputArray colorResized, OutputArray colorResizedFiltered);
    vector<Point2d> getPoint(Point2d p1 , Point2d p2);
    Mat loadReferenceImage(string fileName, int flags = CV_LOAD_IMAGE_GRAYSCALE);
    SevenSegmentDigitFeatures extractReferenceDigitFeaturesByMultiScaleTemplateMatch(Mat src);
    bool loadKNNDataAndTrainKNN();
    double calculateRotationDegrees(Mat src);
    void correctRotation(double rotationDegrees, Mat srcColor, Mat srcGrayScale, OutputArray dstColor, OutputArray dstGrayScale);
    bool isPotentialDigitOrDecimalPoint(Rect rect, SevenSegmentDigitCriteria digitCriteria);
    ReaderResult classifyDigitsBySegmentPositions(Mat src, vector<vector<Point>> contours);
    ReaderResult classifyDigitsByKNearestNeighborhood(vector<SevenSegmentDigitFeatures> digitFeatures, Mat colorResized, Mat colorResizedFiltered, SevenSegmentDigitCriteria digitCriteria);
    //    ReaderResult classifyDigitsByTemplateMatching(Mat src, SevenSegmentDigitFeatures digitFeatures);
    void enhanceContrast(Mat src, Mat dst);
};

#endif // SEVENSEGMENTGAUGEREADER_H
