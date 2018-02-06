#include "var.h"

void CTigVar::setStringValue(std::string & string) {
	pStrValue = std::shared_ptr<std::string> ( new std::string(string));
	type = tigString;
}

void CTigVar::setIntValue(int n) {
	intValue = n;
	type = tigInt;
}

void CTigVar::setObjId(int id) {
	intValue = id;
	type = tigObj;
}

void CTigVar::setFuncAddr(int addr) {
	type = tigFunc;
	funcAddress = addr;
}

std::string CTigVar::getStringValue() {
	if (type == tigString)
		return *pStrValue;
	if (type == tigInt)
		return std::to_string(intValue);
	setStringValue(std::string( "##Undefined!##"));
	return *pStrValue;
}

int CTigVar::getIntValue() {
	return intValue;
}

int CTigVar::getFuncAddress() {
	return funcAddress;
}


