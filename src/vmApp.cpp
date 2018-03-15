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
	pApp->vmMessage(msg);
}

void CVMapp::hotText(std::string & text, int memberId) {
	TvmAppMsg msg;
	msg.type = appHotText;
	msg.text = text;
	msg.integer = memberId;
	pApp->vmMessage(msg);
}




