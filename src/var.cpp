#include "var.h"

CTigVar::CTigVar(const CTigVar & var2) {
	type = var2.type;
	if (var2.type == tigString)
		setStringValue(*var2.pStrValue);
	else if (var2.type == tigArray) {
		setArray();
		pArray->elements = var2.pArray->elements;
	}
	else
		intValue = var2.intValue;

}

void CTigVar::operator=(const int& var2) {
	resetSharedPointers();
	type = tigInt;
	intValue = var2;
}

void CTigVar::operator=(const float & var2) {
	resetSharedPointers();
	type = tigFloat;
	floatValue = var2;
}

void CTigVar::operator=(const std::string & var2) {
	resetSharedPointers();
	type = tigString;
	setStringValue(var2);
}

void CTigVar::operator=(const CArray & var2) {
	resetSharedPointers();
	type = tigArray;
	setArray();
	pArray->elements = var2.elements;
}

void CTigVar::operator=(const CTigVar & var2){
	resetSharedPointers();
	type = var2.type;
	if (var2.type == tigString)
		setStringValue(*var2.pStrValue);
	else if (var2.type == tigArray) {
		setArray();
		pArray->elements = var2.pArray->elements;
	}
	else
		intValue = var2.intValue;
}

void CTigVar::resetSharedPointers() {
	if (type == tigString) {
		pStrValue.reset();
	}
	else
		if (type == tigArray) {
			pArray.reset();
		}
}

void CTigVar::setStringValue(const std::string & string) {
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

void CTigVar::setArray() {
	pArray = std::shared_ptr<CArray>(new CArray );
	type = tigArray;
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


