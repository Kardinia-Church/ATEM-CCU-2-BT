#ifndef UTILITY_H
#define UTILITY_H

#include "WString.h"
String removeNewLine(String str) {
    str.replace("\n","");
    return str;
}

double mapfValue(double val, double in_min, double in_max, double out_min, double out_max)
{
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint32_t mapFloat(float value)
{
  return (uint32_t)mapfValue(value, 0, 1.0, 0, 2047.0);
}

#endif