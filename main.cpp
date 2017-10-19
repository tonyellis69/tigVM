#define _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include<iostream>
#include "src\vm.h"
#include "src\vmConsole.h"

#if defined(_DEBUG)

#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW 
#endif

void getChoice(CTigVM* vm) {
	int choice = 1;
	std::vector<std::string> optionStrs;
	vm->getOptionStrs(optionStrs);
	for (auto optionStr : optionStrs) {
		std::cout << "\n" << choice << ": " << optionStr;
		choice++;
	}
	std::cout << "\n?";

	TVMmsg msg;
	msg.type = vmMsgChoice;
	msg.integer = getchar() - 49;
	int dummy = getchar();
	vm->sendMessage(msg); 
}

void getString(CTigVM* vm) {
	std::string string;
	std::cin >> string;
	int dummy = getchar();

	TVMmsg msg;
	msg.type = vmMsgString;
	msg.text = string;
	vm->sendMessage(msg);
}

int main() {
	CTigVM* vm = new CVMconsole();

	if (!vm->loadProgFile("d:\\projects\\TC\\output.tig")) {
		std::cout << "\nError opening file.";
		getchar();
		return 0;
	}

	vm->execute();

	while (vm->getStatus() != vmEnding) {
		if (vm->getStatus() == vmAwaitChoice) {
			getChoice(vm);
		}
		if (vm->getStatus() == vmAwaitString) {
			getString(vm);
		}
	}

	delete vm;
	getchar();

	_CrtDumpMemoryLeaks();
    return 0;
}

