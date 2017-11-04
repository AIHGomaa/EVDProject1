#ifndef SEVENSEGMENTDIGITFEATURES_H
#define SEVENSEGMENTDIGITFEATURES_H

#include "opencv/cv.h"

class SevenSegmentDigitFeatures
{
public:
    SevenSegmentDigitFeatures(int width, int height, int bottomY, int leftX, int value = -1, double imageScale = 1);
    SevenSegmentDigitFeatures(cv::Rect boundRect);
    int width, height, bottomY, leftX, value;
    double imageScale;
    cv::Rect boundRect;
};

#endif // SEVENSEGMENTDIGITFEATURES_H
