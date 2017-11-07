#include "sevensegmentdigitcriteria.h"

const double SevenSegmentDigitCriteria::digitWTolerance = 0.2;
const double SevenSegmentDigitCriteria::digitHTolerance = 0.2;
const double SevenSegmentDigitCriteria::digitYTolerance = 0.02;

SevenSegmentDigitCriteria::SevenSegmentDigitCriteria() {}

SevenSegmentDigitCriteria SevenSegmentDigitCriteria::create(SevenSegmentDigitFeatures referenceDigitFeatures, Size digitTemplateSize, double imageScaleFactor)
{
    SevenSegmentDigitCriteria result = SevenSegmentDigitCriteria();

    result.minDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 - digitYTolerance * 1.5) + 0.5;
    result.maxDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 + digitYTolerance) + 0.5;
    // Digits 0 and 2..6 and 8..9
    result.minDigitSize = Size(digitTemplateSize.width *  (1 - digitWTolerance * 2) + 0.5, digitTemplateSize.height * (1 - digitHTolerance * 1.5) + 0.5);
    result.maxDigitSize = Size(digitTemplateSize.width *  (1 + digitWTolerance) + 0.5, digitTemplateSize.height * (1 + digitHTolerance) + 0.5);
    // Digit 1
    result.minDigit1Size = Size(result.minDigitSize.width * 0.2, result.minDigitSize.height * 0.8);
    result.maxDigit1Size = Size(result.maxDigitSize.width * 0.4, result.maxDigitSize.height * 0.9);
    // Digit 7
    result.minDigit7Size = Size(result.minDigitSize.width * 0.5, result.minDigitSize.height * 0.8);
    result.maxDigit7Size = Size(result.maxDigitSize.width * 0.8, result.maxDigitSize.height * 0.9);
    // Decimal point
    result.minDecimalPointSize = Size(result.minDigitSize.width * 0.1, result.minDigitSize.height * 0.1);
    result.maxDecimalPointSize = Size(result.maxDigitSize.width * 0.3, result.maxDigitSize.height * 0.25);

    qDebug() << "minDigitBottomY" << result.minDigitBottomY;
    qDebug() << "maxDigitBottomY" << result.maxDigitBottomY;
    qDebug() << "minDigitSize" << result.minDigitSize.width << result.minDigitSize.height;
    qDebug() << "maxDigitSize" << result.maxDigitSize.width << result.maxDigitSize.height;
    qDebug() << "minDigit1Size" << result.minDigit1Size.width << result.minDigit1Size.height;
    qDebug() << "maxDigit1Size" << result.maxDigit1Size.width << result.maxDigit1Size.height;
    qDebug() << "minDigi71Size" << result.minDigit7Size.width << result.minDigit7Size.height;
    qDebug() << "maxDigit7Size" << result.maxDigit7Size.width << result.maxDigit7Size.height;
    qDebug() << "minDecimalPointSize" << result.minDecimalPointSize.width << result.minDecimalPointSize.height;
    qDebug() << "maxDecimalPointSize" << result.maxDecimalPointSize.width << result.maxDecimalPointSize.height;

    return result;
}
