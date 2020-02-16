#include "tigObj.h"

#include "vm.h"

int CTigObj::getMemberInt(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getIntValue();
}

float CTigObj::getMemberFloat(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getFloatValue();
}

std::string CTigObj::getMemberStr(const std::string& memberName) {
	int memberId = pVM->getMemberId(memberName);
	return members[memberId].getStringValue();
}

void CTigObj::setMember(const std::string& memberName, int value) {
	int memberId = pVM->getMemberId(memberName);
	members[memberId] = value;
}

void CTigObj::setMember(const std::string& memberName, float value) {
	int memberId = pVM->getMemberId(memberName);
	members[memberId] = value;
}

void CTigObj::setMember(const std::string& memberName, const std::string& value) {
	int memberId = pVM->getMemberId(memberName);
	members[memberId] = value;
}

/** Temporarily store the name of a function being called externally. */
void CTigObj::passCallname(const std::string& fnName){
	pVM->callName = fnName;
	pVM->callParams.clear();
}

void CTigObj::call() {
	pVM->callPassedFn(id);
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