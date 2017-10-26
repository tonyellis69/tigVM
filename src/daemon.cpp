#include "daemon.h"

/** Re-initialise the timer to zero. */
void CDaemon::startTimer() {
	timer = 0;
}

/** Update by the given interval, and do any work that's necessary. */
void CDaemon::update(int dtSeconds) {
	if (timer < 0)
		return;

	timer += dtSeconds;

	//check the events list for any due events
	for (auto event = std::begin(events); event != std::end(events);) {
		if (timer >= event->delay) {
			//execute code


			event = events.erase(event);
		}
		else
			event++;
	}
}

/** Add a timed event to the list. */
void CDaemon::addEvent(timedEvent & event) {
	events.push_back(event);
}
