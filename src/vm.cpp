#include "vm.h"
#include "..\..\TC\src\sharedTypes.h"

#include "..\3DEngine\src\utils\log.h"

#include <algorithm>    // std::find_if
#include <regex>

using namespace std;

CTigVM::CTigVM() {
	escape = false;
	time(&currentTime);
	status = vmNoProgram;
	window = 0;
	paused = false;
	capitaliseNext = false;
}

CTigVM::~CTigVM() {
	delete progBuf;
}

/** Load this compiled Tig program into memory. */
bool CTigVM::loadProgFile(std::string filename) {
	ifstream progFile(filename, ios::ate | ios::binary);
	if (!progFile.good()) {
		return false;
	}
	currentProgFile = filename;

	progBufSize = readHeader(progFile);
	progBuf = new char[progBufSize];
	progFile.read(progBuf, progBufSize);

	readEventTable(progFile);
	readObjectDefTable(progFile);
	readMemberNameTable(progFile);


	initIds();

	pc = globalCodeAddr;
//	if (eventTable.size()) //assumes first event is the starting event
	//	pc = eventTable[0].address;
	status = vmExecuting;
	currentObject = 0;

	randEngine.seed(740608);
	randEngine.discard(700000);
	//TO DO: Tig code should be able to overrule this with its own seed.

	return true;
}

void CTigVM::initIds() {
	childId = getMemberId("child");
	siblingId = getMemberId("sibling");
	parentId = getMemberId("parent");
	flagsId = getMemberId("#flags");
}

int CTigVM::readHeader(ifstream & progFile) {
	int headerSize = 8;
	int bufSize;
	progFile.seekg(0);
	progFile.read((char*)&bufSize, 4);
	progFile.read((char*)&globalCodeAddr, 4);
	return bufSize - headerSize;
}

void CTigVM::readEventTable(std::ifstream & progFile) {
	int eventTableSize;
	progFile.read((char*)&eventTableSize, 4);
	eventTable.resize(eventTableSize);
	progFile.read((char*)eventTable.data(), eventTableSize * sizeof(TEventRec));
}



/** Read the progfile object defintions and create an object for each one. */
void CTigVM::readObjectDefTable(std::ifstream & progFile) {
	int objectDefTableSize; char nMembers;
	nextFreeObjNo = 0;
	progFile.read((char*)&objectDefTableSize, 4);
	for (int objNo = 0; objNo < objectDefTableSize; objNo++) { //for each object...
		CObjInstance object;
		progFile.read((char*)&object.id, 4);

		char noParentClasses;
		progFile.read((char*)&noParentClasses, 1);
		for (int parentClass = 0; parentClass < noParentClasses; parentClass++) {
			int classId;
			progFile.read((char*)&classId, 4);
			object.classIds.push_back(classId);
			for (auto member : objects[classId].members) { //for each parent class member...
				object.members[member.first] = member.second;
			}
		}


		progFile.read(&nMembers, 1); int memberId;
		for (int memberNo = 0; memberNo < nMembers; memberNo++) {
			CTigVar blank;
			progFile.read((char*)&memberId, 4);
			TigVarType type;
			char c;	int intValue; std::string buf;
			progFile.read(&c, 1);
			type = (TigVarType)c;
			switch (type) {
			case tigString:
				unsigned int size;
				progFile.read((char*)&size, 4);
				buf.resize(size);
				progFile.read(&buf[0], size);
				blank.setStringValue(buf); break;
			case tigFunc:
				progFile.read((char*)&intValue, 4);
				blank.setFuncAddr(intValue); break;
			case tigInt:
				progFile.read((char*)&intValue, 4);
				blank.setIntValue(intValue); break;
			case tigObj:
				progFile.read((char*)&intValue, 4);
				blank.setObjId(intValue); break;
			case tigUndefined:
				progFile.read((char*)&intValue, 4); //can just throw this away
				break;
			case tigArray:
				unsigned int arraySize;
				progFile.read((char*)&arraySize, 4);
				if (arraySize > 0) {
					blank.setArray();
					blank.pArray->elements.resize(arraySize);
					for (auto &element : blank.pArray->elements) {
						char ec; 
						progFile.read(&ec, 1);
						TigVarType elemType = (TigVarType)ec; std::string strBuf;
						switch (elemType) {
						case tigString:
							unsigned int strSize;
							progFile.read((char*)&strSize, 4);
							strBuf.resize(strSize);
							progFile.read(&strBuf[0], strSize);
							element.setStringValue(strBuf); break;
						case tigInt:
							progFile.read((char*)&intValue, 4);
							element.setIntValue(intValue); break;
						case tigObj:
							progFile.read((char*)&intValue, 4);
							element.setObjId(intValue); break;
						case tigUndefined:
							progFile.read((char*)&intValue, 4); //can just throw this away
							break;
						}
					}

				}
				break;
			}
			object.members[memberId] = blank;
		}
		objects[object.id] = object;
		nextFreeObjNo++;
	}
}

/** Read the list of member names from the program file and store them. */
void CTigVM::readMemberNameTable(std::ifstream & progFile) {
	int memberNameTableSize; 
	progFile.read((char*)&memberNameTableSize, 4);
	memberNames.resize(memberNameTableSize);
	for (auto &name : memberNames) {
		std::getline(progFile, name, '\0');
	}
}


/** Execute the currently loaded program starting at the current program counter positionHint. */
void CTigVM::execute() {
	escape = false;
	while (pc < progBufSize && status == vmExecuting) {
		int opCode = readNextOp();
		switch (opCode) {
		case opPushInt: pushInt(); break;
		case opPushStr: pushStr(); break;
		case opPushVar: pushVar(); break;
		case opPrint: print(); break;
		case opOption: option(); break;
		case opEnd: end(); break;
		case opAssign: assign(); break;
		case opGetString: getString(); break;
		case opAdd: add(); break;
		case opSub: sub(); break;
		case opMod: mod(); break;
		case opMinus: minus(); break;
		case opGiveOptions: giveOptions(); break;
		case opJumpEvent: jumpEvent(); break;
		case opStartTimer: startTimer(); break;
		case opTimedEvent: createTimedEvent(); break;
		case opPushObj:	pushObj(); break;
		case opPushSelf: pushSelf(); break;
		case opCall: call(); break;
		case opCallDeref: callDeref(); break;
		case opSuperCall: superCall(); break;
		case opReturn: returnOp(); break;
		case opReturnTrue: returnTrue(); break;
		case opHot: hot(); break;
		case opPurge: purge(); break;
		case opInitArray: initArray(); break;
		case opPushElem: pushElem(); break;
		case opAssignElem: assignElem(); break;
		case opArrayIt: arrayIt(); break;
		case opPop: pop(); break;
		case opEq: compEq(); break;
		case opNE: compNE(); break;
		case opLT: compLT(); break;
		case opLE: compLE(); break;
		case opGT: compGT(); break;
		case opGE: compGE(); break;
		case opJump: jump(); break;
		case opJumpFalse: jumpFalse(); break;
		case opChild: child(); break;
		case opSibling: sibling(); break;
		case opIn: in(); break;
		case opNotIn: notIn(); break;
		case opGetVar: getVar(); break;
		case opChildren: children(); break;
		case opMakeHot: makeHot(); break;
		case opMakeHotAlt: makeHotAlt(); break;
		case opBrk: brk(); break;
		case opMove: move(); break;
		case opOpenWin: openWin(); break;
		case opWin: win(); break;
		case opClr: clr(); break;
		case opClrMarked: clrMarked(); break;
		case opStyle: style(); break;
		case opCap: cap(); break;
		case opCapNext: capNext(); break;
		case opInherits: inherits(); break;
		case opHotClr: hotClr(); break;
		case opHotCheck: hotCheck(); break;
		case opNot: not(); break;
		case opAnd: and(); break;
		case opOr: or(); break;
		case opArrayPush: arrayPush(); break;
		case opArrayRemove: arrayRemove(); break;
		case opMsg: msg(); break;
		case opHas: has(); break; 
		case opMatch: match(); break;
		case opIs: is(); break;
		case opIsNot: isNot(); break;
		case opSet: set(); break;
		case opUnset: unset(); break;
		case opNew: newOp(); break;
		case opDelete: deleteOp(); break;
		case opFinalLoop: finalLoop(); break;
		case opFirstLoop: firstLoop(); break;
		case opRoll: roll(); break;
		case opSortDesc: sortDesc(); break;
		case opLog: log(); break;
		case opPause: pause(); break;
		case opRand: rand(); break;
		}
	}
	if (pc >= progBufSize)
		status = vmEof;
}

/** Update the real-time side of the VM, if needed. */
void CTigVM::update() {
	//see how much time has elapsed
	//update the timer

	time_t newTime;
	time(&newTime);
	int dtSeconds = (int)difftime(newTime, currentTime);
	currentTime = newTime;
	daemon.update(dtSeconds);
}


int CTigVM::readNextOp() {
	return progBuf[pc++];
}

/** Read a string starting at the program counter, advancing it. */
std::string  CTigVM::readString() {
	unsigned int size = readWord();
	string text(progBuf + pc, size);
	pc += size;
	return text;
}

/** Read a 4-byte integer starting at the program counter, advancing it. */
unsigned int CTigVM::readWord() {
	unsigned int nextInt = (unsigned char)progBuf[pc];
	nextInt += progBuf[pc + 1] << 8;
	nextInt += progBuf[pc + 2] << 16;
	nextInt += progBuf[pc + 3] << 24;
	 //int nextInt = (int&)progBuf[pc];
	pc += 4;
	return nextInt;
}

/** Read a single byte starting at the program counter, and advance it. */
char CTigVM::readByte() {
	return progBuf[pc++];
}


/** Push the following integer onto the stack. */
void CTigVM::pushInt() {
	int n = readWord();
	stack.push(n);
}

/** Push the following string onto the stack. */
void CTigVM::pushStr() {
	string text = readString();
	stack.push(text);
}

/** Push the value from a global variable (or member) onto the stack. */
void CTigVM::pushVar() {
	int varId = readWord(); 
	CTigVar* pVar = resolveVariableAddress(varId);

	if (pVar == NULL) {
		stack.pushUndefined();
		return;
	}

	stack.push(*pVar);
}



/** Pop the top value off the stack and print it. */
void CTigVM::print() {
	std::string text = stack.pop().getStringValue();
	text = devil(text);
	if (text.size() == 0)
		return;
	
	latestText = text;
	writeText(latestText);
}

/** Record the following option for later processing. */
void CTigVM::option() {
	TOptionRec option;
	option.index = readByte();
	option.text = readString();
	currentOptionList.push_back(option);
}

/** *Give the user a selection of options to chose from. */
void CTigVM::giveOptions() {
	escape = true;
	status = vmAwaitChoice;
}

/** Exit this Tig program gracefully. */
void CTigVM::end() {
	escape = true;
	status = vmEnding;
}

/** Pop a variable off the stack, pop a value off the stack, assign the latter to the former. */
void CTigVM::assign() {
	int varId = stack.pop().getIntValue();
	CTigVar* pVar = resolveVariableAddress(varId);

	if (!pVar)
		return;
	
	/*if (pVar->type == tigArray) {
		int index = stack.pop().getIntValue();
		pVar->pArray->elements[index] = stack.pop();
	}
	else
		*pVar = stack.pop();*/

		*pVar = stack.pop();

}


CTigVar* CTigVM::resolveVariableAddress(int varId) {
	int objDiscard;
	return resolveVariableAddress(varId, objDiscard);
}

CTigVar* CTigVM::resolveVariableAddress(int varId, int& owningObject) {
	CTigVar* pVar = NULL; int objectId = NULL; 

	if (varId < memberIdStart) {
		return &stack.local(varId);  //it's a local variable
	}

	//not a local variable...
	objectId = stack.pop().getIntValue(); //...so get the parent object
	owningObject = objectId;
	if (objectId == zeroObject) { //it's either a local member or global
		owningObject = currentObject;
		auto it = objects[currentObject].members.find(varId);
		if (it != objects[currentObject].members.end()) { //local member;
			return &it->second;;
		}
		else {
			return &objects[zeroObject].members[varId]; //global variable
			//NB if the global variable doesn't exist this will create it. It's safe because Tig syntax 
			//forbids reading from an undeclared global variable
		}
	}

	//still here? Then it's the member of an object
	if (objects.find(objectId) == objects.end()) { //does the object exist?
		liveLog << "\nError! Attempt to access non-existent object " << objectId << " via member " << getMemberName(varId);
		return NULL;
	}

	auto it2 = objects[objectId].members.find(varId); //does the object have this member?
	if (it2 != objects[objectId].members.end()) {
		return &it2->second;
	}

	liveLog << "\nNon-existent member " << getMemberName(varId) << " called on object " << objectId;
	return NULL;
}


/** Go into 'awaiting string from user' mode. */
void CTigVM::getString() {
	escape = true;
	status = vmAwaitString;
}

/** Pop the two top values off the stack, add, and push the result. */
void CTigVM::add() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result(tigUndefined);
	if (op1.type == tigString) {
		result.setStringValue(op1.getStringValue() + op2.getStringValue());
	}
	if (op1.type == tigInt) {
		int intResult = op1.getIntValue();
		if (op2.type == tigInt)
			intResult += op2.getIntValue();
		else if (op2.type == tigFloat)
			intResult += (int)op2.getFloatValue();
		else if (op2.type == tigString) {
			//intResult += std::stoi(op2.getStringValue());
			stack.push(op1.getStringValue() + op2.getStringValue());
			return;
		}
		else if (op2.type == tigObj) {
			stack.push(result);
			return;
		}
		result.setIntValue(intResult);
	}
	if (op1.type == tigFloat) {
		float floatResult = op1.getFloatValue();
		if (op2.type == tigInt)
			floatResult += op2.getIntValue();
		else if (op2.type == tigFloat)
			floatResult += op2.getFloatValue();
		else if (op2.type == tigString)
			floatResult += std::stof(op2.getStringValue());
		else if (op2.type == tigObj) {
			stack.push(result);
			return;
		}
		result.setFloatValue(floatResult);
	}
	if (op1.type == tigArray) {
		result = op1;
		result.pArray->elements.push_back(op2);
	}
	stack.push(result);
}

/** Pop the top two values off the stack, subtract, and push the result. */
void CTigVM::sub() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result(tigUndefined);

	if (op1.type == tigFloat) {
		result = op1.getFloatValue() - op2.getFloatValue();
	}
	else {
		result = op1.getIntValue() - op2.getIntValue();
		}
	stack.push(result);
}

/** Pop the top two values off the stack, mod, and push the result. */
void CTigVM::mod() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result(tigUndefined);

	if (op1.type == tigFloat) {
		result = fmod(op1.getFloatValue(), op2.getFloatValue());
	}
	else {
		result = op1.getIntValue() % op2.getIntValue();
	}
	stack.push(result);
}

void CTigVM::minus() {
	stack.top(0) = -stack.top(0);
}

/** Jump execution to the following event, never to return. */
void CTigVM::jumpEvent() {
	int eventId = readWord();
	pc = eventTable[eventId].address;
}

/** Start the daemon timer counting up from zero in seconds. */
void CTigVM::startTimer() {
	daemon.startTimer();
}

/** Tell the daemon to run the given event after the given delay. */
void CTigVM::createTimedEvent() {
	timedEvent event;
	int eventId = readWord();
	event.codeAddr = eventTable[eventId].address;
	event.delay = readWord();
	daemon.addEvent(event);
}

/** Push an object identifier onto the stack. */
void CTigVM::pushObj() {
	int objId = readWord();
	//if (objId == selfObjId)
	//	objId = currentObject;
	stack.pushObj(objId);
}

void CTigVM::pushSelf() {
	stack.pushObj(currentObject);
}

/** Execute the member function currently on the stack, leaving the returned value on the stack. */
void CTigVM::call() {
	int memberId = readWord();
	int paramCount = readByte();
	CTigVar* callee = NULL; 
	int obj = 0;

	callee = resolveVariableAddress(memberId, obj);

	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}

	if (callee == NULL) {
		stack.pushUndefined();
		return;
	}

	if (callee->type != tigFunc) {
		stack.push(*callee);
		return;
	}

	stack.push(currentObject);
	stack.push(pc); //save return address

	currentObject = obj;

	pc = callee->getFuncAddress();

	int varCount = readByte();
	stack.reserveLocalVars(varCount);

	for (int p = 0; p < paramCount; p++) {
		stack.local(p) = params[(paramCount - 1) - p];
	}
}



void CTigVM::callDeref() {
	int memberId = stack.pop().getIntValue();
	int paramCount = readByte();
	CTigVar* callee = NULL; int obj = 0;

	callee = resolveVariableAddress(memberId, obj);

	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}

	if (callee == NULL) {
		stack.pushUndefined();
		return;
	}

	if (callee->type != tigFunc) {
		stack.push(*callee);
		return;
	}

	stack.push(currentObject);
	stack.push(pc); //save return address

	currentObject = obj;

	pc = callee->getFuncAddress();

	int varCount = readByte();
	stack.reserveLocalVars(varCount);

	for (int p = 0; p < paramCount; p++) {
		stack.local(p) = params[(paramCount - 1) - p];
	}

}

/** Call the superclass version of the function. */
void CTigVM::superCall() {
	int superClassId = stack.pop().getObjId();
	int memberId = readWord();
	int paramCount = readByte();

	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}

	auto fnMember = objects[superClassId].members.find(memberId);
	if (fnMember == objects[superClassId].members.end()) {
		cerr << "\nError! Attempt to superclass member function " << memberId <<
			" to object " << " superClassId.";
		return;
	}

	stack.push(currentObject);
	stack.push(pc); //save return address

	pc = fnMember->second.getFuncAddress();
	
	int varCount = readByte();
	stack.reserveLocalVars(varCount);

	for (int p = 0; p < paramCount; p++) {
		stack.local(p) = params[(paramCount - 1) - p];
	}
}



/** Return execution to the calling address left on the stack, leaving the current top value at the top. */
void CTigVM::returnOp() {
	CTigVar value = stack.pop();
	int localVarCount = stack.pop().getIntValue();
	stack.freeLocalVars(localVarCount);
	pc = stack.pop().getIntValue(); //get calling address
	currentObject = stack.pop().getIntValue(); //get calling object
	stack.push(value);
	if (pc == NULL)
		status = vmEof;
}

/** Return execution to the calling address left on the stack, leaving 1 at the top. */
void CTigVM::returnTrue() {
	int localVarCount = stack.pop().getIntValue();
	stack.freeLocalVars(localVarCount);
	pc = stack.pop().getIntValue(); //get calling address
	currentObject = stack.pop().getIntValue(); //get calling object
	stack.push(1);
	if (pc == NULL)
		status = vmEof;
}

/** Pass on text identified as hot for the user to do something with. */
void CTigVM::hot() {
	int paramCount = readByte();
	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
	std::reverse(params.begin(),params.end());

	THotTExt hotKeyword;
	hotKeyword.objId = stack.pop().getIntValue();
	hotKeyword.msgId = stack.pop().getIntValue();

	std::string text = stack.pop().getStringValue();
	std::transform(text.begin(), text.end(), text.begin(), ::tolower);
	hotKeyword.text = text;

	for (auto param : params) {
		hotKeyword.params.push_back(param);
	}
	hotKeyword.used = false;

	hotTextKeywords.push_back(hotKeyword);
}

/** Ask the user to remove any hot text with the given values. */
void CTigVM::purge() {
	int objId = stack.pop().getObjId();
	int memberId = stack.pop().getIntValue();
	if (objId == NULL && memberId == NULL) { //purge all
		purge(0);
		return;
	}

	//otherwise, find all hot text function calls using that obj/member combo...
	vector<unsigned int> hotFnIds;
	for (auto fnCall : hotTextFnCalls.container) {
		if (fnCall.second.options[0].msgId == memberId && fnCall.second.options[0].objId == objId)
			hotFnIds.push_back(fnCall.first);
	}

	//... and pass the ids on to the user
	for (auto id : hotFnIds )
		purge(id);
}

/** Initialise an array structure, leaving a Tig value referencing it on the stack. */
void CTigVM::initArray() {
	CTigVar newArray;
	newArray.type = tigArray;
	newArray.setArray();
	int arraySize = readWord();
	newArray.pArray->elements.resize(arraySize);
	//for (auto &element : newArray.pArray->elements) {
	for (int x= arraySize-1; x >= 0; x--) {
		newArray.pArray->elements[x] = stack.pop();
	}
	stack.push(newArray);
	return;
}

/** Push the value in the given array element onto the stack for further processing. */
void CTigVM::pushElem() {
	int varId = stack.pop().getIntValue();

	CTigVar* pArrayVar = NULL;;
	int tmp;
	pArrayVar = resolveVariableAddress(varId);
	int index = stack.pop().getIntValue();
	

	if (pArrayVar == NULL) {
		cerr << "Error! Unrecognised variable " << varId << " used as element identifier in an expression, index "
			<< index;
		stack.pushUndefined();
		return;
	}

	if (pArrayVar->type == tigArray) {
		if (index >= pArrayVar->pArray->elements.size()) {
			cerr << "Error! Array " << varId << " index " << index << " out of range.";
			stack.pushUndefined();
			return;
		}
		stack.push(pArrayVar->pArray->elements[index]);
		return;
	}

	if (pArrayVar->type == tigString) {
		if (index >= pArrayVar->getStringValue().size()) {
			cerr << "Error! String " << varId << " index " << index << " out of range.";
			stack.pushUndefined();
			return;
		}
		stack.push(pArrayVar->getStringValue().substr(index, 1));
		return;
	}
	
	cerr << "Error! Attempt to access variable " << varId << ", index " << index << " as an array or string.";
	stack.pushUndefined();
}


/** Assign the value on the stack to the array address also on the stack. */
void CTigVM::assignElem() {

	int varId = stack.pop().getIntValue();
	CTigVar* pVar = NULL; int objectId = NULL;

	if (varId < memberIdStart) {
		pVar = &stack.local(varId);
	}
	else { //not a local variable
		objectId = stack.pop().getIntValue();
		if (objectId == zeroObject) {
			objectId = currentObject;
			auto it = objects[objectId].members.find(varId);
			if (it != objects[objectId].members.end()) { //local member
				pVar = &it->second;
			}
			else {
				pVar = &objects[zeroObject].members[varId]; //global variable
			}
		}
		else { //member of object objectId?
			auto it2 = objects[objectId].members.find(varId);
			if (it2 != objects[objectId].members.end()) {
				pVar = &it2->second;
			}
		}
	}

	int index = stack.pop().getIntValue();

	CTigVar value = stack.pop();

	if (!pVar) {
		cerr << "\nError! Attempt to assign value " << value.getStringValue() <<
			" to non-existent member " << varId << " of object " << objectId;
		return;
	}

	pVar->pArray->elements[index] = value;
}

/** Array iterator. */
void CTigVM::arrayIt() {
	int resumeAddr = readWord();
	int index = stack.top(-1).getIntValue();

	//Is the 'container' an integer?
	if (stack.top(0).type == tigInt) {
		if (index <= stack.top(0).getIntValue()) { //if we haven't reached the end...
			stack.push(index); //...put index on stack for collection
			stack.top(-2) = ++index;
		}
		else {
			stack.pop();
			stack.pop();
			pc = resumeAddr;
		}
		return;
	}

	//Is the container an array?
	if (stack.top(0).type == tigArray) {
		if (index < stack.top(0).getArraySize()) { //if we haven't reached the end...
			stack.push(stack.top(0).pArray->elements[index]); //...put array element on stack for collection
			stack.top(-2) = ++index;
		}
		else {
			stack.pop();
			stack.pop();
			pc = resumeAddr;
		}
		return;
	}



	//assume container is an object
	if (stack.top(0).type == tigObj) {
		if (index == 0) { //is this the start of the loop? Then try to get container child
			CTigVar containerChild = getMember(stack.top(0).getObjId(), childId);
			if (containerChild.getObjId() > 0) {
				stack.push(containerChild);
				stack.top(-1) = containerChild;
				stack.top(-2) = ++index;
				return;
			}
		}
		else { //no? Then try to get sibling 
			CTigVar containerChildSibling = getMember(stack.top(0).getObjId(), siblingId);
			if (containerChildSibling.getObjId() > 0) {
				stack.push(containerChildSibling);
				stack.top(-1) = containerChildSibling;
				stack.top(-2) = ++index;
				return;
			}
		}
	}

	stack.pop();
	stack.pop();
	pc = resumeAddr;
}

/** Pop the top value off the stack, printing it if it's a string. */
void CTigVM::pop() {
	if (stack.top(0).type == tigString)
		print();
	else
		stack.pop();
}

/** Compare the top 2 values on the stack for equality. */
void CTigVM::compEq() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	
	stack.push(op1 == op2);
}

void CTigVM::compNE() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();

	stack.push(op1 != op2);
}

void CTigVM::compLT() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	stack.push(op1 < op2);
}

void CTigVM::compLE() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	stack.push(op1 <= op2);
}

void CTigVM::compGT() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	stack.push(op1 > op2);
}

void CTigVM::compGE() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	stack.push(op1 >= op2);
}

/** Jump unconditionally. */
void CTigVM::jump() {
	pc = readWord();
}

/** Jump if the top stack value is 0; */
void CTigVM::jumpFalse() {
	CTigVar result = stack.pop();
	int addr = readWord();
	if (result.getIntValue() == 0 || result.type == tigUndefined)
		pc = addr;
}

/** Return the child, if any, of the object on the stack. */
void CTigVM::child() {
	int objId = stack.pop().getObjId();
	CTigVar result = getMember(objId, childId);
	stack.push(result);
}

/** Return the sibling, if any, of the object on the stack. */
void CTigVM::sibling() {
	int objId = stack.pop().getObjId();
	CTigVar result = getMember(objId, siblingId);
	stack.push(result);
}

void CTigVM::in() {
	CTigVar containerObj = stack.pop();
	CTigVar searchObj = stack.pop();

	if (containerObj.type == tigArray) {
		for (auto element : containerObj.pArray->elements) {
			if (searchObj == element) {
				stack.push(1);
				return;
			}
		}
		stack.push(0);
		return;
	}

	//otherwise, lazily assume it's an object
	int container = containerObj.getObjId();
	int obj = searchObj.getObjId();

	int containerChild = getMemberInt(container, childId);
	while (containerChild != 0) {
		if (containerChild == obj) {
			stack.push(1);
			return;
		}
		containerChild = getMemberInt(containerChild, siblingId);
	}
	stack.push(0);
}

void CTigVM::notIn() {
	in();
	stack.top(0).setIntValue(!stack.top(0).getIntValue());
}

/** Get the contents of the variable identified by the stack, and leave it on the stack. */
void CTigVM::getVar() {
	int varId = stack.pop().getIntValue();

	CTigVar* pVar = resolveVariableAddress(varId);

	if (pVar == NULL) {
		//cerr << "\nError! Attempt to access non-existent member " << varId << " of object " << objectId;
		stack.pushUndefined();
	}
	else
		stack.push(*pVar);
}

/** Find the number of children of the object on the stack, and leave that on the stack. */
void CTigVM::children() {
	int objId = stack.pop().getObjId();
	int count = 0;
	int childObjId = getMemberInt(objId, childId);
	while (childObjId != 0) {
		count++;
		childObjId = getMemberInt(childObjId, siblingId);
	}
	stack.push(count);
}

/** Register the function call on the stack as a hot text call, then wrap the associated string with hot text 
	markup identifying that call. */
void CTigVM::makeHot() {
	int paramCount = readByte();
	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
	reverse(params.begin(), params.end());

	int objId = stack.pop().getObjId();
	int method = stack.pop().getIntValue();
	std::string text = stack.pop().getStringValue();


	TFnCall fnCall;
	fnCall.msgId = method;
	fnCall.objId = objId;
	for (auto param : params) {
		fnCall.params.push_back(param);
	}

	//Do we already have a function call for this hot text?
	//The it's a duplicate, so don't create a new hot id.
	unsigned int id = hotTextFnCalls.getItem(text);
	if (id == 0) {
		THotTextFnCall hotTextCall;
		hotTextCall.hotText = text;
		hotTextCall.options.push_back(fnCall);
		id = hotTextFnCalls.addItem(hotTextCall);
	}

	text = "\\h{" + std::to_string(id) + "}" + text + "\\h";
	stack.push(text);
}

/** Same as makeHot, but where the text matches an existing hot text, it isn't sent to output. 
	This enables the overloading of hot texts with multiple function calls.*/
void CTigVM::makeHotAlt() {
	int paramCount = readByte();
	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
	reverse(params.begin(), params.end());
	int objId = stack.pop().getObjId();
	int method = stack.pop().getIntValue();
	std::string text = stack.pop().getStringValue();


	TFnCall fnCall;
	fnCall.msgId = method;
	fnCall.objId = objId;
	for (auto param : params) {
		fnCall.params.push_back(param);
	}

	//do we already have a function call for this hot text?
	unsigned int id = hotTextFnCalls.getItem(text);
	if (id) {
		//it's a variant
		hotTextFnCalls.getItemPtr(id)->options.push_back(fnCall);
		stack.push(string(""));
		return;
	}

	//it's a new hot text call
	THotTextFnCall hotTextCall;
	hotTextCall.hotText = text;
	hotTextCall.options.push_back(fnCall);
	id = hotTextFnCalls.addItem(hotTextCall);

	text = "\\h{" + std::to_string(id) + "}" + text + "\\h";
	stack.push(text);
}



void CTigVM::brk() {
	liveLog << "\nBreak triggered at PC " << pc << ". ";
}

void CTigVM::move() {
	int newParentId = stack.pop().getObjId();
	int objId = stack.pop().getObjId();

	CObjInstance* obj = &objects[objId];
	CObjInstance* newParentObj = &objects[newParentId];
	CObjInstance* objParent = &objects[obj->members[parentId].getObjId()];

	CObjInstance* childObj = &objects[objParent->members[childId].getObjId()];
	CObjInstance* olderSibling = NULL;

	if (objParent->id) {
		while (childObj != obj) {
			olderSibling = childObj;
			childObj = &objects[childObj->members[siblingId].getObjId()];
		}

		if (olderSibling)
			olderSibling->members[siblingId] = obj->members[siblingId];
		else
			objParent->members[childId] = obj->members[siblingId];

	}

	int destChild = newParentObj->members[childId].getObjId();

	if (destChild)
		obj->members[siblingId].setObjId(destChild);
	else
		obj->members[siblingId].setObjId(NULL);

	newParentObj->members[childId].setObjId(objId);
	obj->members[parentId].setObjId(newParentId);
}

/** Tell user app we want to open a window. */
void CTigVM::openWin() {
	int objId = stack.pop().getObjId();
	openWindow(objId);
}

/** Set the window we're outputting to. */
void CTigVM::win() {
	window = stack.pop().getIntValue();
	latestText.clear(); // Avoids effects carrying over from another window.
}

void CTigVM::clr() {
	clearWin();
}

/** Tell the user to remove the most recent marked text, if any.*/
void CTigVM::clrMarked() {
	clearMarkedText();
}

void CTigVM::style() {
	std::string name = stack.pop().getStringValue();
	std::string result = "\\style{" + name + "}";
	stack.push(result);
}

void CTigVM::cap() {
	std::string text = stack.pop().getStringValue();
	text[0] = toupper(text[0]);
	stack.push(text);
}

void CTigVM::capNext() {
	capitaliseNext = true;
}

void CTigVM::inherits() {
	int classId = stack.pop().getObjId();
	int objId = stack.pop().getObjId();

	if (inheritsFrom(objId, classId))
		stack.push(1);
	else
		stack.push(0);
}

void CTigVM::hotClr() {
	hotTextKeywords.clear();
}

void CTigVM::hotCheck() {
	string text = stack.pop().getStringValue();
	for (auto hotText : hotTextKeywords) {
		if (hotText.text == text && hotText.used) {
			stack.push(1);
			return;
		}
	}
	stack.push(0);
}

void CTigVM::not() {
	CTigVar value = stack.pop();
	stack.push(!value.getIntValue());
}

void CTigVM::and() {
	CTigVar op1 = stack.pop();
	CTigVar op2 = stack.pop();
	stack.push(op1.getIntValue() && op2.getIntValue());
}

void CTigVM::or() {
	CTigVar op1 = stack.pop();
	CTigVar op2 = stack.pop();
	stack.push(op1.getIntValue() || op2.getIntValue());
}


void CTigVM::arrayPush() {
	int varId = stack.pop().getIntValue();
	CTigVar* pVar = resolveVariableAddress(varId);
	CTigVar value = stack.pop();
	if (pVar->type != tigArray)
		pVar->setArray();
	pVar->pArray->elements.push_back(value);
}

void CTigVM::arrayRemove() {
	int varId = stack.pop().getIntValue();
	CTigVar* pVar = resolveVariableAddress(varId);
	CTigVar value = stack.pop();
	if (pVar->type != tigArray) {
		liveLog << alertMsg << "\nError! Attempt to remove element from non-array.";
		return;
	}
	for (unsigned int x = 0; x < pVar->pArray->elements.size(); x++) {
		if (pVar->pArray->elements[x] == value) {
			pVar->pArray->elements.erase(pVar->pArray->elements.begin() + x);
			return;
		}

	}
}

void CTigVM::msg() {
	int paramCount = readByte();
	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
	messageApp(params[1].getIntValue(), params[0].getIntValue());
}

/** True if the given object has the given member. */
void CTigVM::has() {
	int memberId = stack.pop().getIntValue();
	int objId = stack.pop().getObjId();
	stack.push(hasMember(objId, memberId));
}

/** Compare the two strings on the stack to see if one is a substring of the other. */
void CTigVM::match() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result(tigUndefined);

	if (op1.type == tigString && op2.type == tigString) {
		unsigned int found = op2.getStringValue().find(op1.getStringValue());
		if (found != string::npos)
			result = (int) found;
	}

	stack.push(result);
}

/** Return true if the object on the stack has the specified flag set. */
void CTigVM::is() {
	int flag = stack.pop().getIntValue();
	CTigVar obj = stack.pop();
	
	//Panic if we don't have an object
	if (obj.type != tigObj) {
		liveLog << alertMsg << "\nError! Attempt to read flag " << flag << " on a non-object.";
		stack.push(tigUndefined);
		return;
	}

	CTigVar flags = getMember(obj.getObjId(), flagsId);
	if (flags.type == tigInt && flags.getIntValue() & flag) {
		stack.push(1);
		return;
	}

	stack.push(0);
}

/** Return true if the object on the stack has not got the specified flag set. */
void CTigVM::isNot() {
	int flag = stack.pop().getIntValue();
	CTigVar obj = stack.pop();

	//Panic if we don't have an object
	if (obj.type != tigObj) {
		cerr << "\nError! Attempt to read flag " << flag << " on a non-object.";
		stack.push(tigUndefined);
		return;
	}

	CTigVar flags = getMember(obj.getObjId(), flagsId);
	if (flags.type == tigInt && flags.getIntValue() & flag) {
		stack.push(0);
		return;
	}

	stack.push(1);
}

/** Set the specified flag for this object. If it doesn't already have the #flags data member,
	supply it. */
void CTigVM::set() {
	int flag = stack.pop().getIntValue();
	CTigVar obj = stack.pop();

	//Panic if we don't have an object
	if (obj.type != tigObj) {
		cerr << "\nError! Attempt to set flag " << flag << " on a non-object.";
		return;
	}

	int objId = obj.getObjId();
	CTigVar flags = getMember(objId, flagsId);
	if (flags.type == tigUndefined) { //if the object lacks the #flags member
		objects[objId].members[flagsId] = flag; //supply it
	}
	else {
		flag = objects[objId].members[flagsId].getIntValue() | flag;
		objects[objId].members[flagsId] = flag;
	}
}

/** Clear the specified flag for the object on the stack, if it has it. */
void CTigVM::unset() {
	int flag = stack.pop().getIntValue();
	CTigVar obj = stack.pop();

	//Panic if we don't have an object
	if (obj.type != tigObj) {
		cerr << "\nError! Attempt to unset flag " << flag << " on a non-object.";
		return;
	}

	int objId = obj.getObjId();
	CTigVar flags = getMember(objId, flagsId);
	if (flags.type == tigUndefined) //if the object lacks the #flags member
		return; //nothing to do

	flag = objects[objId].members[flagsId].getIntValue() & ~flag;
	objects[objId].members[flagsId] = flag;

}

/**	Create a new object of the speciefied class, and put a reference to it on the stack. */
void CTigVM::newOp() {
	//int classId = readWord();
	int classId = stack.pop().getIntValue();

	objects[nextFreeObjNo] = objects[classId];
	objects[nextFreeObjNo].id++;

	

	int initialisations = (int)readByte();
	for (int x = 0; x < initialisations; x++) {
		int memberId = readWord();
		objects[nextFreeObjNo].members[memberId] = stack.pop();
	}

	stack.pushObj(nextFreeObjNo++);
}

void CTigVM::deleteOp() {
	CTigVar obj = stack.pop();
	objects.erase(obj.getObjId());
}


void CTigVM::finalLoop() {
	int index = stack.top(-1).getIntValue();

	if (stack.top(0).type == tigArray) {
		if (index == stack.top(0).getArraySize()) {
			stack.push(1);
			return;
		}
	}

	stack.push(0);
}

/** Return 1 if this is the first iteration of a loop, zero otherwise.*/
void CTigVM::firstLoop() {
	int index = stack.top(-1).getIntValue();

	if (stack.top(0).type == tigArray) {
		if (index == 1) {
			stack.push(1);
			return;
		}
	}

	stack.push(0);
}

/** Generate a random dice roll of the given size, leave it on the stack. */
void CTigVM::roll() {
	int sides = readWord();
	std::uniform_int_distribution<> die{ 1,sides };

	stack.push(die(randEngine));
}


/** Sort the array on the stack in descending order using the supplied member id. */
void CTigVM::sortDesc() {
	int memberId = stack.pop().getIntValue();
	//TO DO: use a value of zero to indicate sort by element value, not member value

	int varId = stack.pop().getIntValue();
	CTigVar* array = resolveVariableAddress(varId);
	if (array->type != tigArray) {
		cerr << "\nError! Attempt to sort non-array.";
		return;
	}

	auto compare = [&](CTigVar elem1, CTigVar elem2) -> bool {
		return getMember(elem1.getObjId(), memberId) > getMember(elem2.getObjId(), memberId);
	};

	std::sort(array->pArray->elements.begin(), array->pArray->elements.end(), compare);
}

/** Send whatever is on the stack to the user's logging system to handle. */
void CTigVM::log() {
	std::string text = stack.pop().getStringValue();
	logText(text);
}

/** Notify the user of either a pause or an umpause for them to deal with. */
void CTigVM::pause() {
	if (paused) {
		paused = false;
	}
	else {
		paused = true;
	}
	handlePause(paused);
}

/** Return a random element from the array on the stack. */
void CTigVM::rand() {
	int varId = stack.pop().getIntValue();
	CTigVar* array = resolveVariableAddress(varId);
	if (array->type != tigArray) {
		liveLog << "\nError! Attempt to randomly access a variable as an array.";
		stack.pushUndefined();
		return;
	}

	unsigned int arraySize = array->pArray->elements.size();
	if (arraySize == 0) {
		liveLog << "\nWarning: randomly accessing an empty array.";
		stack.pushUndefined();
		return;
	}
	
	std::uniform_int_distribution<> randomRange{ 0,(int)arraySize-1 };
	unsigned int randomIndex = randomRange(randEngine);
	CTigVar randomElement = array->pArray->elements[randomIndex];
	stack.push(randomElement);
}


TVMstatus CTigVM::getStatus() {
	return status;
}

void CTigVM::getOptionStrs(std::vector<std::string>& optionStrs) {
	for (auto option : currentOptionList) {
		optionStrs.push_back(option.text);
	}
}

/** Handle a message sent by the user. */
void CTigVM::sendMessage(const TVMmsg& msg) {
	if (msg.type == vmMsgChoice && status == vmAwaitChoice) {
		int choice = msg.integer;
		int optionIndex = currentOptionList[choice].index;
		int tableStart = pc;
		pc += optionIndex * sizeof(int);
		pc = tableStart + readWord();
		currentOptionList.clear();
		status = vmExecuting;
		execute(); //TO DO: maybe not execute here, where we can't guarantee a return to the user. Let the user choose when.
	}

	if (msg.type == vmMsgString && status == vmAwaitString) {
		//push the string on the stack and resume execution
		stack.push(std::string(msg.text));
		status = vmExecuting;
		execute();
	}
}


/** Return the value of the named global variable, if found. */
CTigVar CTigVM::getGlobalVar(std::string varName) {
	//auto it = find_if(globalVarNameTable.begin(), globalVarNameTable.end(), find_varName(varName));
	auto it = find_if(memberNames.begin(), memberNames.end(),
		[&](std::string& nameRec) { return nameRec == varName; });
	CTigVar var;
	int memberId;
	if (it != memberNames.end()) {
		memberId = (it - memberNames.begin()) + memberIdStart;
		auto it2 = objects[zeroObject].members.find(memberId);
		if (it2 != objects[zeroObject].members.end()) {
			return it2->second;
		}
	}

	std::cerr << "\nGlobal variable " + varName + " not found!";
	return var;
}


/** Return a copy of the identified member. */
CTigVar CTigVM::getMember(int objNo, int memberId) {
	CObjInstance* obj = &objects[objNo];
	if (obj->members.find(memberId) == obj->members.end()) {
		CTigVar notFound(tigUndefined);
		return notFound;
	}
	return obj->members[memberId];
}

int CTigVM::getMemberInt(int objNo, int memberId) {
	return getMember(objNo, memberId).getIntValue();
}

/** Return the member id of the named member. */
int CTigVM::getMemberId(std::string name) {
	auto it = find_if(memberNames.begin(), memberNames.end(),
		[&](std::string& currentName) { return currentName == name; });

	if (it == memberNames.end()) {
		std::cerr << "\nMember " << name << " not found.";
		return 0;
	}

	return it - memberNames.begin() + memberIdStart;
}


CTigVar CTigVM::callMember(int objId, int memberId, initializer_list<CTigVar> params) {
	std::vector<CTigVar> paramList;
	for (auto param : params)
		paramList.push_back(param);

	return callMember(objId, memberId, paramList);
}

CTigVar CTigVM::callMember(int objId, std::string msgName, initializer_list<CTigVar> params) {
	int memberId = getMemberId(msgName);
	return callMember(objId, memberId, params);
}

CTigVar CTigVM::callMember(int objId, std::string msgName) {
	return callMember(objId, msgName, {});
}

CTigVar CTigVM::callMember(int objId, int msgId) {
	return callMember(objId, msgId, {});
}

CTigVar CTigVM::callMember(int objId, int msgId, std::vector<CTigVar>& params) {
	CTigVar* objMember = &objects[objId].members[msgId];

	if (objMember->type == tigString)
		return *objMember;

	if (objMember->type == tigFunc) {
		int addr = objMember->getFuncAddress();
		stack.push(NULL); //dummy calling object
		stack.push(NULL); //dummy start positionHint
		currentObject = objId;
		pc = addr;
		int varCount = readByte();
		stack.reserveLocalVars(varCount);

		int x = 0;
		for (auto param : params)
			stack.local(x++) = param;

		status = vmExecuting;
		execute();
		return stack.pop();
	}

	//TO DO: handle other types
	return *objMember;
}




std::string CTigVM::getMemberName(int memberId) {
	return memberNames[memberId - memberIdStart];
}



bool CTigVM::inheritsFrom(int obj, int classObj) {
	for (auto currentClass : objects[obj].classIds) {
		if (currentClass == classObj)
			return true;
		if (inheritsFrom(currentClass, classObj))
			return true;
	}
	return false;
}


void CTigVM::setMemberValue(int objNo, std::string memberName, CTigVar & value) {
	int memberNo = getMemberId(memberName);

	auto it = objects[objNo].members.find(memberNo);
	if (it != objects[objNo].members.end())
		it->second = value;
	else
		cerr << "\nAttempt to set nonexistent member " << memberName << " in object "
		<< objNo << " to " << value.getStringValue();
}

/** Returns true if the object has this member. */
bool CTigVM::hasMember(int objNo, int memberNo) {
	if (objects[objNo].members.find(memberNo) == objects[objNo].members.end())
		return false;
	return true;
}


CObjInstance * CTigVM::getObject(int objId) {
	auto found = objects.find(objId);
	if (found == objects.end())
		return NULL;
	return &found->second;
}



int CTigVM::getObjectId(CObjInstance * obj) {
	auto it = find_if(objects.begin(), objects.end(),
		[&](pair<int,CObjInstance> currentObj) { return currentObj.second.id == obj->id; });
	if (it == objects.end())
		return 0;
	return distance(objects.begin(), it)-1;
}

/** Printer's devil: replaces any instances of hot text with hot text markup, and does a little tidying. */
std::string CTigVM::devil(std::string text) {
	bool endedOnFullstop = false;
	bool lacksFinalSpace = false;
	auto endChar = latestText.find_last_not_of(" \\h");
	if (endChar != std::string::npos) {
		if (latestText[endChar] == '.' || latestText[endChar] == '!') {
			endedOnFullstop = true;
			auto finalSpace = latestText.find_last_of(" ");
			if (finalSpace != std::string::npos && finalSpace < endChar)
				lacksFinalSpace = true;
		}
	}

	//don't print redundant full stop.
	if (text[0] == '.' && endedOnFullstop) 
		text.erase(0,  text.find_first_not_of(" ",1));

	if (text.size() == 0)
		return text;

	//if text starts with a comma
	//and the last text ended with a full stop
	//snip the comma, capitalise
	if (text[0] == ',' && endedOnFullstop) {
		text.erase(0); 
	}

	//if last text ended with a full stop, ensure we capitalise
	if (capitaliseNext || endedOnFullstop) {
		auto startChar = text.find_first_not_of(" ");
		if (startChar != std::string::npos) {
			text[startChar] = toupper(text[startChar]);
			capitaliseNext = false;
		}
	}

	//if last text lacked a final space and there's none here, add one.
	if (lacksFinalSpace && text[0] != ' ')
			text.insert(0, " ");


	//search for current hot text keywords that we haven't already hot texted
	for (auto& hotKeyword : hotTextKeywords) {
		if (!hotKeyword.used) {
			size_t found = regExFind(text, 0, hotKeyword.text);
			while (found != std::string::npos) {
				std::string hotStr;

				THotTextFnCall hotTxtFnCall;
				TFnCall fnCall;
				fnCall.msgId = hotKeyword.msgId;
				fnCall.objId = hotKeyword.objId;
				fnCall.params = hotKeyword.params;
				hotTxtFnCall.hotText = hotKeyword.text;
				hotTxtFnCall.options.push_back(fnCall);
				unsigned int id = hotTextFnCalls.addItem(hotTxtFnCall);

				hotStr = "\\h{" + std::to_string(id) + "}" + hotKeyword.text + "\\h";
				text.replace(found, hotKeyword.text.size(), hotStr);
				hotKeyword.used = true;
				//TO DO: why don't I remove the keyword altogether???
				break; //don't look again

				//found = regExFind(text, found + hotStr.size(), hotKeyword.text);
			}
		}
	}

	//tidying
	if (text.size() && text.back() == '.')
		text += " ";

	return text;
}



/** Search for the keyword where it is not part of a bigger word, or within an already existing hot text markup. */
unsigned int CTigVM::regExFind(std::string source, unsigned int offset, std::string subject) {
	std::regex re("(\\s|^)"  + subject + "([^[:alnum:]]|$)" , regex_constants::icase);
	std::smatch match;
	std::string substring = source.substr(offset);
	if (regex_search(substring, match, re) && match.size()) {
		int prelim = match[1].str().size();
		return match.position() + prelim;
	}
	else
		return std::string::npos;
}

/** Remove the hot text function call with the given id from the list. */
void CTigVM::removeHotTextFnCall(unsigned int id) {
	hotTextFnCalls.removeItem(id);
}

/** Return the stored function call for the given hot text id and variant. */
TFnCall CTigVM::getHotTextFnCall(unsigned int id, int variant) {
	THotTextFnCall  hotTextFnCall =  hotTextFnCalls.getItem(id);
	return hotTextFnCall.options[variant];
}

/** Return the entrire stored function call record the given hot text id. */
THotTextFnCall CTigVM::getHotTextFnCallRec(unsigned int id) {
	return hotTextFnCalls.getItem(id);
}

/**	Clear all memory and reset as if no progfile has been loaded. */
void CTigVM::reset() {
	memberNames.clear();
	objects.clear();
	eventTable.clear();
	delete progBuf;
}

/** Reload the last progfile into memory again. */
bool CTigVM::reloadProgFile() {
	return loadProgFile(currentProgFile);
}

