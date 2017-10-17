#pragma once

#include "vm.h"

class CVMconsole : public CTigVM {
public:
	CVMconsole() {};
	void writeText(std::string& text);
};