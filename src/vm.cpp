#include "vm.h"
#include "..\..\TC\src\sharedTypes.h"

#include <algorithm>    // std::find_if

using namespace std;

CTigVM::CTigVM() {
	escape = false; 
	time(&currentTime);
	status = vmNoProgram;
}

CTigVM::~CTigVM() {
	delete progBuf;
}

/** Load this compiled Tig program into memory. */
bool CTigVM::loadProgFile(std::string filename) {
	ifstream progFile(filename, ios::ate | ios::binary);
	if (!progFile.good()) {
		return false;
	}

	progBufSize = readHeader(progFile);

	progBuf = new char[progBufSize];
	progFile.read(progBuf, progBufSize);

	readEventTable(progFile);
	readGlobalVarTable(progFile);
	readObjectDefTable(progFile);
	readMemberNameTable(progFile);
	
	pc = globalCodeAddr;
	if (eventTable.size()) //assumes first event is the starting event
		pc = eventTable[0].address;
	status = vmExecuting;
	return true;
}

int CTigVM::readHeader(ifstream & progFile) {
	int headerSize = 8;
	int bufSize;
	progFile.seekg(0);
	progFile.read((char*)&bufSize, 4);
	progFile.read((char*)&globalCodeAddr, 4);
	return bufSize - headerSize;
}

void CTigVM::readEventTable(std::ifstream & progFile) {
	int eventTableSize;
	progFile.read((char*)&eventTableSize, 4);
	eventTable.resize(eventTableSize);
	progFile.read((char*)eventTable.data(), eventTableSize * sizeof(TEventRec));
}

void CTigVM::readGlobalVarTable(std::ifstream & progFile) {
	int globalVarTableSize;
	progFile.read((char*)&globalVarTableSize, 4);
	globalVarNameTable.resize(globalVarTableSize);
	int addr; std::string tmp;
	for (auto &globalVarNameRec : globalVarNameTable) {
		std::getline(progFile, tmp, '\0');
		globalVarNameRec.name = tmp;
		progFile.read((char*)&addr, 4);
		globalVarNameRec.id = addr;
	}
	globalVars.resize(globalVarTableSize);
}

/** Read the progfile object defintions and create an object for each one. */
void CTigVM::readObjectDefTable(std::ifstream & progFile) {
	int objectDefTableSize; char nMembers;
	progFile.read((char*)&objectDefTableSize, 4);
	for (int objNo = 0; objNo < objectDefTableSize; objNo++) {
		CObjInstance object;
		progFile.read((char*)&object.id, 4);
		progFile.read((char*)&object.classId, 4);
		if (object.classId != -1) {
			object.members = objects[object.classId].members;
		}

		progFile.read(&nMembers, 1); int memberId;
		for (int memberNo = 0; memberNo < nMembers; memberNo++) {
			CTigVar blank;
			progFile.read((char*)&memberId, 4);
			TigVarType type;
			char c;	int intValue; std::string buf;
			progFile.read(&c, 1);
			type = (TigVarType)c;
			switch (type) {
			case tigString:
				unsigned int size;
				progFile.read((char*)&size, 4);
				buf.resize(size);
				progFile.read(&buf[0], size);
				blank.setStringValue(buf); break;
			case tigFunc:
				progFile.read((char*)&intValue, 4);
				blank.setFuncAddr(intValue); break;
			case tigInt:
				progFile.read((char*)&intValue, 4);
				blank.setIntValue(intValue); break;
			case tigObj:
				progFile.read((char*)&intValue, 4);
				blank.setObjId(intValue); break;
			case tigUndefined:
				progFile.read((char*)&intValue, 4); //can just throw this away
				break;
			}
			object.members[memberId] = blank;
		}
		objects[object.id] = object;
	}
}

/** Read the list of member names from the program file and store them. */
void CTigVM::readMemberNameTable(std::ifstream & progFile) {
	int memberNameTableSize; char numMembers;
	progFile.read((char*)&memberNameTableSize, 4);
	memberNames.resize(memberNameTableSize);
	for (auto &name : memberNames) {
		std::getline(progFile, name, '\0');
	}
}

/** Execute the currently loaded program starting at the current program counter position. */
void CTigVM::execute() {
	escape = false;
	while (pc < progBufSize && status == vmExecuting) {
		int opCode = readNextOp();
		switch(opCode) {
			case opPushInt: pushInt(); break;
			case opPushStr: pushStr(); break;
			case opPushVar: pushVar(); break;
			case opPrint : print(); break;
			case opOption: option(); break;
			case opEnd: end(); break;
			case opAssign: assign(); break;
			case opGetString: getString(); break;
			case opAdd: add(); break;
			case opGiveOptions: giveOptions(); break;
			case opJumpEvent: jumpEvent(); break;
			case opStartTimer: startTimer(); break;
			case opTimedEvent: createTimedEvent(); break;
			case opPushObj:	pushObj(); break;
			case opCall: call(); break;
			case opReturn: returnOp(); break;
			case opHot: hot(); break;
		}
	}
	if (pc >= progBufSize)
		status = vmEof;
}

/** Update the real-time side of the VM, if needed. */
void CTigVM::update() {
	//see how much time has elapsed
	//update the timer

	time_t newTime;
	time(&newTime);
	int dtSeconds = (int)difftime(newTime, currentTime);
	currentTime = newTime;
	daemon.update(dtSeconds);
}


int CTigVM::readNextOp() {
	return progBuf[pc++];
}

/** Read a string starting at the program counter, advancing it. */
std::string  CTigVM::readString() {
	unsigned int size = readWord();
	string text(progBuf + pc, size);
	pc += size;
	return text;
}

/** Read a 4-byte integer starting at the program counter, advancing it. */
unsigned int CTigVM::readWord() {
	unsigned int nextInt = (unsigned char)progBuf[pc];
	nextInt += progBuf[pc + 1] << 8;
	nextInt += progBuf[pc + 2] << 16;
	nextInt += progBuf[pc + 3] << 24;
	pc += 4;
	return nextInt;
}

/** Read a single byte starting at the program counter, and advance it. */
char CTigVM::readByte() {
	return progBuf[pc++];
}


/** Push the following integer onto the stack. */
void CTigVM::pushInt() {
	int n = readWord();
	stack.push(n);
}

/** Push the following string onto the stack. */
void CTigVM::pushStr() {
	string text = readString();
	stack.push(text);
}

/** Push the value from a global variable (or member) onto the stack. */
void CTigVM::pushVar() {
	int varId = readWord();
	if (varId < memberIdStart)
		stack.push(globalVars[varId]);
	else {
		int objectId = stack.pop().getIntValue();
		stack.push(objects[objectId].members[varId]);
	}
}



/** Pop the top value off the stack and print it. */
void CTigVM::print() {
	writeText(stack.pop().getStringValue());
}

/** Record the following option for later processing. */
void CTigVM::option() {
	TOptionRec option;
	option.index = readByte();
	option.text = readString();
	currentOptionList.push_back(option);
}

/** *Give the user a selection of options to chose from. */
void CTigVM::giveOptions() {
	escape = true;
	status = vmAwaitChoice;
}

/** Exit this Tig program gracefully. */
void CTigVM::end() {
	escape = true;
	status = vmEnding;
}

/** Pop a value off the stack and assign it to a (global) variable. */
void CTigVM::assign() {
	CTigVar value = stack.pop(); 

	CTigVar var = stack.pop();
	if (var.type == tigObj) {
		std::cout << "\nError! Attempt to assign a value to an object.";
		escape = true;
		status = vmError;
		return;
	}
	int varId = var.getIntValue();


	if (varId < memberIdStart)
		globalVars[varId] = value;
	else { // it's an object member id
		int objectId = stack.pop().getIntValue();
		objects[objectId].members[varId] = value;
	}
}

/** Go into 'awaiting string from user' mode. */
void CTigVM::getString() {
	escape = true;
	status = vmAwaitString;
}

/** Pop the two top values of the stack, add and push the result. */
void CTigVM::add() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result;
	if (op1.type == tigString) {
		result.setStringValue(op1.getStringValue() + op2.getStringValue());
	}
	stack.push(result);
}

/** Jump execution to the following event, never to return. */
void CTigVM::jumpEvent() {
	int eventId = readWord();
	pc = eventTable[eventId ].address;
}

/** Start the daemon timer counting up from zero in seconds. */
void CTigVM::startTimer() {
	daemon.startTimer();
}

/** Tell the daemon to run the given event after the given delay. */
void CTigVM::createTimedEvent() {
	timedEvent event;
	int eventId = readWord();
	event.codeAddr = eventTable[eventId].address;
	event.delay = readWord();
	daemon.addEvent(event);
}

/** Push an object identifier onto the stack. */
void CTigVM::pushObj() {
	int objId = readWord();
	stack.pushObj(objId);
}

/** Exectute the member function currently on the stack. */
void CTigVM::call() {
	int addr = stack.pop().getFuncAddress();
	stack.push(pc);
	pc = addr;
}

/** Transfer execution to the address at the top of the stack. */
void CTigVM::returnOp() {
	pc = stack.pop().getIntValue();
}

/** Pass on text identified as hot for the user to do something with. */
void CTigVM::hot() {
	std::string text = readString();
	hotText(text, readWord());
}



TVMstatus CTigVM::getStatus() {
	return status;
}

void CTigVM::getOptionStrs(std::vector<std::string>& optionStrs) {
	for (auto option : currentOptionList) {
		optionStrs.push_back(option.text);
	}
}

/** Handle a message sent by the user. */
void CTigVM::sendMessage(const TVMmsg& msg) {
	if (msg.type == vmMsgChoice && status == vmAwaitChoice) {
		int choice = msg.integer;
		int optionIndex = currentOptionList[choice].index;
		int tableStart = pc;
		pc += optionIndex * sizeof(int);
		pc = tableStart + readWord();
		currentOptionList.clear();
		status = vmExecuting;
		execute(); //TO DO: maybe not execute here, where we can't guarantee a return to the user. Let the user choose when.
	}

	if (msg.type == vmMsgString && status == vmAwaitString) {
		//push the string on the stack and resume execution
		stack.push(std::string(msg.text));
		status = vmExecuting;
		execute();
	}
}

/* old way to do it, preserved for reference.
struct find_varName {
	std::string name;
	find_varName(std::string& name) : name(name) {}
	bool operator () (const TGlobalVarNameRec& nameRec) const	{
		return nameRec.name == name;
	}
}; */

/** Return the value of the named global variable, if found. */
CTigVar CTigVM::getGlobalVar(std::string varName) {
	//auto it = find_if(globalVarNameTable.begin(), globalVarNameTable.end(), find_varName(varName));
	auto it = find_if(globalVarNameTable.begin(), globalVarNameTable.end(), 
		[&](TGlobalVarNameRec& nameRec) { return nameRec.name == varName; });
	CTigVar var;
	if (it != globalVarNameTable.end())
		var = globalVars[it->id];
	return var;
}

/** Return a copy of the the named member. */
CTigVar CTigVM::getMember(CTigVar & obj, std::string fnName) {
	int memberNo = getMemberId(fnName);
	return objects[obj.getObjId()].members[memberNo];
}

/** Return the member id of the named member. */
int CTigVM::getMemberId(std::string& name) {
	auto it = find_if(memberNames.begin(), memberNames.end(),
		[&](std::string& currentName) { return currentName == name; });

	if (it == memberNames.end()) {
		std::cerr << "\nMember " << name << " not found.";
		return 0;
	}

	return it - memberNames.begin() + memberIdStart;
}

/** Execute the named object member. */
void CTigVM::ObjMessage(CTigVar & obj, std::string fnName) {
	CTigVar member = getMember(obj, fnName);
	if (member.type == tigString) { //or int or float 
		writeText(member.getStringValue());
		return;
	}
	if (member.type == tigFunc) {
		stack.push(pc); //because return from function pulls call address from stack
		pc = member.getFuncAddress();
		status = vmExecuting;
		execute();
		//TO DO: calling execute internally is not ideal. Might be better to just
		//assume execution gets handled by the caller.
	}
	//status = vmEnding;
}