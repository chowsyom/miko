#ifndef _ZUDPCLOCK_H_
#define _ZUDPCLOCK_H_

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

//#define DEBUG_MODE

#define START_YEAR (2016)
#define SECOND_DAY    (86400)    //60*60*24
#define SECOND_HOUR    (3600)    //60*60
#define SECOND_MIN    (60)    //60
#define NTP_PACKET_SIZE 48 // NTP time stamp is in the first 48 bytes of the message

class ZUdpClockClass
{
	private:
		const unsigned short int daysOfPerMonthBeginYear[2][13] =
		{
			/* Normal years.  */
			{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
			/* Leap years.  */
			{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
		};
		// Calculate time starts on Jan 1 2016. Since 1900 in seconds, that's 3660595200:
		const unsigned long preYears = 3660595200UL;
		// Local time in 8th time zone
		const unsigned long eighthTimeZone = 28800UL;
		unsigned int localPort = 2390;      // local port to listen for UDP packets
		//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
		IPAddress timeServerIP; // time.nist.gov NTP server address
		const char* ntpServerName[5] = {"time.windows.com","time.nist.gov","ntp.sjtu.edu.cn","s1b.time.edu.cn","s1c.time.edu.cn"};
		uint8_t ntpServerIdx = 0;
		uint8_t requestTryCnt = 0;
		unsigned long lastGotTime = 0;
		unsigned long timeStamp = 0;
		byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
		void sendNTPpacket(IPAddress& address);
		bool isLeapYear(int year);
		int getDaysForYear(int year);
	public:
		void begin(void);
		bool getTime(unsigned long& secsSince1900, bool isForce = false);
		bool updateDateTime(uint8_t* date, bool isForce = false);
		void getDateTime(uint8_t* date,char* info);
		void setDateTime(uint8_t* date);
		void sec2date(unsigned long second, int& year, uint8_t& month, uint8_t& day);
		void getDayOfWeek(int year, int month, int day, uint8_t& dow);
		void sec2time(unsigned long seconds, uint8_t& hour, uint8_t& minute, uint8_t& second);
};
extern ZUdpClockClass ZClock;
#endif
