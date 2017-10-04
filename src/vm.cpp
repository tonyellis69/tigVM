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

	progBufSize = progFile.tellg();
	progFile.seekg(0);
	progBuf = new char[progBufSize];

	progFile.read(progBuf, progBufSize);

	return true;
}

/** Execute the currently loaded program. */
void CTigVM::execute() {
	pc = 0;

	//while not the end of the program buffer
	//get the next instruction and execute it.
	while (pc < progBufSize) {
		int opCode = readNextOp();

		switch(opCode) {
		case opPrint: print(); break;

		}
	}
}

int CTigVM::readNextOp() {
	return progBuf[pc++];
}

std::string  CTigVM::readString() {
	int size = readInt();
	string text(progBuf + pc, size);
	pc += size;
	return text;
}

/** Read an integer at the program counter, advancing it. */
int CTigVM::readInt() {
	int nextInt = progBuf[pc];
	pc += 4;
	return nextInt;
}


void CTigVM::print() {
	string text = readString();
	cout << "\n" << text.c_str();
}
