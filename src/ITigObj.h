#pragma once


#include <string>

/** A clean interface to VM Tig objects .*/
class ITigIbj {
public:
	virtual int getMemberInt(const std::string& memberName) = 0;
	virtual float getMemberFloat(const std::string& memberName) = 0;
	virtual std::string getMemberStr(const std::string& memberName) = 0;

	virtual void setMember(const std::string& memberName, int value) = 0;
	virtual void setMember(const std::string& memberName, float value) = 0;
	virtual void setMember(const std::string& memberName, const std::string& value) = 0;




};