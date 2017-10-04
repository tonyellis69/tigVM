#pragma once

#include <string>

/** The Tig virtual machine. Reads compiled Tig code and executes it. */

class CTigVM {
public:
	CTigVM() {};
	~CTigVM();
	bool loadProgFile(std::string filename);
	void execute();
	int readNextOp();
	std::string readString();
	int readInt();


	void print();

	int progBufSize;
	char* progBuf; 
	int pc; ///<Program counter. Points at the next command to execute.
};