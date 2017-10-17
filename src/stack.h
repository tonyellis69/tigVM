#pragma once

#include <string>
#include <vector>

#include "var.h"

/** The virtual machine stack. */
class CStack {
public:
	CStack() {};
	void push(std::string& string);
	CTigVar& top();
	void pop();

	std::vector<CTigVar> stack;
};