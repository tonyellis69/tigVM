#include "var.h"

void CTigVar::setStringValue(std::string & string) {
	pStrValue = std::shared_ptr<std::string> ( new std::string(string));
	type = tigString;
}

void CTigVar::setIntValue(int n) {
	intValue = n;
	type = tigInt;
}

std::string & CTigVar::getStringValue() {
	if (type == tigString)
		return *pStrValue;
}

int CTigVar::getIntValue() {
	return intValue;
}


