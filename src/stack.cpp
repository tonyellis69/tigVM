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

/** Return the top value on the stack. */
CTigVar & CStack::top() {
	return stack.back();
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
