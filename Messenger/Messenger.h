#ifndef _MessengerClass_H_
#define _MessengerClass_H_


#define _PRINT_TIMGING_MODE_

#if ARDUINO <100
	#include <WProgram.h>
#else
	#include <Arduino.h>
#endif

//#include <VirtualWire.h>
//#include <ZESP8266.h>
#include <ZESP8266_UDP.h>
#ifndef _ZESP8266_H_
	//#include <avr/wdt.h>
	#include <ZSD3231.h>
	#include <EEPROM.h>
#else
	#include <ZUdpClock.h>
#endif

#include <ZOptByCmd.h>


#ifndef VW_MAX_MESSAGE_LEN
	#define VW_MAX_MESSAGE_LEN 254
#endif
#define WDT_TIMEOUT WDTO_8S //WDTO_500MS WDTO_1S WDTO_2S WDTO_8S
//#define RF_TIMEOUT 5000   //毫秒
#define RX_CMD_TICKET_LEN 5
#define RELAY_NUMBER 6
#define MESSAGE_CACHE_NUMBER 5
#define RESPONSE_NUMBER 10

struct MessageCache
{
	uint8_t* msg;
	bool enable = false;
	uint8_t id = 0;
	uint8_t sub_id = 0;
};
struct Responses
{
	bool enable = false;
	uint8_t receiver = 0;
	uint8_t cmd = 0;
};

class MessengerClass
{
	

	private:
		uint8_t CLIENT_ID = 0;
		bool DEBUG_MODE = false;
		const unsigned long TIMEOUT_1_HOUR = 3600000;//1小时
		//const unsigned long time_to_reset = 86400000;//24小时
		//uint8_t receiveBuf[VW_MAX_MESSAGE_LEN];
		const uint8_t CMD[10] = {0xFE, 0x7C, 0, 0, 1,0, 0, 0, 0, 0};
		OptByCmd optByCmdFlag[RELAY_NUMBER];
		//unsigned long rf_start_time = 0;
		unsigned long debug_start_time = 0;
		unsigned long relay_start_time = 0;
		unsigned int rxCmdTicket[RX_CMD_TICKET_LEN];
		//uint8_t replyRetryTimes = 0;
		//bool resetEnable = false;
		//bool replyEnable = false;
		//bool isLocalSetting = false;
		bool isRepeatMsg = false;
		//bool isIdle = false;
		//bool isWorking = false;
		//uint8_t* msg_cache;
		//uint8_t sendedSize = 0;
		//uint8_t receiveSize = 0;
		uint8_t infoReplyTimes = 0;
		uint8_t infoId = 0;
		uint8_t infoId_cnt = 0;
		const char* debugInfoName_1st;
		uint8_t onByRMCmd = 0;
		uint8_t relayState = 0;
		uint8_t rxCmdTicketIdx = 0;
		uint8_t lastTxCmdID = 1;
		MessageCache otherMsgs[MESSAGE_CACHE_NUMBER];
		Responses responsePool[RESPONSE_NUMBER];
		void sayHi();
		#ifndef _ZESP8266_H_
		void print(int msg,uint8_t type);
		void println(int msg,uint8_t type);
		#endif
	public:
		void begin(void);
		void loop(void);
		void setIdle(bool idle);
		//void sendMsg(void);
		void sendMsg(char* msg);
		void sendMsg(uint8_t* msg, uint8_t len);
		uint8_t receiveMsg(uint8_t* buf, uint8_t* buflen);
		//void checkMsgTimeout(void);
		void handleMessage(uint8_t* receiveBuf, uint8_t buflen);
		bool checkRemoteCommand(uint8_t id);
		OptByCmd* getOptByCmdFlag(uint8_t id);
		void debugInfo(const char* name, const char* stateInfo);
		void response(uint8_t* buf, uint8_t len);
		bool listen(uint8_t* msg);
		void printString(uint8_t* msg, uint8_t len);
		void print(const char* msg);
		void println(const char* msg);
		void write(const char* msg);
		void printTimingList(uint8_t* timeList, uint8_t len);
		//void handleSerialInputMsg(void);
		void parseTimingList(char* input, uint8_t* timeArray, uint8_t* len);
		void setRelayState(uint8_t state);
};
extern MessengerClass Messenger;
#endif