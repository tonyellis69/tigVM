#pragma once

#include "vm.h"
//#include "..\3DEngine\src\BaseApp.h"

enum TvmAppMsgType {appNone,appWriteText,appWriteBold};
struct TvmAppMsg {
	TvmAppMsgType type;
	std::string text;
	int integer;
};

class CBaseApp;
/** A wrapper for the Tig virtual machine, enabling it to access the user app. */
class CVMapp : public CTigVM {
public:
	CVMapp() {};
	void setApp(CBaseApp* app);
	void writeText(std::string& text);
	void writeHotText(std::string& text, int id);

	CBaseApp* pApp; ///<Pointer to the user application.
};