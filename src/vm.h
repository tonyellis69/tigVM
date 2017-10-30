#pragma once

#include <string>
#include <vector>
#include <map>
#include <ctime>

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

class CObjDef {
public:
	CObjDef() {};
	int id;
	std::vector<int> members;
};


class CObjInstance {
public:
	CObjInstance() {};
	int classId;
	std::map<int, CTigVar> members;
};

enum TVMmsgType { vmMsgChoice, vmMsgString };
struct TVMmsg {
	TVMmsgType type;
	std::string text;
	int integer;
};

enum TVMstatus { vmExecuting, vmAwaitChoice, vmAwaitString, vmEnding, vmError };

/** The Tig virtual machine. Reads compiled Tig code and executes it. */
class CTigVM {
public:
	CTigVM() { escape = false; time(&currentTime); };
	~CTigVM();
	bool loadProgFile(std::string filename);
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

	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(TVMmsg& msg);
	virtual void writeText(std::string& text) {};

	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
	bool escape; ///<Set to true to escape the normal execution loop.
	TVMstatus status; ///<Current mode.

	std::vector<TEventRec> eventTable; ///<Event ids and addresses.
	std::vector<TOptionRec> currentOptionList; ///<A list of user options
	std::vector<TGlobalVarNameRec> globalVarNameTable; ///<Global variable names and ids.
	std::vector<CTigVar> globalVars; ///<Global variable storage.
	std::vector<CObjDef> objectDefTable; ///<Object class definitions.
	std::map<int, CObjInstance> objects; ///<Object instances.

	CStack stack; ///<The virtual machine stack.

	time_t currentTime;
	CDaemon daemon;
};