#include "readerresult.h"

ReaderResult::ReaderResult(){}
ReaderResult::ReaderResult(float value, int precision, MeasureUnit unit, bool succesful)
{
    this->value = value;
    this->precision = precision;
    this->unit = unit;
    this->successful = succesful;
}
