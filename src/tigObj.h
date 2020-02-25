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
	int& tigMemberInt(const std::string& memberName);
	float& tigMemberFloat(const std::string& memberName);
	std::string tigMemberStr(const std::string& memberName);

	int& tigMemberInt(int memberId);

	template <typename T>
	T& getMemberRef(const std::string& memberName) {
		int memberId = pVM->getMemberId(memberName);
		return members[memberId].getRef();
	}

	
	int getObjId() {
		return id;
	}

	ITigObj* getObject(int id);

	int getMemberId(const std::string& memberName);
	int getParamInt(int paramNo);

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
	CTigObjptr* getCppObj() {
		return cppObj;
	}

	inline static CTigVM* pVM; ///<Gives access to VM member list etc


};


