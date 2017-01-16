#ifndef _ZSD3231_H_
#define _ZSD3231_H_

#if ARDUINO <100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#include <SPI.h>
#include <Wire.h>

#define RTC_Address   0x68  //ʱ������IIC���ߵ�ַ

class ZSD3231ClockClass
{
	public:
		void begin(void);
		bool updateDateTime(uint8_t* date);
		void getDateTime(uint8_t* date, char* info);
		void setDateTime(uint8_t* date);
	private:
		uint8_t date_cache[7];
		unsigned long last_read_time = 0;
};
extern ZSD3231ClockClass ZClock;
#endif