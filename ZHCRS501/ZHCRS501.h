#ifndef _ZHCRS501_H_
#define _ZHCRS501_H_

#if ARDUINO <100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include<SoftwareSerial.h>
#include <Messenger.h>

//#define HCSR501_FQ_DELAY 1000   //∫¡√Î
#define HCSR501_NUMBERS 2

extern "C"
{
	extern bool hcrs501_isFired(uint8_t sensorId, uint8_t HCSR501_PIN);
	extern bool hcrs501_continue_active(uint8_t id, bool isFired);
}
#endif