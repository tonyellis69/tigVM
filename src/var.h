#pragma once

#include <string>
#include <memory>
#include <vector>

#include "..\..\TC\src\sharedTypes.h"

class CTigVar;
class CArray {
public:
	std::vector<CTigVar> elements;
};

/** A class encapsulationg the Tig dynamic data type, which can be a string, float, int, etc. */
class CTigVar {
public:
	CTigVar() { intValue = 0; floatValue = 0.0; funcAddress = 0; type = tigInt; }
	CTigVar(int intVal) { setIntValue(intVal); }
	CTigVar(TigVarType typeVal) { type = typeVal; };
	//~CTigVar() {};
	void setStringValue(std::string& string);
	void setIntValue(int n);
	void setFloatValue(float n);
	void setObjId(int id);
	void setFuncAddr(int addr);
	void setArray(CArray& array);
	std::string getStringValue();
	int getIntValue();
	float getFloatValue();
	int getFuncAddress();
	int getObjId();

	TigVarType type;
	std::shared_ptr<std::string> pStrValue;
	union {
		
		//std::string* pStrValue;
		int intValue;
		float floatValue;
		int funcAddress;
	};
	CArray* pArray; //TO DO: needs to be shared pointer!
};