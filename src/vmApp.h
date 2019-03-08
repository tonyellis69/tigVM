#pragma once

#include "vm.h"
//#include "..\3DEngine\src\BaseApp.h"

enum TvmAppMsgType {appNone,appWriteText,appWriteBold,appHotText,appPurge,appClearWin,
	appOpenWin,appMsg};
struct TvmAppMsg {
	TvmAppMsgType type;
	std::string text;
	int integer;
	int integer2;
};

class CBaseApp;
/** A wrapper for the Tig virtual machine, enabling it to access the user app. */
class CVMapp : public CTigVM {
public:
	CVMapp() {};
	void setApp(CBaseApp* app);
	void writeText(std::string& text);
	void hotText(std::string& text, int memberId, int objectId);
	//void purge(int memberId, int objId);
	void purge(unsigned int hotFnCallId);
	void clearWin();
	void openWindow(int objId);
	void messageApp(int p1, int p2);

	CBaseApp* pApp; ///<Pointer to the user application.
};