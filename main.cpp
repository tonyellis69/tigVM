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

int main() {
	CTigVM* vm = new CTigVM();

	//read a compiled Tig file into memory
	if (!vm->loadProgFile("d:\\projects\\TC\\output.tig")) {
		std::cout << "\nError opening file.";
		getchar();
		return 0;
	}

	//execute it
	vm->execute();

	getchar();

	delete vm;
	_CrtDumpMemoryLeaks();
    return 0;
}

