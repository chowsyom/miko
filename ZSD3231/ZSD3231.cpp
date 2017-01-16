#include "ZSD3231.h"


void ZSD3231ClockClass::begin(void){}
bool ZSD3231ClockClass::updateDateTime(uint8_t* date)
{
	bool flag = false;
	uint8_t diff = (uint8_t) (((millis() - last_read_time)/1000)%256);
	if(last_read_time > 0 && diff < 10 && (date_cache[0] + diff < 60))
	{
		memcpy(date, date_cache, 7);
		date[0] = date_cache[0] + diff;
	}
	else
	{
		uint8_t n = 0;
		Wire.beginTransmission(RTC_Address);
		flag = Wire.write(uint8_t(0x00)) > 0;
		Wire.endTransmission();
		if(flag)
		{
			flag = Wire.requestFrom(RTC_Address, 7) > 0;
			bool uFlag = false;
			while ( flag && Wire.available())
			{
				date[n] = Wire.read();
				if (n == 2) {
					date[n] = (date[n] & 0x7f);
				}
				date[n] = (((date[n] & 0xf0) >> 4) * 10) + (date[n] & 0x0f);
				date_cache[n] = date[n];
				n++;
				uFlag = true;
			}
			if(uFlag) last_read_time = millis();
			delayMicroseconds(1);
			Wire.endTransmission();
		}		
	}
	return flag;
}
void ZSD3231ClockClass::getDateTime(uint8_t* date, char* info)
{
	updateDateTime(date);
	sprintf(info,"20%02d-%02d-%02d %02d:%02d:%02d, %d",date[6],date[5],date[4],date[2],date[1],date[0],date[3]);
}
inline byte decToBcd(byte val)
{
	// Convert normal decimal numbers to binary coded decimal
	return ( (val / 10 * 16) + (val % 10) );
}
//…Ë÷√SD3231 ±º‰
void ZSD3231ClockClass::setDateTime(uint8_t* date)
{
	Wire.beginTransmission(RTC_Address);
	Wire.write(uint8_t(0x00));
	uint8_t i;
	for (i = 0; i < 7; i++)
	{
		date[i] = date[i] == 0xFF ? 0 : date[i];
		uint8_t val = decToBcd(date[i]);
		if (i == 2) {
		  val = val & 0b10111111;
		}
		Wire.write(val);
	}
	Wire.endTransmission();
	// Clear OSF flag
	Wire.write(uint8_t(0x0f));
	Wire.endTransmission();
	Wire.requestFrom(RTC_Address, 1);
	uint8_t ctrl = Wire.read();
	Wire.write(uint8_t(0x0f));
	Wire.write(ctrl & 0b01111111);
	Wire.endTransmission();
	delay(1);
}
ZSD3231ClockClass ZClock;