#pragma once

#include <string>
#include <vector>

#include "stack.h"

struct TEventRec {
	int id;
	int address;
};

struct TOptionRec {
	std::string text; ///<Option text.
	int branchId; ///<The event it leads to.
};

struct TGlobalVarNameRec {
	std::string name; 
	int id;
};

enum TVMstatus { vmExecuting, vmAwaitChoice, vmEnding };
enum TVMmsg { vmMsgChoice };

/** The Tig virtual machine. Reads compiled Tig code and executes it. */
class CTigVM {
public:
	CTigVM() { escape = false; };
	~CTigVM();
	bool loadProgFile(std::string filename);
	void execute();
	int readNextOp();
	std::string readString();
	unsigned int readWord();
	char readByte();

	void pushInt();
	void pushStr();
	void pushVar();
	void print();
	void option();
	void end();

	void assign();

	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(TVMmsg msg, int msgInt);
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

	CStack stack; ///<The virtual machine stack.
};