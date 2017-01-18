#ifndef _ZESP8266_H_
#define _ZESP8266_H_

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ZUdpClock.h>


//#define _DEBUG_MODE_	//仅插串口时打开


//#ifdef _SERVER_MODE_
#define reserve 0
#define MAX_WIFI_CLIENTS 5
// #else
// 	#define reserve 1
// 	#define MAX_WIFI_CLIENTS 4
// #endif
#define CLIENT_POOL_SIZE 5
#define TCP_PORT 	8888	//端口范围0-65535
#define UDP_PORT 	8889

class ZESP8266
{
	public:
		uint8_t CLIENT_ID = 0xFF;
		//WiFiUDP udp;
		virtual void begin(void);
		void loop(void);
		void send(byte* sbuf, uint8_t len);
		void send(byte* sbuf, uint8_t len,bool cmdOnly);
		uint8_t receive(uint8_t* buf, uint8_t* buflen);
		void sendUdp(uint8_t *buf, uint8_t len);
		void sendUdp(uint8_t *buf, uint8_t len, IPAddress& ip, uint16_t port);
		void print(String msg);
		void println(String msg);
		void write(char* msg);
		void printTimingList(uint8_t* timeList, uint8_t len);

	private:

		const char *AP[4][2] = {{"ZZY-2.4G","Auglx19780712"},{"ES_001","go2map123"},{"ZZY-2.4G-EXT","Auglx19780712"},{"ZZY-EXT","Auglx19780712"}};
		//const int udpPort = 7777;//端口范围0-65535
		const unsigned long ALIVETIMEOUT = 300000;
		const unsigned int HEARTBEATTIMEOUT = 60000;//int
		unsigned long wifiClientStartConnectTime[MAX_WIFI_CLIENTS + reserve];
		unsigned long heartBeatStartTime = 0;//no -
		//unsigned long serverStartConnectTime = 0;//no -
		const byte CMD[9] = {0xFE,0x7C,0xFF,0xDB,0x01,0x02,0xFC,0,0};
		const byte HEART_BEAT_CMD[9] = {0x23, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x2E, 0x23, 0};
		WiFiClient wifiClients[MAX_WIFI_CLIENTS + reserve];
		WiFiUDP udp;
		uint16_t pool_port[CLIENT_POOL_SIZE];
		uint8_t pool_client_ip[CLIENT_POOL_SIZE][6];
		uint8_t debugClientIdx = 0xFF;
		bool OTA_enable = false;
		void configWifi(uint8_t id);
		void initWifi(uint8_t id);
		void printIP();
		void printMAC();
		void OTAinit(void);
		void connectServer();
		uint8_t handleMessage(uint8_t* buf,size_t len,uint8_t* rbuf, uint8_t clientIdx);
		void showConnected(WiFiClient& wificlient);
		uint8_t checkLocalCmd(byte* buf);
		bool checkWifiClient(uint8_t idx);
		void printString(uint8_t* str, uint8_t len);
		uint8_t receiveTCP(uint8_t* rbuf, uint8_t* len, uint8_t clientIdx);
		void updateCleintPool(IPAddress& ip, uint16_t port, uint8_t* buf);
};
extern ZESP8266 ZWifi;
#endif