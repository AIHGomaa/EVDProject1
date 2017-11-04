#ifndef MEASUREUNIT_H
#define MEASUREUNIT_H

#include <QObject>

class MeasureUnit
{
public:
    MeasureUnit();
    MeasureUnit(float factor, std::string symbol);
    float factor;
    std::string symbol;
};

#endif // MEASUREUNIT_H
