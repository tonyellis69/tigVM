#include "vm.h"
#include "..\..\TC\src\sharedTypes.h"

#include <algorithm>    // std::find_if

using namespace std;

CTigVM::CTigVM() {
	escape = false;
	time(&currentTime);
	status = vmNoProgram;
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
	readGlobalVarTable(progFile);
	readObjectDefTable(progFile);
	readMemberNameTable(progFile);

	pc = globalCodeAddr;
//	if (eventTable.size()) //assumes first event is the starting event
	//	pc = eventTable[0].address;
	status = vmExecuting;
	currentObject = 0;
	return true;
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

void CTigVM::readGlobalVarTable(std::ifstream & progFile) {
	int globalVarTableSize;
	progFile.read((char*)&globalVarTableSize, 4);
	globalVarNameTable.resize(globalVarTableSize);
	int addr; std::string tmp;
	for (auto &globalVarNameRec : globalVarNameTable) {
		std::getline(progFile, tmp, '\0');
		globalVarNameRec.name = tmp;
		progFile.read((char*)&addr, 4);
		globalVarNameRec.id = addr;
	}
	globalVars.resize(globalVarTableSize);
}

/** Read the progfile object defintions and create an object for each one. */
void CTigVM::readObjectDefTable(std::ifstream & progFile) {
	CObjInstance zeroObject;
	objects[0] = zeroObject;
	int objectDefTableSize; char nMembers;
	progFile.read((char*)&objectDefTableSize, 4);
	for (int objNo = 0; objNo < objectDefTableSize; objNo++) {
		CObjInstance object;
		progFile.read((char*)&object.id, 4);
		progFile.read((char*)&object.classId, 4);
		if (object.classId != -1) {
			object.members = objects[object.classId].members;
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
					blank.pArray = new CArray(); //TO DO: use shared_ptr
					blank.type = tigArray;
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
	int memberNameTableSize; char numMembers;
	progFile.read((char*)&memberNameTableSize, 4);
	memberNames.resize(memberNameTableSize);
	for (auto &name : memberNames) {
		std::getline(progFile, name, '\0');
	}
}

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
		case opGiveOptions: giveOptions(); break;
		case opJumpEvent: jumpEvent(); break;
		case opStartTimer: startTimer(); break;
		case opTimedEvent: createTimedEvent(); break;
		case opPushObj:	pushObj(); break;
		case opCall: call(); break;
		case opReturn: returnOp(); break;
		case opReturnTrue: returnTrue(); break;
		case opHot: hot(); break;
		case opInitArray: initArray(); break;
		case opPushElem: pushElem(); break;
		case opAssignElem: assignElem(); break;
		case opEq: compEq(); break;
		case opJump: jump(); break;
		case opJumpFalse: jumpFalse(); break;

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
		if (varId >= globalVarStart) {
			varId -= globalVarStart;
			stack.push(globalVars[varId]);
		}
		else { //local variable
			stack.push(stack.local(varId));
		}
	}
	else { //data member
		int objectId = stack.pop().getIntValue();
		if (objectId == selfObjId) {
			objectId = currentObject;
		}
		stack.push(objects[objectId].members[varId]);
	}
}



/** Pop the top value off the stack and print it. */
void CTigVM::print() {
	writeText(stack.pop().getStringValue());
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

/** Pop a value off the stack and assign it to a (global) variable. */
void CTigVM::assign() {
	CTigVar value = stack.pop();

	CTigVar var = stack.pop();

	int varId = var.getIntValue();

	if (varId < globalVarStart) {
		stack.local(varId) = value;
		return;
	}


	if (varId < memberIdStart && varId >= globalVarStart) {
		varId -= 100;
		if (globalVars[varId].type == tigArray) {
			int index = stack.pop().getIntValue();
			globalVars[varId].pArray->elements[index] = value;
		}
		else
			globalVars[varId] = value;
		return;
	}

	// variable is an object member 
	int objectId = stack.pop().getIntValue();
	if (objectId == selfObjId)
		objectId = currentObject;
	if (objects[objectId].members[varId].type == tigArray) {
		int index = stack.pop().getIntValue();
		objects[objectId].members[varId].pArray->elements[index] = value;
	}
	else
		objects[objectId].members[varId] = value;

}

/** Go into 'awaiting string from user' mode. */
void CTigVM::getString() {
	escape = true;
	status = vmAwaitString;
}

/** Pop the two top values of the stack, add and push the result. */
void CTigVM::add() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	CTigVar result;
	if (op1.type == tigString) {
		result.setStringValue(op1.getStringValue() + op2.getStringValue());
	}
	stack.push(result);
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
	stack.pushObj(objId);
}

/** Execute the member function currently on the stack, leaving the returned value on the stack. */
void CTigVM::call() {
	int obj = stack.pop().getObjId();
	int memberId = readWord();
	if (obj == selfObjId)
		obj = currentObject;

	currentObject = obj;
	stack.push(currentObject);
	stack.push(pc); //save return address
	pc = objects[obj].members[memberId].getFuncAddress();;
	int varCount = readByte();
	stack.reserveLocalVars(varCount);


	/*
	int addr = stack.pop().getFuncAddress();

	/////////////////////////////
	//should retrieve object id here

	stack.push(pc); //save return address
	pc = addr;
	int varCount = readByte();
	stack.reserveLocalVars(varCount);
	*/
}


/** Return execution to the calling address left on the stack, leaving the current top value at the top. */
void CTigVM::returnOp() {
	CTigVar value = stack.pop();
	int localVarCount = stack.pop().getIntValue();
	stack.freeLocalVars(localVarCount);
	pc = stack.pop().getIntValue(); //get calling address
	currentObject = stack.pop().getIntValue(); //get calling object
	stack.push(value);
}

/** Return execution to the calling address left on the stack, leaving 1 at the top. */
void CTigVM::returnTrue() {
	int localVarCount = stack.pop().getIntValue();
	stack.freeLocalVars(localVarCount);
	pc = stack.pop().getIntValue(); //get calling address
	currentObject = stack.pop().getIntValue(); //get calling object
	stack.push(1);
}

/** Pass on text identified as hot for the user to do something with. */
void CTigVM::hot() {
	std::string text = readString();
	hotText(text, readWord());
}

/** Initialise an array structure, leaving a Tig value referencing it on the stack. */
void CTigVM::initArray() {
	CTigVar newArray;
	newArray.type = tigArray;
	newArray.pArray = new CArray(); //TO DO: use shared_ptr
	int arraySize = readWord();
	newArray.pArray->elements.resize(arraySize);
	for (auto &element : newArray.pArray->elements) {
		TigVarType type = (TigVarType)readByte();
		switch (type) {
		case tigString:
			element.setStringValue(readString()); break;
		case tigInt:
			element.setIntValue(readWord()); break;
		case tigObj:
			element.setObjId(readWord()); break;
		case tigUndefined:
			readWord(); //can just throw this away
			break;
		}
	}
	stack.push(newArray);
}

/** Push the value in given array element onto the stack for further processing. */
void CTigVM::pushElem() {
	int varId = stack.pop().getIntValue();
	int index = stack.pop().getIntValue();

	CTigVar arrayVar;

	if (varId < memberIdStart) {
		arrayVar = globalVars[varId];
		stack.push(arrayVar.pArray->elements[index]);
	}
	else {
		int objId = index;
		index = stack.pop().getIntValue();
		stack.push(objects[objId].members[varId].pArray->elements[index]);
	}

}

/** Assign the value on the stack to the array address also on the stack. */
void CTigVM::assignElem() {
	//TO DO: what happened here? Do I do this elsewhere? Investigate
}

/** Compare the top 2 values on the stack for equality. */
void CTigVM::compEq() {
	CTigVar op2 = stack.pop();
	CTigVar op1 = stack.pop();
	int result = 0;
	if (op1.type == tigInt && op2.type == tigInt) {
		result = op1.getIntValue() == op2.getIntValue();
	}
	else if (op1.type == tigInt && op2.type == tigFloat) {
		result = op1.getIntValue() == op2.getFloatValue();
	}
	else if (op1.type == tigFloat && op2.type == tigInt) {
		result = op1.getFloatValue() == op2.getIntValue();
	}
	else if (op1.type == tigFloat && op2.type == tigFloat) {
		result = op1.getFloatValue() == op2.getFloatValue();
	} 
	else if (op1.type == tigString && op2.type == tigString) {
		result = op1.getStringValue() == op2.getStringValue();
	}

	stack.push(result);
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
	auto it = find_if(globalVarNameTable.begin(), globalVarNameTable.end(),
		[&](TGlobalVarNameRec& nameRec) { return nameRec.name == varName; });
	CTigVar var;
	if (it != globalVarNameTable.end())
		var = globalVars[it->id - globalVarStart];
	else
		std::cerr << "\nGlobal variable" + varName + " not found!";
	return var;
}

/** Return a copy of the identified member. */
CTigVar CTigVM::getMember(CTigVar & obj, int memberId) {
	return objects[obj.getObjId()].members[memberId];
}

/** Return a copy of the identified member. */
CTigVar CTigVM::getMember(int objNo, int memberId) {
	return objects[objNo].members[memberId];
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
		stack.push(pc);
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
	return getMember(objNo, memberNo).getIntValue();
}

/** Return the class of the given object.*/
int CTigVM::getClass(int objNo) {
	return objects[objNo].classId;
}

bool CTigVM::inheritsFrom(int objId, int classId) {
	while (objects[objId].classId != 0) {
		if (objects[objId].classId == classId)
			return true;
		objId = objects[objId].classId;
	}
	return false;
}

bool CTigVM::inheritsFrom(CObjInstance* obj, CObjInstance* classObj) {
	while (obj->classId != 0) {
		if (obj->classId == classObj->id)
			return true;
		obj = &objects[obj->classId];
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
