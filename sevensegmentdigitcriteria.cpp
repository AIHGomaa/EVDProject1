#include "sevensegmentdigitcriteria.h"

const double SevenSegmentDigitCriteria::digitWTolerance = 0.45;
const double SevenSegmentDigitCriteria::digitHTolerance = 0.3;
const double SevenSegmentDigitCriteria::digitYTolerance = 0.03;

SevenSegmentDigitCriteria::SevenSegmentDigitCriteria() {}

SevenSegmentDigitCriteria SevenSegmentDigitCriteria::create(SevenSegmentDigitFeatures referenceDigitFeatures, Size digitTemplateSize, double imageScaleFactor)
{
    SevenSegmentDigitCriteria result = SevenSegmentDigitCriteria();

    result.minDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 - digitYTolerance) + 0.5;
    result.maxDigitBottomY = referenceDigitFeatures.bottomY * imageScaleFactor * (1 + digitYTolerance) + 0.5;
    // Digits 0 and 2..9
    result.minDigitSize = Size(digitTemplateSize.width * (1 - digitWTolerance) + 0.5, digitTemplateSize.height * (1 - digitHTolerance) + 0.5);
    result.maxDigitSize = Size(digitTemplateSize.width * (1 + digitWTolerance) + 0.5, digitTemplateSize.height * (1 + digitHTolerance) + 0.5);
    // Digit 1
    result.minDigit1Size = Size(result.minDigitSize.width / 5, result.minDigitSize.height);
    result.maxDigit1Size = Size(result.maxDigitSize.width / 3, result.maxDigitSize.height);
    // Decimal point
    result.minDecimalPointSize = Size(result.minDigitSize.width / 6, result.minDigitSize.height / 6);
    result.maxDecimalPointSize = Size(result.maxDigitSize.width / 4, result.maxDigitSize.height / 4);

    qDebug() << "minDigitBottomY" << result.minDigitBottomY;
    qDebug() << "maxDigitBottomY" << result.maxDigitBottomY;
    qDebug() << "minDigitSize" << result.minDigitSize.width << result.minDigitSize.height;
    qDebug() << "maxDigitSize" << result.maxDigitSize.width << result.maxDigitSize.height;
    qDebug() << "minDigit1Size" << result.minDigit1Size.width << result.minDigit1Size.height;
    qDebug() << "maxDigit1Size" << result.maxDigit1Size.width << result.maxDigit1Size.height;
    qDebug() << "minDecimalPointSize" << result.minDecimalPointSize.width << result.minDecimalPointSize.height;
    qDebug() << "maxDecimalPointSize" << result.maxDecimalPointSize.width << result.maxDecimalPointSize.height;

    return result;
}
