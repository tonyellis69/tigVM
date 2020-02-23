#pragma once


#include <string>

class CTigObjptr;

/** A clean interface to VM Tig objects .*/
class ITigObj {
public:
	virtual int getMemberInt(const std::string& memberName) = 0;
	virtual float getMemberFloat(const std::string& memberName) = 0;
	virtual std::string getMemberStr(const std::string& memberName) = 0;

	virtual void setMember(const std::string& memberName, int value) = 0;
	virtual void setMember(const std::string& memberName, float value) = 0;
	virtual void setMember(const std::string& memberName, const std::string& value) = 0;

	template <typename H, typename... T>
	void call(H h,T... t) {
		passParam(h);
		call(t...);
	};

	virtual void call() = 0;

	virtual int getObjId() = 0;
	virtual ITigObj* getObject(int id) = 0;
	virtual int getMemberId(const std::string& memberName) = 0;
	virtual int getParamInt(int paramNo) = 0;


	virtual void setCppObj(CTigObjptr* cppObj) = 0;
	virtual CTigObjptr* getCppObj() = 0;
	

private:
	virtual void passCallname(const std::string& fnName) = 0;
	virtual void passParam(int param) = 0;
	virtual void passParam(float param) = 0;
	virtual void passParam(const std::string& param) = 0;
	virtual void passParam(CTigObjptr& param) = 0;


};


class CTigObjptr {
public:
	int getMemberInt(const std::string& memberName) {
		return tigObj->getMemberInt(memberName);
	}

	void setMember(const std::string& memberName, int value) {
		tigObj->setMember(memberName, value);
	}

	template <typename H, typename... T>
	void callTig(H h, T... t) {
		tigObj->call(h,t...);
	};

	void callTig() {
		tigObj->call();
	}

	void setTigObj(ITigObj* tigObj) {
		this->tigObj = tigObj;
		tigObj->setCppObj(this);
	}
	int getObjId() {
		return tigObj->getObjId();
	}

	ITigObj* getObject(int id) {
		return tigObj->getObject(id);
	}

	int getMemberId(const std::string& memberName) {
		return tigObj->getMemberId(memberName);
	}
	int getParamInt(int paramNo) {
		return tigObj->getParamInt(paramNo);
	}

	virtual int tigCall(int memberId) = 0;

	ITigObj* tigObj;
};