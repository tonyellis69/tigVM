#pragma once


#include "..\\glm\glm\glm.hpp"

#include "vm.h"
//#include "..\3DEngine\src\BaseApp.h"

enum TvmAppMsgType {appNone,appWriteText,appWriteBold,appHotText,appPurge,appClearWin,
	appOpenWin, appOpenWinModal, appMsg, appTempTxt, appPause, appUnpause, appClearMarked,
	appHotTxtChange, appSolidifyTmpText, appCollapseTmpTxt, appDisplayNarrativeChoice,
	appFlush, appSetStyle, appClearToBookmark, appSetLineFadein, appFinishDisplay};
struct TvmAppMsg {
	TvmAppMsgType type;
	std::string text;
	int integer;
	int integer2;
	glm::i32vec2 pos;
};

class CBaseApp;
/** A wrapper for the Tig virtual machine, enabling it to access the user app. */
class CVMapp : public CTigVM {
public:
	CVMapp() {};
	void setApp(CBaseApp* app);
	void writeText(std::string& text);
	//void hotText(std::string& text, int memberId, int objectId);
	void purge(unsigned int hotFnCallId);
	void clearWin();
	void clearMarkedText();
	void openWindow(int objId, bool modal);
	void messageApp(int p1, int p2);
	void logText(std::string& text);
	void temporaryText(int onOff, int winId);
	void handlePause(bool pauseOn);
	void flush();

	CBaseApp* pApp; ///<Pointer to the user application.
};