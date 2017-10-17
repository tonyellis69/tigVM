#include "var.h"

void CTigVar::setStringValue(std::string & string) {
	pStrValue = new std::string(string);
	type = tigString;
}

std::string & CTigVar::getStringValue() {
	return *pStrValue;
}
