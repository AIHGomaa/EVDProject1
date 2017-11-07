#include "sevensegmentdigitfeatures.h"

SevenSegmentDigitFeatures::SevenSegmentDigitFeatures(int width, int height, int bottomY, int leftX, int value, double imageScale)
{
    this->width = width;
    this->height = height;
    this->bottomY = bottomY;
    this->leftX = leftX;
    this->value = value;
    this->imageScale = imageScale;
    this->boundRect = cv::Rect(leftX, bottomY - height, width, height);
}

SevenSegmentDigitFeatures::SevenSegmentDigitFeatures(cv::Rect boundRect)
{
    this->boundRect = boundRect;
    this->width = boundRect.width;
    this->height = boundRect.height;
    this->leftX = boundRect.x;
    this->bottomY = boundRect.y + boundRect.height;
}
