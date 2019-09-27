#include "var.h"
#include <iostream>
#include <sstream>

CTigVar::CTigVar(const CTigVar & var2) {
	type = var2.type;
	if (var2.type == tigString)
		setStringValue(*var2.pStrValue);
	else if (var2.type == tigArray) {
		setArray();
		pArray->elements = var2.pArray->elements;
	}
	else if (var2.type == tigFloat)
		floatValue = var2.floatValue;
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

bool CTigVar::operator==( CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() == var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() == var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() == var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() == var2.getStringValue();
	}
	else if (type == tigArray && var2.type == tigArray) {
		return cmpArray(var2);
	}
	else {
		return getIntValue() == var2.getIntValue();
	}
}

bool CTigVar::operator!=(CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() != var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() != var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() != var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() != var2.getStringValue();
	}
	else if (type == tigArray && var2.type == tigArray) {
		return !cmpArray(var2);
	}
	else {
		return getIntValue() != var2.getIntValue();
	}
}


bool CTigVar::operator>(CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() > var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() > var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() > var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() > var2.getStringValue();
	} else 
		return getIntValue() > var2.getIntValue();
		
}

bool CTigVar::operator>=(CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() >= var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() >= var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() >= var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() >= var2.getStringValue();
	}
	else
		return getIntValue() >= var2.getIntValue();
}

bool CTigVar::operator<=(CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() <= var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() <= var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() <= var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() <= var2.getStringValue();
	}
	else
		return getIntValue() <= var2.getIntValue();
}

bool CTigVar::operator<(CTigVar & var2) {
	if (type == tigInt && var2.type == tigFloat) {
		return getIntValue() < var2.getFloatValue();
	}
	else if (type == tigFloat && var2.type == tigInt) {
		return getFloatValue() < var2.getIntValue();
	}
	else if (type == tigFloat && var2.type == tigFloat) {
		return getFloatValue() < var2.getFloatValue();
	}
	else if (type == tigString && var2.type == tigString) {
		return getStringValue() < var2.getStringValue();
	}
	else
		return getIntValue() < var2.getIntValue();
}

CTigVar CTigVar::operator-() {
	if (type == tigInt)
		return CTigVar(-intValue);
	else if (type == tigFloat)
		return CTigVar(-floatValue);
	std::cerr << "\nError: unary minus used with non-numeric variable.";
	return CTigVar(tigUndefined);
}

CTigVar CTigVar::operator / (CTigVar& var2) {
	if (var2.type == tigFloat) {
		if (type == tigFloat)
			return getFloatValue() / var2.getFloatValue();
		if (type == tigInt)
			return getIntValue() / var2.getFloatValue();
	}
	if (var2.type == tigInt) {
		if (type == tigFloat)
			return getFloatValue() / var2.getIntValue();
		if (type == tigInt)
			return getIntValue() / var2.getIntValue();
	}
	return CTigVar(tigUndefined);
}

CTigVar CTigVar::operator*(CTigVar& var2) {
	if (var2.type == tigFloat) {
		if (type == tigFloat)
			return getFloatValue() * var2.getFloatValue();
		if (type == tigInt)
			return getIntValue() * var2.getFloatValue();
	}
	if (var2.type == tigInt) {
		if (type == tigFloat)
			return getFloatValue() * var2.getIntValue();
		if (type == tigInt)
			return getIntValue() * var2.getIntValue();
	}
	return CTigVar(tigUndefined);
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
	if (type == tigFloat) {
		std::ostringstream out;
		out.precision(2);
		out << std::fixed << floatValue;
		return out.str();

		//return std::to_string(floatValue);
	}
	if (type == tigArray)
		return std::to_string(pArray->elements.size());
	if (type == tigObj)
		return "object " + std::to_string(intValue);
	setStringValue(std::string( "##Undefined!##"));
	return *pStrValue;
}

int CTigVar::getIntValue() {
	if (type == tigInt)
		return intValue;
	if (type == tigFloat)
		return (int)floatValue;
	if (type == tigString)
		return std::atoi(pStrValue->c_str());
	if (type == tigArray)
		return pArray->elements.size();
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

int CTigVar::getArraySize() {
	if (type == tigArray) {
		return pArray->elements.size();
	}
	return 0;
}

bool CTigVar::cmpArray(CTigVar & var2) {
	if (type == tigArray && var2.type == tigArray &&
		pArray->elements.size() == var2.pArray->elements.size()) {
		for (unsigned int x = 0; x < pArray->elements.size(); x++) {
			if (pArray->elements[x] != var2.pArray->elements[x])
				return false;
		}
		return true;
	}
	return false;
}


