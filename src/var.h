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
	CTigVar(float floatVal) { setFloatValue(floatVal); }
	CTigVar(TigVarType typeVal) { type = typeVal; };
	CTigVar(const CTigVar& var2);
	void operator = (const int& var2);
	void operator = (const float& var2);
	void operator = (const std::string& var2);
	void operator = (const CArray& var2);
	void operator = (const CTigVar& var2);
	bool operator == ( CTigVar& var2);
	bool operator != (CTigVar& var2);
	bool operator > (CTigVar& var2);
	bool operator >= (CTigVar& var2);
	bool operator <= (CTigVar& var2);
	bool operator < (CTigVar& var2);
	CTigVar operator - ();
	CTigVar operator / (CTigVar& var2);
	CTigVar operator * (CTigVar& var2);
	void resetSharedPointers();
	//~CTigVar() {};
	void setStringValue(const std::string& string);
	void setIntValue(int n);
	void setFloatValue(float n);
	void setObjId(int id);
	void setFuncAddr(int addr);
	void setArray();
	std::string getStringValue();
	int getIntValue();
	float getFloatValue();
	int getFuncAddress();
	int getObjId();
	int getArraySize();
	template <class T>
	T getValue() {
		if (type == tigFloat)
			return getFloatValue();
		return getIntValue();
	}
	template <class TX, class TY>
	bool comp(TX, TY) {
		return TX > TY;
	}


	TigVarType type;
	std::shared_ptr<std::string> pStrValue;
	union {
		
		//std::string* pStrValue;
		int intValue;
		float floatValue;
		int funcAddress;
	};
	std::shared_ptr<CArray> pArray;

private:
	bool cmpArray(CTigVar& var2);

};