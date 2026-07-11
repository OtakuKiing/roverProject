#include <useful-functions.h>

bool timer(uint32_t start, uint32_t duration) {  // non-blocking timer 
	uint32_t now = millis();
	if ((now - start) < duration) {
		return false;
	} else {return true;}
}