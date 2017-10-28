#include "sevensegmentdigitfeatures.h"

SevenSegmentDigitFeatures::SevenSegmentDigitFeatures(int width, int height, int bottomY, int leftX, int value, double imageScale)
{
    this->width = width;
    this->height = height;
    this->bottomY = bottomY;
    this->leftX = leftX;
    this->value = value;
    this->imageScale = imageScale;
}
