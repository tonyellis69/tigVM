#pragma once

#include <vector>
#include <map>
#include <string>

#include "var.h"
#include "ITigObj.h"

/** A class encapsulating a Tig object. */
class CTigVM;
class CTigObj : public ITigIbj {
public:
	CTigObj() { id = 0; };
	int getMemberInt(const std::string& memberName);
	float getMemberFloat(const std::string& memberName);
	std::string getMemberStr(const std::string& memberName);

	void setMember(const std::string& memberName, int value);
	void setMember(const std::string& memberName, float value);
	void setMember(const std::string& memberName, const std::string& value);


	int id;
	std::vector<int> classIds;
	std::map<int, CTigVar> members;

	inline static CTigVM* pVM; ///<Gives access to VM member list etc

private:

};
