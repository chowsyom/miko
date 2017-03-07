#ifndef _ZESP8266_H_
#define _ZESP8266_H_

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <ZUdpClock.h>


//#define _DEBUG_MODE_	//仅插串口时打开
#define _USING_OTA_	//ESP12

#ifdef _USING_OTA_
	#include <ArduinoOTA.h>
#endif

#define CLIENT_POOL_SIZE 5

struct TargetClient
{
	uint32_t ip = 0;
	uint16_t port = 0;
	uint8_t id = 0;
	uint8_t failCnt = 0xFF;
};

class ZESP8266_UDP
{
	private:
		const int UDP_PORT = 8889;	//端口范围0-65535
		const char *AP[4][2] = {{"ZZY-2.4G","Auglx19780712"},{"ES_001","go2map123"},{"ZZY-2.4G-EXT","Auglx19780712"},{"ZZY-EXT","Auglx19780712"}};
		//const int udpPort = 7777;//端口范围0-65535
		const unsigned long ALIVETIMEOUT = 300000;
		//unsigned long serverStartConnectTime = 0;//no -
		const byte CMD[9] = {0xFE,0x7C,0xFF,0xDB,0x01,0x02,0xFC,0,0};
		const byte HEART_BEAT_CMD[9] = {0x23, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x23, 0};
		const unsigned long TIMEOUT_1_HOUR = 3600000;//1小时
		WiFiUDP udp;
		uint8_t CLIENT_ID = 0xFF;
		TargetClient pool_client_ip[CLIENT_POOL_SIZE];
		uint32_t debugClientIP = 0;
		bool OTA_enable = false;
		unsigned long debug_start_time = 0;
		void configWifi(uint8_t id);
		void initWifi(uint8_t id);
		void printIP();
		//void printMAC();
		#ifdef _USING_OTA_
			void OTAinit(void);
		#endif
		//void connectServer();
		uint8_t handleMessage(uint8_t* buf,size_t len,uint8_t* rbuf, IPAddress& ip);
		void showConnected();
		uint8_t checkLocalCmd(byte* buf);
		//bool checkWifiClient(uint8_t idx);
		void printString(uint8_t* str, uint8_t len);
		void updateCleintPool(IPAddress& ip, uint16_t port, uint8_t* buf);
	public:
		ZESP8266_UDP();
		void begin(void);
		void loop(void);
		void send(byte* sbuf, uint8_t len);
		void send(byte* sbuf, uint8_t len,bool cmdOnly);
		uint8_t receive(uint8_t* buf, uint8_t* buflen);
		void sendUdp(uint8_t *buf, uint8_t len);
		void sendUdp(uint8_t *buf, uint8_t len, IPAddress& ip, uint16_t port);
		void write(const char* msg);
		void printTimingList(uint8_t* timeList, uint8_t len);		
};
extern ZESP8266_UDP ZWifi;
#endif