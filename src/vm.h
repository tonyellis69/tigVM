#pragma once

#include <string>
#include <vector>

struct TEventRec {
	int id;
	int address;
};

struct TOptionRec {
	std::string text; ///<Option text.
	int branchId; ///<The event it leads to.
};

enum TVMstatus { vmExecuting, vmAwaitChoice };
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
	int readInt();
	char readByte();
	
	void print();
	void option();

	TVMstatus getStatus();
	void getOptionStrs(std::vector<std::string>& optionStrs);
	void sendMessage(TVMmsg msg, int msgInt);


	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
	bool escape; ///<Set to true to escape the normal execution loop.
	TVMstatus status; ///<Current mode.

	std::vector<TEventRec> eventTable; ///<Event ids and addresses.
	std::vector<TOptionRec> currentOptionList; ///<A list of user options
};