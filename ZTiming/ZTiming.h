#ifndef _ZTiming_H_
#define _ZTiming_H_

#if ARDUINO <100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <EEPROM.h>
//#include <ZSD3231.h>
#include <ZOptByCmd.h>
#include <Messenger.h>

#define TIMELIST_NUMBER 8
#define TIMELIST_ROM_ADDR 10


class ZTimingClass
{
	private:
		uint8_t timeList[TIMELIST_NUMBER][5];
		uint8_t timingMsg[TIMELIST_NUMBER*5+2];
	public:
		void begin(void);
		void loop(void);
		boolean checkTimingList(uint8_t addr1, uint8_t addr2, uint8_t* date);
		boolean checkTimingList(uint8_t addr1, uint8_t addr2, OptByCmd* optByCmd, uint8_t* date);
		void updateTimingListFromRom(uint8_t romAddr);
		void getTimingListFromRom(uint8_t romAddr, uint8_t* list, uint8_t* dataSize);
		void writeTimingListToRom(uint8_t romAddr, uint8_t* time);
		void updateTimingList(uint8_t* addr);
		void printTimingList(void);
};

extern ZTimingClass ZTiming;
#endif