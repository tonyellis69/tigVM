#pragma once


#include <string>

class CTigObjptr;

/** A clean interface to VM Tig objects .*/
class ITigObj {
public:
	virtual int& tigMemberInt(const std::string& memberName) = 0;
	virtual float& tigMemberFloat(const std::string& memberName) = 0;
	virtual std::string tigMemberStr(const std::string& memberName) = 0;

	virtual int& tigMemberInt(int memberId) = 0;
	virtual std::string tigMemberStr(int memberId) = 0;


	virtual bool isTigClass(int classId) = 0;

	template <typename H, typename... T>
	void call(H h,T... t) {
		passParam(h);
		call(t...);
	};

	template <typename H, typename... T>
	int callInt(H h, T... t) {
		passParam(h);
		return callInt(t...);
	};

	template <typename H, typename... T>
	ITigObj* callObj(H h, T... t) {
		passParam(h);
		return callObj(t...);
	};

	virtual void call() = 0;
	virtual int callInt() = 0;
	virtual ITigObj* callObj() = 0;

	virtual int getObjId() = 0;
	virtual ITigObj* getObject(int id) = 0;
	virtual int getMemberId(const std::string& memberName) = 0;
	virtual int getParamInt(int paramNo) = 0;

	template <typename T>
	T& getRef(const std::string& memberName) {
		return getMemberRef(memberName);
	}

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


	int& tigMemberInt(const std::string& memberName) {
		return tigObj->tigMemberInt(memberName);
	}
	int& tigMemberInt(int memberId) {
		return tigObj->tigMemberInt(memberId);
	}

	std::string tigMemberString(int memberId) {
		return tigObj->tigMemberStr(memberId);
	}

	bool isTigClass(int classId) {
		return tigObj->isTigClass(classId);
	}

	template <typename H, typename... T>
	void callTig(H h, T... t) {
		tigObj->call(h,t...);
	};
	
	template <typename H, typename... T>
	int callTigInt(H h, T... t) {
		return tigObj->callInt(h, t...);
	};

	template <typename H, typename... T>
	ITigObj* callTigObj(H h, T... t) {
		return tigObj->callObj(h, t...);
	};

	//void callTig() {
	//	tigObj->call();
	//}



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
	ITigObj* getParamObj(int paramNo) {
		int objId = tigObj->getParamInt(paramNo);
		return tigObj->getObject(objId);
	}

	virtual int tigCall(int memberId) = 0;

	ITigObj* tigObj;
};