#pragma once

#include <string>
#include <vector>

#include "var.h"

/** The virtual machine stack. */
class CStack {
public:
	CStack() {};
	void push(std::string& string);
	void push(int n);
	void push(CTigVar& var);
	void pushObj(int id);
	CTigVar& top();
	CTigVar pop();

	std::vector<CTigVar> stack;
};