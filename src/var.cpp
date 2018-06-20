#include "var.h"

void CTigVar::setStringValue(std::string & string) {
	pStrValue = std::shared_ptr<std::string> ( new std::string(string));
	type = tigString;
}

void CTigVar::setIntValue(int n) {
	intValue = n;
	type = tigInt;
}

void CTigVar::setFloatValue(float n) {
	floatValue = n;
	type = tigFloat;
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
	if (type == tigFloat)
		return std::to_string(floatValue);
	setStringValue(std::string( "##Undefined!##"));
	return *pStrValue;
}

int CTigVar::getIntValue() {
	return intValue;
}

float CTigVar::getFloatValue() {
	return floatValue;
}

int CTigVar::getFuncAddress() {
	return funcAddress;
}

int CTigVar::getObjId() {
	return intValue;
}


