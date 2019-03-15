#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <iostream>
#include <fstream>
#include <list>
#include <random>

#include "stack.h"
#include "daemon.h"
#include "..\3DEngine\src\utils\idContainer.h"

struct TEventRec {
	int id;
	int address;
};

struct TOptionRec {
	int index;///<Position among the total number of options for this event.
	std::string text; ///<Option text.
};

struct THotTExt {
	//THotTExt() { used = false; }
	std::string text;
	int msgId;
	int objId;
	bool used;
	std::vector<CTigVar> params;
};

/**	Details the function call specifics of a hot text. */
struct THotTextFnCall {
	int msgId; ///<The function id.
	int objId; ///<The object it is being called on.
	std::vector<CTigVar> params;
};



class CObjInstance {
public:
	CObjInstance() { id = 0;  };
	int id;
	std::vector<int> classIds;
	std::map<int, CTigVar> members;
};



enum TVMmsgType { vmMsgChoice, vmMsgString };
struct TVMmsg {
	TVMmsgType type;
	std::string text;
	int integer;
};

enum TVMstatus { vmExecuting, vmAwaitChoice, vmAwaitString, vmEnding, vmError, vmNoProgram,
	vmEof};


/** The Tig virtual machine. Reads compiled Tig code and executes it. */
class CTigVM {
public:
	CTigVM();
	~CTigVM();
	bool loadProgFile(std::string filename);
	void initIds();
	int readHeader(std::ifstream& progFile);
	void readEventTable(std::ifstream& progFile);
	void readObjectDefTable(std::ifstream& progFile);
	void readMemberNameTable(std::ifstream & progFile);
	void execute();
	void update();

	int readNextOp();
	std::string readString();
	unsigned int readWord();
	char readByte();

	void pushInt();
	void pushStr();
	void pushVar();
	void print();
	void option();
	void giveOptions();
	void end();
	void assign();
	void getString();
	void add();
	void sub();
	void mod();
	void minus();
	void jumpEvent();
	void startTimer();
	void createTimedEvent();
	void pushObj();
	void pushSelf();
	void call();

	void callDeref();
	void superCall();
	void returnOp();
	void returnTrue();
	void hot();
	void purge();
	void initArray();
	void pushElem();
	void assignElem();
	void arrayIt();
	void pop();
	void compEq();
	void compNE();
	void compLT();
	void compLE();
	void compGT();
	void compGE();
	void jump();
	void jumpFalse();
	void child();
	void sibling();
	void in();
	void notIn();
	void getVar();
	void children();
	void makeHot();
	void brk();
	void move();
	void openWin();
	void win();
	void clr();
	void style();
	void cap();
	void inherits();
	void hotClr();
	void hotCheck();
	void not();
	void and();
	void or();
	void arrayPush();
	void msg();
	void has();
	void match();
	void is();
	void isNot();
	void set();
	void unset();
	void newOp();
	void deleteOp();
	void finalLoop();
	void roll();
	void sortDesc();
	void log();

	CTigVar * resolveVariableAddress(int varId);
	CTigVar * resolveVariableAddress(int varId, int& owningObject);
	//CTigVar * resolveMemberFuncAddress(int memberId, int& obj);
	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(const TVMmsg& msg);
	virtual void writeText(std::string& text) {};
	virtual void hotText(std::string& text, int memberId, int objectId) {};
	virtual void purge(int memberId, int objId) {};
	virtual void purge(unsigned int hotFnCallId) {};
	virtual void clearWin() {};
	virtual void openWindow(int objId) {};
	virtual void messageApp(int p1, int p2) {};
	virtual void logText(std::string& text) {};
	
	CTigVar getGlobalVar(std::string varName);
	CTigVar getMember(int objNo, int memberId);
	int getMemberInt(int objNo, int memberId);
	int getMemberId(std::string  name);

	CTigVar callMember(int objId, int memberId, std::initializer_list<CTigVar> params);
	CTigVar callMember(int objId, std::string msgName, std::initializer_list<CTigVar> params);
	CTigVar callMember(int objId, std::string msgName);
	CTigVar callMember(int objId, int msgId);
	CTigVar callMember(int objId, int msgId, std::vector<CTigVar>& params);
	std::string getMemberName(int memberId);

	bool inheritsFrom(int obj, int classObj);

	void setMemberValue(int objNo, std::string memberName, CTigVar& value);

	bool hasMember(int objNo, int memberNo);

	CObjInstance* getObject(int objId);
	int getObjectId(CObjInstance* obj);

	std::string devil(std::string text);

	unsigned int regExFind(std::string source, unsigned int offset, std::string subject);

	void removeHotTextFnCall(unsigned int id);
	THotTextFnCall getHotTextFnCall(unsigned int id);

	void reset();
	bool reloadProgFile();

	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
	int globalCodeAddr; 
	bool escape; ///<Set to true to escape the normal execution loop.
	TVMstatus status; ///<Current mode.

	std::vector<TEventRec> eventTable; ///<Event ids and addresses.
	std::vector<TOptionRec> currentOptionList; ///<A list of user options
	std::map<int, CObjInstance> objects; ///<Object instances.
	std::vector<std::string> memberNames; ///<Member names in id number order.

	CStack stack; ///<The virtual machine stack.

	time_t currentTime;
	CDaemon daemon;

	int currentObject; ///<Owner of the function currently being executed.
	
	int childId; ///<Member id for the 'child' member
	int siblingId; ///<Member id for the 'sibling' member
	int parentId; ///<Member id for the 'parent' member
	int flagsId; ///<Member id for the '#flags' member


	int window; ///<The output we're currently writing to.

	std::vector<THotTExt> hotTextKeywords; ///<A list of known hot texts, if any, for the devil to look out for.
	CIdMap<THotTextFnCall> hotTextFnCalls; ///<The function calls, with parameters, of any current hot texts.

	std::string latestText; ///<The most recent text sent to output.

private:
	std::string currentProgFile; ///<Path of the current program file.
	int nextFreeObjNo;

	std::mt19937 randEngine; ///<The vm's own randon number engine.

};