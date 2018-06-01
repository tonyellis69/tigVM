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

struct TGlobalVarNameRec {
	std::string name; 
	int id;
};

class CObjInstance {
public:
	CObjInstance() { id = 0; classId = 0; };
	int id;
	int classId;
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
	int readHeader(std::ifstream& progFile);
	void readEventTable(std::ifstream& progFile);
	void readGlobalVarTable(std::ifstream& progFile);
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
	void jumpEvent();
	void startTimer();
	void createTimedEvent();
	void pushObj();
	void call();
	void returnOp();
	void returnTrue();
	void hot();
	void initArray();
	void pushElem();
	void assignElem();
	void compEq();

	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(const TVMmsg& msg);
	virtual void writeText(std::string& text) {};
	virtual void hotText(std::string& text, int memberId) {};
	
	CTigVar getGlobalVar(std::string varName);
	CTigVar getMember(CTigVar & obj, int memberId);
	CTigVar getMember(int objNo, int memberId);
	CTigVar getMember(CTigVar& obj, std::string fnName);
	int getMemberId(std::string  name);
	CTigVar objMessage(CTigVar & obj, int memberId);
	CTigVar objMessage(int objNo, int memberId);
	CTigVar objMessage(int objNo, std::string fnName);
	CTigVar objMessage(CTigVar & obj, std::string fnName);
	CTigVar executeObjMember(CTigVar & ObjMember);
	std::string getMemberName(int memberId);
	int getMemberValue(int objNo,int memberId);
	int getClass(int objNo);
	int getObjectId(std::string objName);

	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
	int globalCodeAddr; 
	bool escape; ///<Set to true to escape the normal execution loop.
	TVMstatus status; ///<Current mode.

	std::vector<TEventRec> eventTable; ///<Event ids and addresses.
	std::vector<TOptionRec> currentOptionList; ///<A list of user options
	std::vector<TGlobalVarNameRec> globalVarNameTable; ///<Global variable names and ids.
	std::vector<CTigVar> globalVars; ///<Global variable storage.
	std::map<int, CObjInstance> objects; ///<Object instances.
	std::vector<std::string> memberNames; ///<Member names in id number order.

	CStack stack; ///<The virtual machine stack.

	time_t currentTime;
	CDaemon daemon;
};