#ifndef READERRESULT_H
#define READERRESULT_H

#include <QObject>
#include "measureunit.h"

class ReaderResult
{
public:
    ReaderResult();
    ReaderResult(float value, int precision, MeasureUnit unit, bool succesful);
    float value;
    MeasureUnit unit;
    bool successful = false;
    int precision;
};

#endif // READERRESULT_H
