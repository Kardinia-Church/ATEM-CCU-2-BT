#ifndef UTILITY_H
#define UTILITY_H

#include "WString.h"
String removeNewLine(String str) {
    str.replace("\n","");
    return str;
}

#endif