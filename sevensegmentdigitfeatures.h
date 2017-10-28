#ifndef SEVENSEGMENTDIGITFEATURES_H
#define SEVENSEGMENTDIGITFEATURES_H

class SevenSegmentDigitFeatures
{
public:
    SevenSegmentDigitFeatures(int width, int height, int bottomY, int leftX, int value = -1, double imageScale = 1);
    int width, height, bottomY, leftX, value;
    double imageScale;
};

#endif // SEVENSEGMENTDIGITFEATURES_H
