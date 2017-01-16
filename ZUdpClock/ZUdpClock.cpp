#include "ZUdpClock.h"

WiFiUDP udp;

void ZUdpClockClass::sendNTPpacket(IPAddress& address)
{
	#ifdef DEBUG_MODE
	Serial.println("sending NTP packet...");
	#endif
	// set all bytes in the buffer to 0
	memset(packetBuffer, 0, NTP_PACKET_SIZE);
	// Initialize values needed to form NTP request
	// (see URL above for details on the packets)
	packetBuffer[0] = 0b11100011;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xEC;  // Peer Clock Precision
	// 8 bytes of zero for Root Delay & Root Dispersion
	packetBuffer[12]  = 49;
	packetBuffer[13]  = 0x4E;
	packetBuffer[14]  = 49;
	packetBuffer[15]  = 52;
	// all NTP fields have been given values, now
	// you can send a packet requesting a timestamp:
	udp.beginPacket(address, 123); //NTP requests are to port 123
	udp.write(packetBuffer, NTP_PACKET_SIZE);
	udp.endPacket();
}
void ZUdpClockClass::begin(void)
{
	udp.begin(localPort);
}
bool ZUdpClockClass::getTime(unsigned long& secsSince1900, bool isForce)
{
	bool flag = false;
	unsigned long curSysSec = millis()/1000;
	if(!isForce && lastGotTime>0 && (curSysSec - timeStamp < 60))
	{
		secsSince1900 = lastGotTime + (curSysSec - timeStamp);
		flag = true;
	}
	else
	{
		if(requestTryCnt > 10)
		{
			requestTryCnt = 0;
			ntpServerIdx  = (ntpServerIdx+1)%5;
		}
		WiFi.hostByName(ntpServerName[ntpServerIdx], timeServerIP);
		#ifdef DEBUG_MODE
		Serial.print("Time server:");
		Serial.print(ntpServerName[ntpServerIdx]);
		Serial.print(" - ");
		Serial.println(timeServerIP);
		#endif
		sendNTPpacket(timeServerIP); // send an NTP packet to a time server
		// wait to see if a reply is available
		delay(1000);

		int cb = udp.parsePacket();
		if (!cb) 
		{
			#ifdef DEBUG_MODE
				Serial.println("no packet yet.");
			#endif
			if(lastGotTime>0 && !isForce)
			{
				secsSince1900 = lastGotTime + (curSysSec - timeStamp);
				flag = true;
			}
			else flag = false;
			requestTryCnt++;
		}
		else {
			requestTryCnt = 0;
			timeStamp = millis()/1000;
			#ifdef DEBUG_MODE
				Serial.print("packet received, length=");
				Serial.println(cb);
			#endif
			// We've received a packet, read the data from it
			udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
			//the timestamp starts at byte 40 of the received packet and is four bytes,
			// or two words, long. First, esxtract the two words:
			unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			// combine the four bytes (two words) into a long integer
			// this is NTP time (seconds since Jan 1 1900):
			lastGotTime = (highWord << 16 | lowWord) + 2;
			secsSince1900 = lastGotTime;
			flag = true;
		}
	}
	return flag;
}
bool ZUdpClockClass::updateDateTime(uint8_t* date, bool isForce)
{
	bool flag = false;
	unsigned long sec1900 = 0;
	if(this->getTime(sec1900,isForce))
	{
		unsigned long sec = sec1900 - preYears + eighthTimeZone;
		int year = 0;
		uint8_t month = 0;
		uint8_t day = 0;
		uint8_t dow = 0;
		uint8_t hour = 0;
		uint8_t minute = 0;
		uint8_t second = 0;
		this->sec2date(sec, year, month, day);
		this->sec2time(sec, hour, minute, second);
		this->getDayOfWeek(year, month, day, dow);
		date[0] = second;
		date[1] = minute;
		date[2] = hour;
		date[3] = dow;
		date[4] = day;
		date[5] = month;
		date[6] = (uint8_t) (year - 2000);
		flag = true;
	}
	return flag;
}
void ZUdpClockClass::getDateTime(uint8_t* date, char* info)
{
	updateDateTime(date);
	sprintf(info,"20%02d-%02d-%02d %02d:%02d:%02d, %d",date[6],date[5],date[4],date[2],date[1],date[0],date[3]);
}
//设置SD3231时间
void ZUdpClockClass::setDateTime(uint8_t* date)
{
}

//判断一个年份是否为闰年，是就返回1，不是就返回0
bool ZUdpClockClass::isLeapYear(int year)
{
	return ( (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0) );
}
//获取一年的天数
int ZUdpClockClass::getDaysForYear(int year)
{
	return (isLeapYear(year) ? 366 : 365);
}

//根据秒数计算日期
void ZUdpClockClass::sec2date(unsigned long second, int& year, uint8_t& month, uint8_t& day)
{
	int days = (int) (second / SECOND_DAY);
	int curYear = START_YEAR;
	int leftDays = days;

	//calc year
	int daysCurYear = getDaysForYear(curYear);
	while (leftDays >= daysCurYear)
	{
		leftDays -= daysCurYear;
		curYear++;
		daysCurYear = getDaysForYear(curYear);
	}
	year = curYear;

	//calc month and day
	int isLeepYear = isLeapYear(curYear);
	for (uint8_t i = 1; i < 13; i++)
	{
		if (leftDays < daysOfPerMonthBeginYear[isLeepYear][i])
		{
			month = (uint8_t) i;
			day = (uint8_t) (leftDays - daysOfPerMonthBeginYear[isLeepYear][i - 1] + 1);
			break;
		}
	}
}
//通过日期算星期几
void ZUdpClockClass::getDayOfWeek(int year, int month, int day, uint8_t& dow)
{
	if(month == 0 || day == 0)
	{
		dow = 0;
	}
	else 
	{
		if (month == 1 || month == 2) 
		{
			month += 12;
			year--;
		}
		dow = (uint8_t) (1 + (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7);
	}
}
//计算时间
void ZUdpClockClass::sec2time(unsigned long seconds, uint8_t& hour, uint8_t& minute, uint8_t& second)
{
	int leftSeconds = seconds % SECOND_DAY;
	hour = (uint8_t) (leftSeconds / SECOND_HOUR);
	minute = (uint8_t) ((leftSeconds % SECOND_HOUR) / SECOND_MIN);
	second = (uint8_t) (leftSeconds % SECOND_MIN);
}

ZUdpClockClass ZClock;