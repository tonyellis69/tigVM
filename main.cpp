#define _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include<iostream>
#include "src\vm.h"

#if defined(_DEBUG)

#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW 
#endif

int getChoice(std::vector<std::string>& optionStrs) {
	int choice = 1;
	for (auto optionStr : optionStrs) {
		std::cout << "\n" << choice << ": " << optionStr;
		choice++;
	}
	std::cout << "\n?";
	int key = getchar() - 49;
	int dummy = getchar();
	return key;
}

int main() {
	CTigVM* vm = new CTigVM();

	if (!vm->loadProgFile("d:\\projects\\TC\\output.tig")) {
		std::cout << "\nError opening file.";
		getchar();
		return 0;
	}

	vm->execute();

	while (vm->getStatus() == vmAwaitChoice) {
		std::vector<std::string> optionStrs;
		vm->getOptionStrs(optionStrs);
		int choice = getChoice(optionStrs);
		vm->sendMessage(vmMsgChoice, choice);
	}

	delete vm;
	_CrtDumpMemoryLeaks();
    return 0;
}

