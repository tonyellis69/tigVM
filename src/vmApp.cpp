#include "vmApp.h"

#include "..\3DEngine\src\BaseApp.h"


void CVMapp::setApp(CBaseApp * app) {
	pApp = app;
}

void CVMapp::writeText(std::string & text) {
	TvmAppMsg msg;
	msg.type = appWriteText;
	//TO DO: hot text mode causes a different msg, so app handles text differently?
	msg.text = text;
	msg.integer = window;
	pApp->vmMessage(msg);
}
/*
void CVMapp::hotText(std::string & text, int memberId, int objectId) {
	TvmAppMsg msg;
	msg.type = appHotText;
	msg.text = text;
	msg.integer = memberId;
	msg.integer2 = NULL;
	pApp->vmMessage(msg);
}*/

void CVMapp::purge(unsigned int hotFnCallId) {
	TvmAppMsg msg;
	msg.type = appPurge;
	msg.integer = hotFnCallId;
	pApp->vmMessage(msg);
	//whatever hot text was removed, remove also from the list of hot text function calls.
}



void CVMapp::clearWin() {
	TvmAppMsg msg;
	msg.type = appClearWin;
	msg.integer = window;
	pApp->vmMessage(msg);
}

void CVMapp::clearMarkedText() {
	TvmAppMsg msg;
	msg.type = appClearMarked;
	msg.integer = window;
	pApp->vmMessage(msg);
}

void CVMapp::openWindow(int objId, bool modal) {
	TvmAppMsg msg;
	if (modal)
		msg.type = appOpenWinModal;
	else
		msg.type = appOpenWin;
	msg.integer = objId;
	pApp->vmMessage(msg);
}

void CVMapp::messageApp(int p1, int p2) {
	TvmAppMsg msg;
	msg.type = appMsg;
	msg.integer = p1;
	msg.integer2 = p2;
	pApp->vmMessage(msg);
}

void CVMapp::logText(std::string & text) {
	liveLog << text;
	sysLog << text;
}

void CVMapp::temporaryText(int onOff, int winId) {
	TvmAppMsg msg;
	msg.type = appTempTxt;
	msg.integer = onOff;
	msg.integer2 = winId;
	pApp->vmMessage(msg);
}

/** Pass on a notification from the vm to pause or unpause. */
void CVMapp::handlePause(bool pauseOn) {
	TvmAppMsg msg;
	if (pauseOn)
		msg.type = appPause;
	else
		msg.type = appUnpause;
	pApp->vmMessage(msg);
}



