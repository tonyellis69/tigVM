#include "tigObj.h"

#include "vm.h"

int& CTigObj::tigMemberInt(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getIntRef();
}

float& CTigObj::tigMemberFloat(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getFloatRef();
}

std::string CTigObj::tigMemberStr(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getStringRef();
}

int& CTigObj::tigMemberInt(int memberId) {
	return members[memberId].getIntRef();
}

std::string CTigObj::tigMemberStr(int memberId) {
	return members[memberId].getStringRef();
}



ITigObj* CTigObj::getObject(int id) {
	return pVM->getObject(id);
}

int CTigObj::getMemberId(const std::string& memberName) {
	return pVM->getMemberId(memberName);
}

int CTigObj::getParamInt(int paramNo) {
	return pVM->getParamInt(paramNo);
}

/** Temporarily store the name of a function being called externally. */
void CTigObj::passCallname(const std::string& fnName){
	pVM->callName = fnName;
	pVM->callParams.clear();
}

void CTigObj::call() {
	pVM->callPassedFn(id);
}

int CTigObj::callInt() {
	return pVM->callPassedFn(id).getIntValue();
}

ITigObj* CTigObj::callObj() {
	int objId = pVM->callPassedFn(id).getObjId();
	return pVM->getObject(objId);
}

std::string CTigObj::callStr() {
	return pVM->callPassedFn(id).getStringValue();
}

void CTigObj::passParam(int param) {
	pVM->callParams.push_back(param);
}

void CTigObj::passParam(float param) {
	pVM->callParams.push_back(param);
}

void CTigObj::passParam(const std::string& param) {
	pVM->callParams.push_back(param);
}

void CTigObj::passParam(CTigObjptr& param) {
	pVM->callParams.push_back(param.getObjId());
}

bool CTigObj::isTigClass(int classId) {
	return pVM->inheritsFrom(id,classId);
}
