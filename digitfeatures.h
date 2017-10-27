#ifndef DIGITFEATURES_H
#define DIGITFEATURES_H

class DigitFeatures
{
public:
    DigitFeatures(int width, int height, int bottomY, int leftX, int value = -1, double imageScale = 1);
    int width, height, bottomY, leftX, value;
    double imageScale;
};

#endif // DIGITFEATURES_H
