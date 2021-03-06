#ifndef SEVENSEGMENTDIGITCRITERIA_H
#define SEVENSEGMENTDIGITCRITERIA_H

#include "qdebug.h"
#include "sevensegmentdigitfeatures.h"
#include "opencv/cv.h"

using namespace cv;

class SevenSegmentDigitCriteria
{
private:
public:
    SevenSegmentDigitCriteria();
    static SevenSegmentDigitCriteria create(SevenSegmentDigitFeatures referenceDigitFeatures, Size digitTemplateSize, double imageScaleFactor);

    //TODO: configurable in ui
    static const double digitWTolerance;
    static const double digitHTolerance;
    static const double digitYTolerance;

    int minDigitBottomY;
    int maxDigitBottomY;
    // Digits 0 and 2..6 and 8..9
    Size minDigitSize;
    Size maxDigitSize;
    // Digit 1
    Size minDigit1Size;
    Size maxDigit1Size;
    // Digit 7
    Size minDigit7Size;
    Size maxDigit7Size;
    // Decimal point
    Size minDecimalPointSize;
    Size maxDecimalPointSize;
};

#endif // SEVENSEGMENTDIGITCRITERIA_H
