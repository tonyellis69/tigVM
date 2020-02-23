#include "stack.h"

/** Push a string value onto the stack. */
void CStack::push(std::string& string) {
	CTigVar word;
	word.setStringValue(string);
	stack.push_back(word);
}

void CStack::push(int n) {
	CTigVar word;
	word.setIntValue(n);
	stack.push_back(word);
}

void CStack::push(float n) {
	CTigVar word;
	word.setFloatValue(n);
	stack.push_back(word);
}

void CStack::push(CTigVar & var) {
	stack.push_back(var);
}

void CStack::pushObj(int id) {
	CTigVar word;
	word.setObjId(id);
	stack.push_back(word);

}

void CStack::pushUndefined() {
	CTigVar fail;
	fail.type = tigUndefined;
	stack.push_back(fail);
}


/** Remove the top element of the stack, and return a copy. */
CTigVar CStack::pop() {
	/*if (stack.back().type == tigString) {
		delete stack.back().pStrValue;
	}
	stack.pop_back();*/
	CTigVar word = stack.back();
	stack.pop_back();
	return word;
}

CTigVar & CStack::local(int var) {
	return stack[localVarStart + var]; 
}

/** Reserve space for the current frame's local variables. */
void CStack::reserveLocalVars(int varCount, int paramCount) {
	int oldLocalVarStart = localVarStart;
	localVarStart = stack.size() - paramCount;
	if (varCount > 0) {
		stack.resize(stack.size() + (varCount-paramCount));
	}

	stack.push_back(oldLocalVarStart);
}

/** Free space reserved for local variables. */
void CStack::freeLocalVars() {
	int oldLocalVarStart = pop().getIntValue();

	stack.resize(localVarStart);
	localVarStart = oldLocalVarStart;
}
