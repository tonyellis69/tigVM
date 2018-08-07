#include "vm.h"
#include "..\..\TC\src\sharedTypes.h"

#include <algorithm>    // std::find_if

using namespace std;

CTigVM::CTigVM() {
	escape = false;
	time(&currentTime);
	status = vmNoProgram;
	window = 0;
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

	progBufSize = readHeader(progFile);

	progBuf = new char[progBufSize];
	progFile.read(progBuf, progBufSize);

	readEventTable(progFile);
	//readGlobalVarTable(progFile);
	readObjectDefTable(progFile);
	readMemberNameTable(progFile);
	//readGlobalFnTable(progFile);

	initIds();

	pc = globalCodeAddr;
//	if (eventTable.size()) //assumes first event is the starting event
	//	pc = eventTable[0].address;
	status = vmExecuting;
	currentObject = 0;
	return true;
}

void CTigVM::initIds() {
	childId = getMemberId("child");
	siblingId = getMemberId("sibling");
	parentId = getMemberId("parent");
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
	//CObjInstance zeroObject;
	//objects[0] = zeroObject;
	int objectDefTableSize; char nMembers;
	progFile.read((char*)&objectDefTableSize, 4);
	for (int objNo = 0; objNo < objectDefTableSize; objNo++) {
		CObjInstance object;
		progFile.read((char*)&object.id, 4);

		char noParentClasses;
		progFile.read((char*)&noParentClasses, 1);
		for (int parentClass = 0; parentClass < noParentClasses; parentClass++) {
			int classId;
			progFile.read((char*)&classId, 4);
			object.classIds.push_back(classId);
			for (auto member : objects[classId].members) {
				object.members[member.first] = member.second;
			}
		}

		//progFile.read((char*)&object.classId, 4);
		//if (object.classId != -1) {
		//	object.members = objects[object.classId].members;
	//	}

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

/** Read the list of global function addresses and store them. */
/*
void CTigVM::readGlobalFnTable(std::ifstream & progFile) {
	int fnTableSize;
	progFile.read((char*)&fnTableSize, 4);
	//globalFuncs.resize(fnTableSize);
	//for (auto &addr : globalFuncs) {
	for (int f=0; f < fnTableSize; f++) {
		int id, addr;
		progFile.read((char*)&id, 4);
		progFile.read((char*)&addr, 4);
		globalFuncs[id] = addr;
	}
}*/

/** Execute the currently loaded program starting at the current program counter position. */
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
		case opGT: compGT(); break;
		case opJump: jump(); break;
		case opJumpFalse: jumpFalse(); break;
		case opChild: child(); break;
		case opSibling: sibling(); break;
		case opGetVar: getVar(); break;
		case opChildren: children(); break;
		case opMakeHot: makeHot(); break;
		case opBrk: brk(); break;
		case opMove: move(); break;
		case opOpenWin: openWin(); break;
		case opWin: win(); break;
		case opClr: clr(); break;
		case opStyle: style(); break;
		case opCap: cap(); break;
		case opInherits: inherits(); break;
		case opHotClr: hotClr(); break;
		case opHotCheck: hotCheck(); break;
		case opNot: not(); break;
		case opAnd: and(); break;
		case opOr: or(); break;
		case opArrayPush: arrayPush(); break;
		case opMsg: msg(); break;
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
	if (varId < memberIdStart) { //global variable
		//if (varId >= globalVarStart) {
		//	varId -= globalVarStart;
		//	stack.push(globalVars[varId]);
		//}
	//	else { //local variable
		stack.push(stack.local(varId));
		return;
		//}
	}

	//Must be a data member
	int objectId = stack.pop().getIntValue();

	if (objectId == zeroObject) { 	//could be local, could be global
		auto it = objects[currentObject].members.find(varId);
		if (it != objects[currentObject].members.end()) { //it's local
			stack.push(it->second);
			return;
		}
	}

	//still here? Either global or belongs to another object
	auto it2 = objects[objectId].members.find(varId);
	if (it2 != objects[objectId].members.end()) {
		stack.push(it2->second);
		return;
	}



	cerr << "\nError! Attempt to access non-existent variable " << varId << " in object " << objectId;
	stack.pushUndefined();


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

/** Pop a value off the stack and assign it to a variable. */
void CTigVM::assign() {
	CTigVar* pVar = resolveVariableAddress(); 
	//CTigVar value = stack.pop();

	if (!pVar)
		return;
	
	if (pVar->type == tigArray) {
		int index = stack.pop().getIntValue();
		pVar->pArray->elements[index] = stack.pop();
	}
	else
		*pVar = stack.pop();
}

CTigVar* CTigVM::resolveVariableAddress() {
	int varId = stack.pop().getIntValue();

	CTigVar* pVar = NULL; int objectId = NULL;

	if (varId < memberIdStart) {
		pVar = &stack.local(varId);
	}
	else { //not a local variable
		int objectId = stack.pop().getIntValue();
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

	if (!pVar) {
		cerr << "\nError! Attempt to access non-existent member " << varId << " of object " << objectId;
	}
	return pVar;
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
	CTigVar* callee = NULL; int obj = 0;

	if (memberId < memberIdStart) { //local var
		callee = &stack.local(memberId);
	}
	else {
		int op1 = stack.pop().getObjId();
		obj = op1;
		if (obj == zeroObject)
			obj = currentObject;

		auto fnMember = objects[obj].members.find(memberId);
		if (fnMember != objects[obj].members.end()) { //it's a local member
			callee = &fnMember->second;
		}
		else if (op1 == zeroObject) { //could be global
			fnMember = objects[zeroObject].members.find(memberId);
			if (fnMember != objects[zeroObject].members.end()) {
				callee = &fnMember->second;
			}
		}

	}

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

	if (memberId < memberIdStart) { //local var
		callee = &stack.local(memberId);
	}
	else {
		int op1 = stack.pop().getObjId();
		obj = op1;
		if (obj == zeroObject)
			obj = currentObject;

		auto fnMember = objects[obj].members.find(memberId);
		if (fnMember != objects[obj].members.end()) { //it's a local member
			callee = &fnMember->second;
		}
		else if (op1 == zeroObject) { //could be global
			fnMember = objects[zeroObject].members.find(memberId);
			if (fnMember != objects[zeroObject].members.end()) {
				callee = &fnMember->second;
			}
		}

	}

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


/*
void CTigVM::callFn() {
	int funcNo = readWord();
	int paramCount = readByte();

	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
 
	stack.push(currentObject);
	stack.push(pc); //save return address

	pc = globalFuncs[funcNo];
	int varCount = readByte();
	stack.reserveLocalVars(varCount);

	for (int p = 0; p < paramCount; p++) {
		stack.local(p) = params[(paramCount-1) - p ];
	}
}
*/

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
	int objId = stack.pop().getIntValue();
	int memberId = stack.pop().getIntValue();
	std::string text = stack.pop().getStringValue();
	hotText(text, memberId , objId);
	hotTexts.push_back({ text, memberId, objId,false });
}

/** Ask the user to remove any hot text with the given values. */
void CTigVM::purge() {
	int objId = stack.pop().getObjId();
	int memberId = stack.pop().getIntValue();
	purge(memberId, objId);
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

/** Push the value in given array element onto the stack for further processing. */
void CTigVM::pushElem() {
	int varId = stack.pop().getIntValue();
	int index = stack.pop().getIntValue();

	CTigVar arrayVar;

	if (varId < memberIdStart) {
		arrayVar = stack.local(varId);
		stack.push(arrayVar.pArray->elements[index]);
		return;
	}
	else {
		int objId = index;
		index = stack.pop().getIntValue();

		if (objId == zeroObject) { 	//could be local, could be global
			auto it = objects[currentObject].members.find(varId);
			if (it != objects[currentObject].members.end()) { //it's local
				stack.push(it->second.pArray->elements[index]);
				return;
			}
		}

		//still here? Either global or belongs to another object
		auto it2 = objects[objId].members.find(varId);
		if (it2 != objects[objId].members.end()) {
			stack.push(it2->second.pArray->elements[index]);
			return;
		}
	}
	cerr << "Error! Unrecognised variable " << varId << " used as array identifier in an expression, index "
		<< index;
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
	if (index == 3)
		int b = 0;
	if (index < stack.top(0).getArraySize()) {
		stack.push(stack.top(0).pArray->elements[index]);
		stack.top(-2) = ++index;
	}
	else {
		stack.pop();
		stack.pop();
		pc = resumeAddr;
	}
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

void CTigVM::compGT() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	stack.push(op1 > op2);
}

/** Jump unconditionally. */
void CTigVM::jump() {
	pc = readWord();
}

/** Jump if the top stack value is 0; */
void CTigVM::jumpFalse() {
	int result = stack.pop().getIntValue();
	int addr = readWord();
	if (result == 0)
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

/** Get the contents of the variable identified by the stack, and leave it on the stack. */
void CTigVM::getVar() {
	int varId = stack.pop().getIntValue();
	if (varId < memberIdStart) { //local variable
		CTigVar localVar = stack.local(varId);
		stack.push(localVar);
		////////////////Local var start not initialised
		return;
	}

	//otherwise it's a data member
	int objectId = stack.pop().getIntValue();
	if (objectId == zeroObject) {
		auto it = objects[currentObject].members.find(varId);
		if (it != objects[currentObject].members.end()) { //it's local
			stack.push(it->second);
			return;
		}
	}

	//still here? Either global or belongs to another object
	auto it2 = objects[objectId].members.find(varId);
	if (it2 != objects[objectId].members.end()) {
		stack.push(it2->second);
		return;
	}

	cerr << "\nError! Attempt to access non-existent member " << varId << " of object " << objectId;
	stack.pushUndefined();
}

/** Find the number of children of the object on the stack, and leave that on the stack. */
void CTigVM::children() {
	int objId = stack.pop().getObjId();
	int count = 0;
	CTigVar childObj = getMember(objId, childId);
	while (childObj.getObjId() != 0) {
		count++;
		childObj = getMember(childObj, siblingId);
	}
	stack.push(count);
}

/** Wrap the string on the stack with hot text markup */
void CTigVM::makeHot() {
	int objId = stack.pop().getObjId();
	int method = stack.pop().getIntValue();
	std::string text = stack.pop().getStringValue();

	text = "\\h{" + std::to_string(method) + '@' + std::to_string(objId) + "}" + text + "\\h";
	stack.push(text);
}

void CTigVM::brk() {
	cerr << "\nBreak triggered at PC " << pc << ". ";
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
}

void CTigVM::clr() {
	clearWin();
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

void CTigVM::inherits() {
	int classId = stack.pop().getObjId();
	int objId = stack.pop().getObjId();

	if (inheritsFrom(&objects[objId], &objects[classId]))
		stack.push(1);
	else
		stack.push(0);
}

void CTigVM::hotClr() {
	hotTexts.clear();
}

void CTigVM::hotCheck() {
	std:string text = stack.pop().getStringValue();
	for (auto hotText : hotTexts) {
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
	CTigVar* pVar = resolveVariableAddress();
	CTigVar value = stack.pop();
	if (pVar->type != tigArray)
		pVar->setArray();
	pVar->pArray->elements.push_back(value);
}

void CTigVM::msg() {
	int paramCount = readByte();
	std::vector<CTigVar> params(paramCount);
	for (int p = 0; p < paramCount; p++) {
		params[p] = stack.pop();
	}
	messageApp(params[1].getIntValue(), params[0].getIntValue());
	params.clear();
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

/* old way to do it, preserved for reference.
struct find_varName {
	std::string name;
	find_varName(std::string& name) : name(name) {}
	bool operator () (const TGlobalVarNameRec& nameRec) const	{
		return nameRec.name == name;
	}
}; */

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
CTigVar CTigVM::getMember(CTigVar & obj, int memberId) {
	return objects[obj.getObjId()].members[memberId];
}

/** Return a copy of the identified member. */
//////////////////////////Try to use this and remove others!!!!!!!!!!!!!!!!!!
CTigVar CTigVM::getMember(int objNo, int memberId) {
	CObjInstance* obj = &objects[objNo];
	if (obj->members.find(memberId) == obj->members.end()) {
		CTigVar notFound(tigUndefined);
		return notFound;
	}
	return obj->members[memberId];
}

/** Return a copy of the named member. */
CTigVar CTigVM::getMember(CTigVar & obj, std::string fnName) {
	int memberNo = getMemberId(fnName);
	return objects[obj.getObjId()].members[memberNo];
}

CTigVar CTigVM::getMember(CObjInstance * obj, int memberId) {
	return obj->members[memberId];
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


/** Execute the identified object member. */
CTigVar CTigVM::objMessage(CTigVar & obj, int memberId) {
	CTigVar member = getMember(obj, memberId);
	currentObject = obj.getObjId();
	return executeObjMember(member);
}

CTigVar CTigVM::objMessage(int objNo, int memberId) {
	CTigVar member = getMember(objNo, memberId);
	currentObject = objNo;
	return executeObjMember(member);
}

CTigVar CTigVM::objMessage(int objNo, std::string fnName) {
	int memberNo = getMemberId(fnName);
	CTigVar member = getMember(objNo, memberNo);
	currentObject = objNo;
	return executeObjMember(member);
}

/** Execute the named object member. */
CTigVar CTigVM::objMessage(CTigVar & obj, std::string fnName) {
	CTigVar member = getMember(obj, fnName);
	currentObject = obj.getObjId();
	return executeObjMember(member);
}

CTigVar CTigVM::objMessage(CObjInstance* obj, std::string fnName) {
	int memberNo = getMemberId(fnName);
	CTigVar member = getMember(obj, memberNo);
	currentObject = obj->id;
	return executeObjMember(member);
}



CTigVar CTigVM::executeObjMember(CTigVar & ObjMember) {
	if (ObjMember.type == tigString) { //or int or float 
		//writeText(ObjMember.getStringValue());
		return ObjMember;
	}
	if (ObjMember.type == tigFunc) {

		int addr = ObjMember.getFuncAddress();
		stack.push(NULL); //dummy calling object
		stack.push(NULL); //dummy start position
		pc = addr;
		int varCount = readByte();
		stack.reserveLocalVars(varCount);
		status = vmExecuting;
		execute();
		return stack.pop();
		//TO DO: calling execute internally is not ideal. Might be better to just
		//assume execution gets handled by the caller.
	}
	return ObjMember; //This may not be the best default action...
}

CTigVar CTigVM::callMember(int objId, std::string msgName, initializer_list<CTigVar> params) {
	int memberId = getMemberId(msgName);
	CTigVar* objMember = &objects[objId].members[memberId];

	if (objMember->type == tigString)
		return *objMember;

	if (objMember->type == tigFunc) {
		int addr = objMember->getFuncAddress();
		stack.push(NULL); //dummy calling object
		stack.push(NULL); //dummy start position
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
	return CTigVar();
}

std::string CTigVM::getMemberName(int memberId) {
	return memberNames[memberId - memberIdStart];
}

/** Return the given member value as an int. */
int CTigVM::getMemberValue(int objNo, int memberId) {
	CObjInstance* obj = &objects[objNo];
	if (obj->members.find(memberId) == obj->members.end())
		return 0;
	return obj->members[memberId].getIntValue();
}

int CTigVM::getMemberValue(int objNo, std::string memberName) {
	int memberNo = getMemberId(memberName);
	return getMemberValue(objNo, memberNo);
}

/** Return the class of the given object.*/
int CTigVM::getClass(int objNo) {
//	return objects[objNo].classId;
	return NULL;
}

bool CTigVM::inheritsFrom(CObjInstance* obj, int classId) {
	CObjInstance* classObj = &objects[classId];
	return inheritsFrom(obj,classObj);
}

bool CTigVM::inheritsFrom(CObjInstance* obj, CObjInstance* classObj) {
	for (auto currentClass : obj->classIds) {
		if (currentClass == classObj->id)
			return true;
		CObjInstance* currentClassObj = &objects[currentClass];
		if (inheritsFrom(currentClassObj, classObj))
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

bool CTigVM::hasMember(CObjInstance* obj, int memberNo) {
	if (obj->members.find(memberNo) == obj->members.end())
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
	//don't print redundant full stop.
	if (text[0] == '.') {
		auto endChar = latestText.find_last_not_of(" ");
		if (endChar != std::string::npos) {
			if (latestText[endChar] == '.') {
				text.erase(0,  text.find_first_not_of(" ",1));
			}
		}
	}

	if (text.size() == 0)
		return text;

	//if text starts with a comma
	//and the last text ended with a full stop
	//snip the comma, capitalise
	if (text[0] == ',') {
		auto endChar = latestText.find_last_not_of(" ");
		if (endChar != std::string::npos) {
			if (latestText[endChar] == '.') {
				text.erase(0, text.find_first_not_of(" ", 1));
				text[0] = toupper(text[0]);
			}
		}
	}

	for (auto& hotText : hotTexts) {
		if (!hotText.used) {
			size_t found = text.find(hotText.text);
			while (found != std::string::npos) { //found, but check it isn't part of a bigger word:
				if ((found == 0 || !isalnum(text[found - 1])) && !isalnum(text[found + hotText.text.size()])) {
					std::string hotStr = "\\h{" + std::to_string(hotText.msgId) + '@' + std::to_string(hotText.objId) + "}";
					hotStr += hotText.text + "\\h";
					text.replace(found, hotText.text.size(), hotStr);
					hotText.used = true;
					break; //don't look again
				}
				found = text.find(hotText.text, found + hotText.text.size());
			}
		}
	}

	//tidying
	if (text.back() == '.')
		text += " ";

	return text;
}

