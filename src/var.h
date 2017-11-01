#pragma once

#include <string>
#include <memory>

#include "..\..\TC\src\sharedTypes.h"

/** A class encapsulationg the Tig dynamic data type, which can be a string, float, int, etc. */
class CTigVar {
public:
	//CTigVar() {};
	//~CTigVar() {};
	void setStringValue(std::string& string);
	void setIntValue(int n);
	void setObjId(int id);
	std::string& getStringValue();
	int getIntValue();



	TigVarType type;
	std::shared_ptr<std::string> pStrValue;
	union {
		
		//std::string* pStrValue;
		int intValue;
		float floatValue;
	};

};