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
			case opPushStr: pushStr(); break;
			case opPrint : print(); break;
			case opOption: option(); break;
			case opEnd: end(); break;
			case opAssign: assign(); break;
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

/** Push a string onto the stack. */
void CTigVM::pushStr() {
	string text = readString();
	stack.push(text);
}

/** Pop the top value off the stack and print it. */
void CTigVM::print() {
	writeText(stack.top().getStringValue());
	stack.pop();
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

/** Pop the top value off the stack and assign it to a (global) variable. */
void CTigVM::assign() {
	int varId = readWord();
	globalVars[varId] = stack.top();
	stack.pop();
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
void CTigVM::sendMessage(TVMmsg msg, int msgInt) {
	//if we're being sent a user choice
	//and we're in awaiting choice mode
	//find the address of the chosen event
	//and resume execution there
	if (msg == vmMsgChoice && status == vmAwaitChoice) {
		int eventId = currentOptionList[msgInt].branchId;
		pc = eventTable[eventId - 1].address;
		execute();

	}
}
