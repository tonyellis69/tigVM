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

	pc = 0;
	return true;
}

/** Execute the currently loaded program starting at the current program counter position. */
void CTigVM::execute() {

	escape = false;
	status = vmExecuting;
	//while not the end of the program buffer
	//get the next instruction and execute it.
	while (pc < progBufSize && !escape) {
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
		}
	}
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

/** Push the value from a global variable onto the stack. */
void CTigVM::pushVar() {
	int varId = readWord();
	stack.push(globalVars[varId]);
}



/** Pop the top value off the stack and print it. */
void CTigVM::print() {
	writeText(stack.pop().getStringValue());
}

/**	Handle a user option instruction. */
void CTigVM::option() {
	//read options into an option structure
	currentOptionList.clear();
	int nOptions = readByte();
	currentOptionList.resize(nOptions);
	for (int option = 0; option < nOptions; option++) {
		currentOptionList[option].text = readString();
		currentOptionList[option].branchId = readWord();
	}

	//go into suspended mode
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
	int varId = stack.pop().getIntValue();
	globalVars[varId] = value;
}

/** Go into 'awaiting string from user' mode. */
void CTigVM::getString() {
	escape = true;
	status = vmAwaitString;
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
		int eventId = currentOptionList[choice].branchId;
		pc = eventTable[eventId - 1].address;
		execute();
	}

	if (msg.type == vmMsgString && status == vmAwaitString) {
		//push the string on the stack and resume execution
		stack.push(msg.text);
		execute();
	}

}
