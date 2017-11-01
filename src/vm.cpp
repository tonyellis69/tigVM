#include "vm.h"
#include "..\..\TC\src\sharedTypes.h"
#include <fstream>
#include <iostream>

using namespace std;

CTigVM::~CTigVM() {
	delete progBuf;
}

/** Load this compiled Tig program into memory. */
bool CTigVM::loadProgFile(std::string filename) {
	ifstream progFile(filename, ios::ate | ios::binary);
	if (!progFile.good()) {
		return false;
	}

	//read header
	int headerSize = 4;
	progFile.seekg(0);
	progFile.read((char*)&progBufSize, 4);
	progBufSize -= headerSize;

	progBuf = new char[progBufSize];
	progFile.read(progBuf, progBufSize);

	//read event table
	int eventTableSize;
	progFile.read((char*)&eventTableSize, 4);
	eventTable.resize(eventTableSize);
	progFile.read((char*)eventTable.data(), eventTableSize * sizeof(TEventRec));

	//read global variable table
	int globalVarTableSize;
	progFile.read((char*)&globalVarTableSize, 4);
	globalVarNameTable.resize(globalVarTableSize);
	int addr; std::string tmp;
	for (auto globalVarNameRec : globalVarNameTable) {
		std::getline(progFile, tmp, '\0');
		globalVarNameRec.name = tmp;
		progFile.read((char*)&addr, 4);
		globalVarNameRec.id = addr;
	}
	globalVars.resize(globalVarTableSize);

	//read object definition table
	int objectDefTableSize; char nMembers;
	progFile.read((char*)&objectDefTableSize, 4);
	for (int objNo = 0; objNo < objectDefTableSize; objNo++) {
		CObjDef objDef;
		progFile.read((char*)&objDef.id, 4);
		progFile.read(&nMembers, 1); int memberId;
		
		CObjInstance object;
		object.classId = objDef.id;

		for (int memberNo = 0; memberNo < nMembers; memberNo++) {
			CTigVar blank;
			progFile.read((char*)&memberId, 4);
			TigVarType type;
			char c;
			progFile.read(&c, 1);
			type = (TigVarType) c;
			if (type == tigString) {
				unsigned int size;
				progFile.read((char*)&size, 4);
				std::string buf;
				buf.resize(size);
				progFile.read(&buf[0], size);
				blank.setStringValue(buf);
			}
			else {
				int intValue;
				progFile.read((char*)&intValue, 4);
				blank.setIntValue(intValue);
			}
			objDef.members[memberId] = blank;		
			object.members[memberId] = blank;
		}
		objectDefTable.push_back(objDef);
		objects[objDef.id] = object;

	}

	pc = 0;
	return true;
}

/** Execute the currently loaded program starting at the current program counter position. */
void CTigVM::execute() {
	escape = false;
	status = vmExecuting;
	//while not the end of the program buffer
	//get the next instruction and execute it.
	//while (pc < progBufSize && !escape) {
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
		}
	}
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





TVMstatus CTigVM::getStatus() {
	return status;
}

void CTigVM::getOptionStrs(std::vector<std::string>& optionStrs) {
	for (auto option : currentOptionList) {
		optionStrs.push_back(option.text);
	}
}

/** Handle the various messages the user can send. */
void CTigVM::sendMessage(TVMmsg& msg) {
	if (msg.type == vmMsgChoice && status == vmAwaitChoice) {
		int choice = msg.integer;
		int optionIndex = currentOptionList[choice].index;
		int tableStart = pc;
		pc += optionIndex * sizeof(int);
		pc = tableStart + readWord();
		currentOptionList.clear();
		execute();
	}

	if (msg.type == vmMsgString && status == vmAwaitString) {
		//push the string on the stack and resume execution
		stack.push(msg.text);
		execute();
	}
}
