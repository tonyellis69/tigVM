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


/** Remove the top element of the stack, freeing any memory. */
CTigVar CStack::pop() {
	/*if (stack.back().type == tigString) {
		delete stack.back().pStrValue;
	}
	stack.pop_back();*/
	CTigVar word = stack.back();
	stack.pop_back();
	return word;
}

/** Reserve space for the current frame's local variables. */
void CStack::reserveLocalVars(int varCount) {
	CTigVar currentLocalVarStart(localVarStart);
	stack.push_back(currentLocalVarStart);
	if (varCount > 0) {
		localVarStart = stack.size();
		stack.resize(stack.size() + varCount);
	}
	CTigVar varCountRef(varCount);
	stack.push_back(varCountRef);
}

/** Free space reserved for local variables. */
void CStack::freeLocalVars(int varCount) {
	stack.resize(stack.size() - varCount);
	localVarStart = pop().getIntValue();
}
