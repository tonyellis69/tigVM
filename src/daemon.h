#pragma once

#include <vector>

struct timedEvent {
	int delay; ///<Number of seconds before this event triggers.
	int codeAddr; ///<Code to execute when event triggers.
};


/** A class for managing events that occur in real time. */
class CDaemon {
public:
	CDaemon() { timer = -1; };
	void startTimer(); 
	void update(int dtSeconds);
	void addEvent(timedEvent& event);
	
	std::vector<timedEvent> events; ///<A list of time-specific events.
	int timer; ///<Tracks the time in seconds;
};