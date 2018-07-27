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
	CTigVar& operator[] (int x) {
		return stack[x];
	}
	int size() { return stack.size(); }
	CTigVar& top(int x) {
		return stack[stack.size() - 1 + x];
	}
	CTigVar pop();
	CTigVar& local(int var);
	

	void reserveLocalVars(int varCount);
	void freeLocalVars(int varCount);

	std::vector<CTigVar> stack;
	int localVarStart; ///<Index of first local variable in this frame.
};