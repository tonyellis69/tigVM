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

void CVMapp::hotText(std::string & text, int memberId, int objectId) {
	TvmAppMsg msg;
	msg.type = appHotText;
	msg.text = text;
	msg.integer = memberId;
	msg.integer2 = NULL;
	pApp->vmMessage(msg);
}

void CVMapp::purge(int memberId, int objId) {
	TvmAppMsg msg;
	msg.type = appPurge;
	msg.integer = memberId;
	msg.integer2 = objId;
	pApp->vmMessage(msg);
}

void CVMapp::clearWin() {
	TvmAppMsg msg;
	msg.type = appClearWin;
	msg.integer = window;
	pApp->vmMessage(msg);
}

void CVMapp::openWindow(int objId) {
	TvmAppMsg msg;
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




