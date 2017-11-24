#include "vmApp.h"

#include "..\3DEngine\src\BaseApp.h"

//#include<iostream>

void CVMapp::setApp(CBaseApp * app) {
	pApp = app;
}

void CVMapp::writeText(std::string & text) {
	TVMmsg msg;
	msg.type = vmMsgString;
	msg.text = text;
	pApp->vmMessage(msg);
}
