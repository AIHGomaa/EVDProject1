#include "readerresult.h"

ReaderResult::ReaderResult(){}
ReaderResult::ReaderResult(float value, int precision, MeasureUnit unit, bool succesfull)
{
    this->value = value;
    this->precision = precision;
    this->unit = unit;
    this->successful = succesfull;
}
