#include "vmApp.h"

#include "..\3DEngine\src\BaseApp.h"

//#include<iostream>

void CVMapp::setApp(CBaseApp * app) {
	pApp = app;
}

void CVMapp::writeText(std::string & text) {
	TvmAppMsg msg;
	bool bold = false;

	//parse text into chunks of the same style, and send each to be printed by the user app.
	//current text = whole text

	//can we find '\'?
	//and it's followed by a special escape character?
		//make current text = preceding text
		//write current text
		//change style
	//loop
	
	std::string writeTxt = text;
	std::string remainingTxt = text;
	TvmAppMsg styleChangeMsg;
	while (remainingTxt.size() ) {
		styleChangeMsg.type = appNone;
		size_t found = remainingTxt.find('\\');
		if (found != std::string::npos) {
			if (remainingTxt[found + 1] == 'b') {
				styleChangeMsg.type = appWriteBold;
				bold = !bold;
				styleChangeMsg.integer = int(bold);
			}	
			//other markup checks here
		}
	
		writeTxt = remainingTxt.substr(0, found);
		remainingTxt = remainingTxt.substr(found + 2);
		msg.type = appWriteText;
		msg.text = writeTxt;
		pApp->vmMessage(msg);

		pApp->vmMessage(styleChangeMsg);
	}
	
}

void CVMapp::writeHotText(std::string & text, int id) {
	TvmAppMsg msg;
	msg.type = appWriteText;
	msg.text = text;
	pApp->vmMessage(msg);
}
