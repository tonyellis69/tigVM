#pragma once

#include "vm.h"
//#include "..\3DEngine\src\BaseApp.h"

enum TvmAppMsgType {appNone,appWriteText,appWriteBold,appHotText,appPurge};
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
	void purgeMsg(int memberId, int objId);

	CBaseApp* pApp; ///<Pointer to the user application.
};