#include "stack.h"

/** Push a string value onto the stack. */
void CStack::push(std::string& string) {
	CTigVar word;
	word.setStringValue(string);
	stack.push_back(word);
}

/** Return the top value on the stack. */
CTigVar & CStack::top() {
	return stack.back();
}

/** Remove the top element of the stack, freeing any memory. */
void CStack::pop() {
	if (stack.back().type == tigString) {
		delete stack.back().pStrValue;
	}
	stack.pop_back();
}
