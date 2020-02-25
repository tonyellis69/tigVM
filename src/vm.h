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
#include "tigObj.h"

#include "Ivm.h"

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

class TFnCall {
public:
	bool operator == (TFnCall& var2) {
		bool paramMatch = true;
		for (unsigned int x = 0; x < params.size(); x++) {
			if (params[x] != var2.params[x]) {
				paramMatch = false;
				break;
			}
		}
		return objId == var2.objId && msgId == var2.msgId &&
			paramMatch;
	}
	int objId; ///<The object the call is being called on.
	int msgId; ///<The function id.
	std::vector<CTigVar> params;
};

/**	Details the function call specifics of a hot text. */
struct THotTextFnCall {

	std::string hotText; ///<The actual hot text text.
	std::vector<TFnCall> options; ///<The 1 or more calls attached to this hot text.

	//int msgId; ///<The function id.
	//int objId; ///<The object it is being called on.
	//std::vector<CTigVar> params;
};





enum TVMmsgType { vmMsgChoice, vmMsgString };
struct TVMmsg {
	TVMmsgType type;
	std::string text;
	int integer;
};

enum TVMstatus { vmExecuting, vmAwaitChoice, vmAwaitString, vmEnding, vmError, vmNoProgram,
	vmEof, vmPaused};


/** The Tig virtual machine. Reads compiled Tig code and executes it. */
class CTigVM : public Ivm {
public:
	CTigVM();
	~CTigVM();
	bool loadProgFile(std::string filename);
	void initIds();
	int readHeader(std::ifstream& progFile);
	void readEventTable(std::ifstream& progFile);
	void readObjectDefTable(std::ifstream& progFile);
	void readObjectNameTable(std::ifstream& progFile);
	void readConstNameTable(std::ifstream& progFile);
	void readMemberNameTable(std::ifstream & progFile);
	void readFlagNameTable(std::ifstream& progFile);
	void execute();
	void update();

	int readNextOp();
	std::string readString();
	unsigned int readWord();
	float readFWord();
	char readByte();

	void pushInt();
	void pushStr();
	void pushFloat();
	void pushVar();
	void print();
	void option();
	void giveOptions();
	void end();
	void assign();
	void getString();
	void div();
	void mult();
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
	void makeHotAlt();
	void brk();
	void move();
	void openWin();
	void openWinModal();
	void win();
	void clr();
	void clrMarked();
	void style();
	void cap();
	void capNext();
	void inherits();
	void hotClr();
	void hotCheck();
	void not();
	void and();
	void or();
	void arrayPush();
	void arrayRemove();
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
	void firstLoop();
	void roll();
	void rand();
	void sortDesc();
	void log();
	void pause();
	void randArray();
	void roundOp();
	void minOp();
	void maxOp();


	CTigVar * resolveVariableAddress(int varId);
	CTigVar * resolveVariableAddress(int varId, int& owningObject);
	//CTigVar * resolveMemberFuncAddress(int memberId, int& obj);
	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(const TVMmsg& msg);
	virtual void writeText(std::string& text) {};
	//virtual void hotText(std::string& text, int memberId, int objectId) {};
	virtual void purge(int memberId, int objId) {};
	virtual void purge(unsigned int hotFnCallId) {};
	virtual void clearWin() {};
	virtual void clearMarkedText() {};
	virtual void openWindow(int objId, bool modal) {};
	virtual void messageApp(int p1, int p2) {};
	virtual void messageApp(const std::string& p1, int p2) {};
	virtual void logText(std::string& text) {};
	virtual void temporaryText(int onOff, int winId) {};
	virtual void handlePause(bool pauseOn) {};
	virtual void flush() {};
	
	CTigVar getGlobalVar(std::string varName);
	CTigVar getMember(int objNo, int memberId);
	int tigMemberInt(int objNo, int memberId);
	int getMemberId(std::string  name);

	int getFlagBitmask(std::string name);

	CTigVar callMember(int objId, int memberId, std::initializer_list<CTigVar> params);
	CTigVar callMember(int objId, std::string msgName, std::initializer_list<CTigVar> params);
	CTigVar callMember(int objId, std::string msgName);
	CTigVar callMember(int objId, int msgId);
	CTigVar callMember(int objId, int msgId, std::vector<CTigVar>& params);
	std::string getMemberName(int memberId);

	bool inheritsFrom(int obj, int classObj);

	void setMemberValue(int objNo, std::string memberName, CTigVar& value);

	bool hasMember(int objNo, int memberNo);

	//CTigObj* getTigObj(int objId);
	int getObjectId(CTigObj* obj);
	ITigObj* getObject(const std::string& objName);
	ITigObj* getObject(int objId);
	int getConst(const std::string& constName);

	std::string devil(std::string text);

	unsigned int regExFind(std::string source, unsigned int offset, std::string subject);

	void removeHotTextFnCall(unsigned int id);
	TFnCall getHotTextFnCall(unsigned int id, int variant);
	int getHotTextFnCallObj(int hotId);

	THotTextFnCall getHotTextFnCallRec(unsigned int id);

	void reset();
	bool reloadProgFile();

	bool hasFlag(int objId, int flagId);

	void callPassedFn(int objId);

	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
	int globalCodeAddr; 
	bool escape; ///<Set to true to escape the normal execution loop.
	TVMstatus status; ///<Current mode.

	std::vector<TEventRec> eventTable; ///<Event ids and addresses.
	std::vector<TOptionRec> currentOptionList; ///<A list of user options
	std::map<int, CTigObj> objects; ///<Object instances.
	std::map<std::string, int> objectNames;
	std::map<std::string, int> constNames;
	std::vector<std::string> memberNames; ///<Member names in id number order.
	std::vector<std::string> flagNames; ///<Flag names in bitmask order.

	CStack stack; ///<The virtual machine stack.

	time_t currentTime;
	CDaemon daemon;

	int currentObject; ///<Owner of the function currently being executed.
	
	int childId; ///<Member id for the 'child' member
	int siblingId; ///<Member id for the 'sibling' member
	int parentId; ///<Member id for the 'parent' member
	int flagsId; ///<Member id for the '#flags' member
	int gameStateId; ///


	int window; ///<The output we're currently writing to.

	std::vector<THotTExt> hotTextKeywords; ///<A list of known hot texts, if any, for the devil to look out for.
	CIdMap<THotTextFnCall> hotTextFnCalls; ///<The function calls, with parameters, of any current hot texts.

	std::string latestText; ///<The most recent text sent to output.

	std::string callName; ///<Temp store for name of a function being called externally.
	std::vector<CTigVar> callParams; ///<Temp store for paramaters of function called externally.

	int getParamInt(int paramNo);
	float getParamFloat(int paramNo);
	std::string getParamStr(int paramNo);

private:
	void callExternal(int objId, int memberId);

	


	std::string currentProgFile; ///<Path of the current program file.
	int nextFreeObjNo;

	std::mt19937 randEngine; ///<The vm's own randon number engine.
	//std::shuffle_order_engine<std::mt19937, 256> randEngine;
	bool paused;
	bool capitaliseNext; ///<If true, capitaliseNext the next letter printed.


};

const int externalFunc = -1;