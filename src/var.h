#pragma once

#include <string>

enum TigVarType { tigString, tigInt, tigFloat };

/** A class encapsulationg the Tig dynamic data type, which can be a string, float, int, etc. */
class CTigVar {
public:
	CTigVar() {};
	void setStringValue(std::string& string);
	std::string& getStringValue();

	TigVarType type;

	union {
		std::string* pStrValue;
		int intValue;
		float floatValue;
	};


};