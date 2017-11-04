#include "measureunit.h"

MeasureUnit::MeasureUnit(){}

MeasureUnit::MeasureUnit(float factor, std::string symbol)
{
    this->factor = factor;
    this->symbol = symbol;
}
