#pragma once

#include "vm.h"
//#include "..\3DEngine\src\BaseApp.h"


class CBaseApp;
/** A wrapper for the Tig virtual machine, enabling it to access the user app. */
class CVMapp : public CTigVM {
public:
	CVMapp() {};
	void setApp(CBaseApp* app);
	void writeText(std::string& text);

	CBaseApp* pApp; ///<Pointer to the user application.
};