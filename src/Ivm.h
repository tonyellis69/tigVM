#pragma once

#include <string>
#include <type_traits>

#include "ITigObj.h"

/** An interface for the Tig virtual machine. */
class Ivm {
public:
	virtual int getConst(const std::string& constName) = 0;
	virtual ITigObj* getObject(const std::string& objName) = 0;


	virtual int getStackValueInt() = 0;
	virtual float getStackValueFloat() = 0;
	virtual std::string getStackValueStr() = 0;
};