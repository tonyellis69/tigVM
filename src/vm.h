#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <iostream>
#include <fstream>

#include "stack.h"
#include "daemon.h"

struct TEventRec {
	int id;
	int address;
};

struct TOptionRec {
	int index;///<Position among the total number of options for this event.
	std::string text; ///<Option text.
};

struct THotTExt {
	std::string text;
	int msgId;
	int objId;
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
	void jump();
	void jumpFalse();
	void child();
	void sibling();
	void getVar();
	void children();
	void makeHot();
	void brk();
	void move();
	void win();
	void clr();
	void style();
	void cap();
	void inherits();
	void hotClr();


	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(const TVMmsg& msg);
	virtual void writeText(std::string& text) {};
	virtual void hotText(std::string& text, int memberId, int objectId) {};
	virtual void purge(int memberId, int objId) {};
	virtual void clearWin() {};
	
	CTigVar getGlobalVar(std::string varName);
	CTigVar getMember(CTigVar & obj, int memberId);
	CTigVar getMember(int objNo, int memberId);
	CTigVar getMember(CTigVar& obj, std::string fnName);
	CTigVar getMember(CObjInstance* obj, int memberId);
	int getMemberId(std::string  name);
	CTigVar objMessage(CTigVar & obj, int memberId);
	CTigVar objMessage(int objNo, int memberId);
	CTigVar objMessage(int objNo, std::string fnName);
	CTigVar objMessage(CTigVar & obj, std::string fnName);
	CTigVar objMessage(CObjInstance * obj, std::string fnName);
	CTigVar executeObjMember(CTigVar & ObjMember);
	std::string getMemberName(int memberId);
	int getMemberValue(int objNo,int memberId);
	int getMemberValue(int objNo, std::string memberName);
	int getClass(int objNo);
	bool inheritsFrom(CObjInstance * obj, int classId);
	bool inheritsFrom(CObjInstance * obj, CObjInstance * classObj);

	void setMemberValue(int objNo, std::string memberName, CTigVar& value);

	bool hasMember(int objNo, int memberNo);
	bool hasMember(CObjInstance * obj, int memberNo);

	CObjInstance* getObject(int objId);
	int getObjectId(CObjInstance* obj);

	std::string devil(std::string text);

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
//	std::map<int,int> globalFuncs; ///<Addresses of global functions in id order.

	CStack stack; ///<The virtual machine stack.

	time_t currentTime;
	CDaemon daemon;

	int currentObject; ///<Owner of the function currently being executed.
	
	int childId; ///<Member id for the 'child' member
	int siblingId; ///<Member id for the 'sibling' member
	int parentId; ///<Member id for the 'parent' member

	int window; ///<The output we're currently writing to.

	std::vector<THotTExt> hotTexts; ///<The list of known hot texts, if any, for the devil to look out for.
};