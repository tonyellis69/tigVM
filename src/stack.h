#pragma once

#include <string>
#include <vector>

#include "var.h"

/** The virtual machine stack. */
class CStack {
public:
	CStack() { localVarStart = 0; };
	void push(std::string& string);
	void push(int n);
	void push(CTigVar& var);
	void pushObj(int id);
	void pushUndefined();
	CTigVar& top();
	CTigVar pop();
	CTigVar& local(int var) { return stack[localVarStart + var]; }
	

	void reserveLocalVars(int varCount);
	void freeLocalVars(int varCount);

	std::vector<CTigVar> stack;
	int localVarStart; ///<Index of first local variable in this frame.
};