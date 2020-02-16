#pragma once

#include <vector>
#include <map>
#include <string>

#include "var.h"
#include "ITigObj.h"

/** A class encapsulating a Tig object. */
class CTigVM;
class CTigObj : public ITigObj {
public:
	CTigObj() { id = 0; cppObj = 0; };
	static void setVM(CTigVM* vm) {
		pVM = vm;
	}
	int getMemberInt(const std::string& memberName);
	float getMemberFloat(const std::string& memberName);
	std::string getMemberStr(const std::string& memberName);

	void setMember(const std::string& memberName, int value);
	void setMember(const std::string& memberName, float value);
	void setMember(const std::string& memberName, const std::string& value);

	int getObjId() {
		return id;
	}



	int id;
	std::vector<int> classIds;
	std::map<int, CTigVar> members;

		CTigObjptr* cppObj; ///<Interfaces to attached C++ object, if any

private:
	void passCallname(const std::string& fnName);
	void call();
	void passParam(int param) ;
	void passParam(float param) ;
	void passParam(const std::string& param);
	void passParam(CTigObjptr& param);

	void setCppObj(CTigObjptr* cppObj) {
		this->cppObj = cppObj;
	}

	inline static CTigVM* pVM; ///<Gives access to VM member list etc


};


