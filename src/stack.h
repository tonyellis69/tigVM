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
	void push(float n);
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
	CTigVar& top() {
		return stack[stack.size() - 1];
	}
	CTigVar pop();
	CTigVar& local(int var);
	

	void reserveLocalVars(int varCount, int paramCount);
	void freeLocalVars();
	void removeTop(int count) {
		unsigned int currentSize = stack.size();
		stack.resize(currentSize - count);
	}
	void pushReturnAddress(int returnObj, int addr) {
		push(returnObj);
		push(addr);
	}
	std::tuple<int, int> popReturnAddress() {
		int addr = pop().getIntValue();
		int obj = pop().getIntValue();
		return { addr,obj };
	}

	std::vector<CTigVar> stack;
	int localVarStart; ///<Index of first local variable in this frame.

};