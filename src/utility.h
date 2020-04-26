#include "WString.h"
String removeNewLine(String str) {
    str.replace("\n","");
    return str;
}