#ifndef READERRESULT_H
#define READERRESULT_H

#include <QObject>
#include "measureunit.h"

class ReaderResult
{
public:
    ReaderResult();
    float value;
    MeasureUnit unit;
};

#endif // READERRESULT_H
