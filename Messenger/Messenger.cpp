#include "Messenger.h"


void MessengerClass::begin(void)
{
	//Serial.begin(115200);
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.begin(9600);
		softSerial.listen();
	#endif
	#ifdef _ZESP8266_H_
		ZWifi.begin();
		ZClock.begin();
	#else
		Wire.begin();
		//wdt_enable(WDT_TIMEOUT);
	#endif
	CLIENT_ID = EEPROM.read(0);//放在ZWifi.begin();之后，要初始化EEPROM.begin(100);
	//CMD[3] = CLIENT_ID;
	this->sayHi();
	
	#ifdef VirtualWire_h
		vw_set_ptt_inverted(true); // Required for DR3100
		vw_setup(2000);  // Bits per sec
		vw_rx_start();       // Start the receiver PLL running
	#endif
}
void MessengerClass::sayHi()
{
	uint8_t date[7];
	char timeStr[22];
	#ifdef _ZESP8266_H_
		char info[25];
		sprintf(info,"Hi! This's Miko No.%d\n",CLIENT_ID);
		this->write(info);
	#else
		ZClock.getDateTime(date,timeStr);
		this->print("Hi! This's Miko No.");
		this->println(CLIENT_ID,DEC);
		this->println(timeStr);
	#endif
}
//void MessengerClass::loop(void)
//{
//	bool hasOtherMsg = false;
//	uint8_t* msg;
//	uint8_t msgSize=0;
//	this->loop(&hasOtherMsg, msg, &msgSize);
//}
void MessengerClass::loop(void)//(bool* hasOtherMsg, uint8_t* msg, uint8_t* msgSize)
{
	#ifdef _ZESP8266_H_
		ZWifi.loop();
	#endif
	if(millis() - debug_start_time > TIMEOUT_1_HOUR)
	{
		DEBUG_MODE = false;
	}	
	uint8_t len = VW_MAX_MESSAGE_LEN;
	uint8_t receiveBuf[len];
	//如果收到远程指令，处理指令
	if(this->receiveMsg(receiveBuf, &len))
	{
		this->handleMessage(receiveBuf, len);
	}
	//else this->checkMsgTimeout();
	//#ifndef _ZESP8266_H_
		//if(millis() > time_to_reset && isIdle) resetEnable = true;
		//feed dog
		//if(!resetEnable) wdt_reset();
	//#endif
}
void MessengerClass::setIdle(bool idle)
{
  //isIdle = !isWorking && idle;
}
//发送消息
// void MessengerClass::sendMsg(void)
// {
//   this->sendMsg(msg_cache, sendedSize);
// }
void MessengerClass::sendMsg(char* msg)
{
  this->sendMsg((uint8_t *)msg, strlen(msg));
}
void MessengerClass::sendMsg(uint8_t* msg, uint8_t len)
{
  //replyEnable = false;
  //isLocalSetting = false;
  //isWorking = true;
  msg[3] = CLIENT_ID;
  if(!isRepeatMsg) lastTxCmdID++;
  if(lastTxCmdID>0xFE) lastTxCmdID = 1;
  msg[4] = lastTxCmdID;
 //  if (msg[2] == CLIENT_ID)
 //  {
	// for (uint8_t i = 0; i < len+1; i++)
	// {
	//   receiveBuf[i] = msg[i];
	// }
	// receiveSize = len;
	// //isLocalSetting = true;
 //  }
 //  else
 //  {
	//msg_cache = msg;
	//sendedSize = len;
	//#ifdef VirtualWire_h
	//	vw_send(msg, len);
	//	vw_wait_tx();
	#ifdef _ZESP8266_H_
		ZWifi.send((byte *) msg, len);
	#else
		Serial.write((byte *) msg, len);
		delay(10);
		//this->print("Miko-");
		//this->print(CLIENT_ID,DEC);
		this->print("Sended ");
		this->printString(msg, len);
	#endif
	delay(10);
	//rf_start_time = millis();
	// if (msg[5] != 0xFD && msg[5] != 0xFC)
	// {
	//   replyEnable = true; //0xFE|0x7C|0x01|0xFF|0x01|0x01~0xFE
	// }
	
  //}
  isRepeatMsg = false;
  //isWorking = false;
}

//接收信息
uint8_t MessengerClass::receiveMsg(uint8_t* buf, uint8_t* buflen)
{
	//#ifdef VirtualWire_h
		//return vw_get_message(buf, buflen);
	#ifdef _ZESP8266_H_
		*buflen = ZWifi.receive(buf, buflen);
	#else
		if (Serial.available())
		{
			delay(100);
			size_t len = Serial.available();
			len = len>*buflen?*buflen:len;
			byte sbuf[len];
			Serial.readBytes(sbuf, len);
			Serial.flush();
			memcpy(buf, sbuf, len);
			*buflen = len;
		}
		else *buflen = 0;
	#endif
	if(*buflen>0) // Non-blocking
	{	
		buf[*buflen] = 0;
		#ifndef _ZESP8266_H_
		    if(*buf==0xFE)
			{
				//this->print("Miko-");
				//this->print(CLIENT_ID,DEC);
				this->print("Got ");
			}
		    this->printString(buf, *buflen);
		    delay(10);
		#endif
	}
	return *buflen;
}
#ifndef _ZESP8266_H_
void MessengerClass::printString(uint8_t* msg, uint8_t len)
{
	if(*msg==0xFE)
	{
		uint8_t p = 0;
		this->print("[");
		this->print(len, DEC);
		this->print(" B = ");
		while (p < len)
		{
			this->print(*(msg+p),HEX);
			this->print(" ");
			p++;
			if(p>15)
			{
				this->print("...");
				break;
			}
		}
		this->print("]\n");
	}
	else
	{
		this->write((char*) msg);
		this->print("\n");
	}
}
#else
void MessengerClass::printString(uint8_t* msg, uint8_t len){}
#endif

//超时重发
// void MessengerClass::checkMsgTimeout(void)
// {
//   if (replyRetryTimes < 3)
//   {
// 	if (millis() - rf_start_time > RF_TIMEOUT )//&& replyEnable)
// 	{
// 	  isRepeatMsg = true;
// 	  //this->sendMsg();
// 	  replyRetryTimes++;
// 	}
//   }
//   else
//   {
// 	//replyEnable = false;
// 	replyRetryTimes = 0;
// 	this->println("Sended fail.");
// 	delay(100);
//   }
// }
inline void itoBin(uint8_t num, char* buf)
{
	bool flag = false;
	uint8_t j = 0;
	for(uint8_t i=8;i>0;i--)
	{
		uint8_t k = 1 << (i-1);
		if(k & num) flag = true;//从高位到低位，第一个1开始记录
		if(flag) buf[j++] = (k & num) ? '1':'0';
	}
	if(!flag) buf[j++] = '0'; 
	buf[j] = 0;
}
//处理接收到的指令
void MessengerClass::handleMessage(uint8_t* receiveBuf, uint8_t buflen)
{
  //解析指令
  //0xFE|0x7C|接收端设备号(0xFF全部)|发送端设备号|指令ID|指令/状态编号(0xFD)|指令值(0xFF=0)
  //指令/状态编号 : 0x01=设置继电器状态,0x02=,0x03=设置系统时间,0x04=设置定时列表,0x05=红外释热传感器,0x06=温湿度,0x07=PM2.5,0x08=CO2,0xFD=回复收到指令长度
  //0xFE|0x7C|0x01|0xFF|0x01|0x01|0x01~0xFE
  //0xFE|0x7C|0x01|0xFF|0x02|0x02|0x01~0xFE
  //0xFE|0x7C|0x01|0xFF|0x03|0x03|0xFF 0x2F 0x0E 0x04 0x16 0x09 0x10 //秒,分,时,周,日,月,年 2016-09-22 w4 14:47:00
  //0xFE|0x7C|0x01|0xFF|0x04|0x04|0x01,0x0F,0x1F,0x0F,0x2F,...    //0x04设置定时 week1(1111111),hour11,minute11,hour12,minute12,week2(1111111),hour21,minute21,hour22,minute22,...,

  //FE 7C 01 01 01 02 0F
  //FE 7C 01 01 02 03 FF 2F 0E 08 16 09 10
  //FE 7C 01 01 03 04 08:00-10:00,01;00:00-00:00,02;00:00-00:00,03;00:00-00:00,04;08:00-09:00,05;17:00-18:00,06;21:00-22:00,07;22:00-19:30,08
  //FE 7C 01 01 04 FC 01 05 
  if (buflen > 6 && receiveBuf[0] == 0xFE && (receiveBuf[1] == 0x7C || receiveBuf[1] == 0x7F)) //以0xFE, 0x7C|0x7F开头
  {
	//*gotMsg = true;
	//*receiverId = receiveBuf[2];
	//匹配设备号和指令id
	//同一个设备发送了相同的指令则忽略
	//0xDB为Debug模式不受限制
	if (receiveBuf[2] == CLIENT_ID || receiveBuf[2] == 0xFF)
	{
		unsigned int ticket;
		ticket = (receiveBuf[3] << 8) + receiveBuf[4];
		bool isRepeat = false;
		for(uint8_t i=RX_CMD_TICKET_LEN;i>0;i--)
		{
			if(rxCmdTicket[i-1]==ticket)
			{
				isRepeat = true;
				break;
			}
		}
		if(receiveBuf[3] == 0xDB)
		{
			debug_start_time = millis();
			DEBUG_MODE = true;
		}
		delayMicroseconds(1);
		if(!isRepeat || receiveBuf[3] == 0xDB)
		{
			//isWorking = true;
			//lastRxClientID = receiveBuf[3];
			//lastRxCmdID = receiveBuf[4];
			rxCmdTicket[rxCmdTicketIdx] = ticket;
			rxCmdTicketIdx++;
			if(rxCmdTicketIdx>RX_CMD_TICKET_LEN-1) rxCmdTicketIdx = 0;

			uint8_t date[7];
			uint8_t* addr;
			addr = receiveBuf + 6;
			//设置应答
			if(receiveBuf[5] != 0xFB && receiveBuf[5] != 0xFC && receiveBuf[5] != 0xFD && receiveBuf[3] != 0xDB)
			{
				for(uint8_t i=0;i<RESPONSE_NUMBER;i++)
				{
					if(!responsePool[i].enable)
					{
						responsePool[i].receiver = receiveBuf[3];
						responsePool[i].cmd = receiveBuf[5];
						responsePool[i].enable = true;
						break;
					}
				}
				//通用回复
				if(receiveBuf[1] != 0x7F)
				{
					uint8_t sbuf[2];
					sbuf[0] = receiveBuf[5];
					sbuf[1] = 0xFF;
					this->response(sbuf,2);
					delay(100);
				}
			}
			switch (receiveBuf[5])
			{
				case 0xFB:
					#ifndef _ZESP8266_H_
					this->println("Bye!");
					#endif
					DEBUG_MODE = false;
				break;
				case 0xFC:
					//retMsg[2] = receiveBuf[3];
					//retMsg[6] = 0xFC;
					if (*addr != 0xFF)
					{
						infoId = *addr;
						infoReplyTimes = (*(addr + 1));
					}
				break;
				case 0x01:
				  //设置设备编号
				  
				  if(receiveBuf[3] == 0xDB)
				  {
					if(*addr != 0xFC)
					{
						CLIENT_ID = *addr;
						EEPROM.write(0, *addr);
						#ifdef _ZESP8266_H_
							EEPROM.commit();
							char info[20];
							sprintf(info,"Updata ID to No.%d\n",*addr);
							this->write(info);
						#else
							this->print("Updata ID to No.");
							this->println(*addr,DEC);
						#endif
						delay(100);
					}
					else
					{
						this->sayHi();
					}
				  }
				  break;
				case 0x02:
				  //设置继电器状态
				  if(*addr != 0xFC) 
				  {
					  ZClock.updateDateTime(date);
					  uint8_t relays = *addr == 0xFF ? 0 : *addr;
					  for (uint8_t i = 0; i < RELAY_NUMBER; i++)
					  {
						if((relays >> i) & 0x01)
						{
							bool isOn = *(addr+1)==0x01;
							optByCmdFlag[i].time[0]=date[2];
							optByCmdFlag[i].time[1]=date[1];
							optByCmdFlag[i].isOn=isOn;
							optByCmdFlag[i].enable=true;
							uint8_t j = 1 << i;
							if(isOn) onByRMCmd |= j;
							else onByRMCmd &= ~j;
						}
					  }
					  if(receiveBuf[3] == 0xDB)
					  {
					  	#ifdef _ZESP8266_H_
					  		//to BIN
					  		char s_bin[9];
							itoBin(onByRMCmd, s_bin);
					  		char info[25];
							sprintf(info,"Updated Relays:%s\n",s_bin);
							this->write(info);
						#else
							this->print("Updated Relays:");
							this->println(onByRMCmd, BIN);
						#endif
						delay(100);
					  }
				  }
				  else if(receiveBuf[3] == 0xDB) 
				  {
				  		#ifdef _ZESP8266_H_
					  		//to BIN
					  		char s_bin[9];
							itoBin(relayState, s_bin);
					  		char info[23];
							sprintf(info,"Relays State:%s\n",s_bin);
							this->write(info);
						#else
					  		this->print("Relays State:");
					  		this->println(relayState, BIN);
					  	#endif	
					  	delay(100);
				  }
				  break;
				case 0x03:
				  //更新系统时间
				  if(*addr != 0xFC) 
				  {
					  uint8_t i=0;
					  while (*addr != 0 && i < 7)
					  {
						date[i] = *addr;
						addr++;
						i++;
					  }
					  ZClock.setDateTime(date);//uint8_t date[7];
					  if(receiveBuf[3] == 0xDB)
					  {
							this->print("Updated ");
					  }
				  }
				  if(receiveBuf[3] == 0xDB) 
				  {
				  	char timeStr[22];
				  	ZClock.getDateTime(date,timeStr);
				  	#ifdef _ZESP8266_H_
						char info[50];
						sprintf(info,"System Clock:%s\n",timeStr);
						this->write(info);
					#else
						this->print("System Clock:");
						this->println(timeStr);
					#endif
				  }
				  break;
				//case 0x04:
				  //更新定时时间列表
				  //this->updateTimingList(addr);
				  //break;
				default:
					//外部处理otherMsgs[MESSAGE_CACHE_NUMBER];
					for(uint8_t i=0;i<MESSAGE_CACHE_NUMBER;i++)
					{
						if(otherMsgs[i].id == receiveBuf[5])
						{
							if(receiveBuf[5] == 0xFD && otherMsgs[i].sub_id != receiveBuf[6])
							{
								continue;
							}
							memcpy((otherMsgs[i].msg)+1, receiveBuf + 6, buflen - 6);
							otherMsgs[i].enable = true;
							break;
						}
						
					}
					break;
			}
		}
		//isWorking = false;
	}
  }
}
bool MessengerClass::listen(uint8_t* prMsg)
{
	bool hvflag = false;
	bool exflag = false;
	uint8_t j = 0xFF;
	for(uint8_t i=0;i<MESSAGE_CACHE_NUMBER;i++)
	{
		if(otherMsgs[i].id == 0 && j==0xFF) j = i;
		if(otherMsgs[i].id == *prMsg) 
		{
			//如果是FD，还需判断后面一位
			if(*prMsg == 0xFD && otherMsgs[i].sub_id != *(prMsg+1))
			{
				continue;
			}
			exflag = true;
			if(otherMsgs[i].enable) 
			{
				hvflag = true;
				otherMsgs[i].enable = false;
			}
			break;
		}
	}
	if(!exflag&&0<=j&&j<MESSAGE_CACHE_NUMBER)
	{
		otherMsgs[j].id = *prMsg;
		if(*prMsg == 0xFD) otherMsgs[j].sub_id = *(prMsg+1);
		otherMsgs[j].msg = prMsg;
	}
	return hvflag;
}
bool MessengerClass::checkRemoteCommand(uint8_t id)
{
	return onByRMCmd & id;
}
void MessengerClass::setRelayState(uint8_t state)
{
	relayState = state;
}
OptByCmd* MessengerClass::getOptByCmdFlag(uint8_t id)
{
	return &optByCmdFlag[id];
}
void MessengerClass::response(uint8_t* buf, uint8_t len)
{
	//匹配指令{0xFE, 0x7C, 0, 0, 1,0xFD, 0, 0, 0, 0};
	for(uint8_t i=0;i<RESPONSE_NUMBER;i++)
	{
		if(responsePool[i].enable)
		{
			if (responsePool[i].cmd==buf[0])
			{
				uint8_t sbuf[len+7];
				memcpy(sbuf,CMD,5);
				memcpy(sbuf+6,buf,len);
				sbuf[1] = 0x7C;
				sbuf[2] = responsePool[i].receiver;
				sbuf[5] = 0xFD;
				sbuf[len+6] = 0;
				responsePool[i].enable = false;
				this->sendMsg(sbuf, len+7);
			}
		}
	}
	
}
void MessengerClass::debugInfo(const char* name, const char* stateInfo)
{
	if (infoId != 0)
 	{
		if(NULL == debugInfoName_1st) debugInfoName_1st = name;
		else if(debugInfoName_1st==name)
		{
			infoId_cnt = 0;
			if(infoId == 0xDB) infoReplyTimes = 0;
		}
		infoId_cnt++;
		if(infoId == 0xDB || (infoId == infoId_cnt && (millis() - relay_start_time  > 500)))
		{
			relay_start_time = millis();
	    	if(infoReplyTimes > 0)
	    	{
				infoReplyTimes--;
				char info[32];
				sprintf(info,"%d.%s=%s\n",infoId_cnt,name,stateInfo);//.c_str()
				this->write(info);
			}
			else
			{
				infoId = 0;
				infoId_cnt = 0;
				debugInfoName_1st = NULL;
			}
		}
	}


	//单个ID，每500毫秒返回一个，避免太快
 // 	if ((infoId == infoId_cnt && (millis() - relay_start_time  > 500)) || infoId == 0xDB)//{0xFE, 0x7C, 0, 0, 1,0xFD, 0xFC, 0xFF, 0, 0};
 // 	{
 //    	relay_start_time = millis();
 //    	if(loopEnd && infoId == 0xDB) infoReplyTimes = 0;
 //    	if(infoReplyTimes > 0)
 //    	{
	// 		infoReplyTimes--;
	// 		char info[32];
	// 		sprintf(info,"%d.%s=%s\n",infoId_cnt,name,stateInfo);//.c_str()
	// 		this->write(info);
	// 	}
	// 	else
	// 	{
	// 		infoId = 0;
	// 	}
	// }
}
void MessengerClass::print(const char* msg)
{
	if(DEBUG_MODE)
	{
		#ifdef _ZESP8266_H_
			ZWifi.write(msg);
		#else
			Serial.print(msg);
		#endif
	}
}
void MessengerClass::println(const char* msg)
{
	if(DEBUG_MODE)
	{
		#ifdef _ZESP8266_H_
			uint8_t len = strlen(msg);
	    	char buf[len+3];
	    	buf[len] = 0x0D;
	    	buf[len+1] = 0x0A;
	    	buf[len+2] = 0;
	    	memcpy(buf,msg,len);
			ZWifi.write(buf);
		#else
			Serial.println(msg);
		#endif
	}
}
//打印信息
#ifndef _ZESP8266_H_
	//非esp8266时使用
	void MessengerClass::print(int msg,uint8_t type)
	{
		if(DEBUG_MODE)
		{
			Serial.print(msg,type);
		}
	}
	void MessengerClass::println(int msg,uint8_t type)
	{
		if(DEBUG_MODE)
		{
			Serial.println(msg,type);
		}
	}
#endif
void MessengerClass::write(const char* msg)
{
	if(DEBUG_MODE)
	{
	#ifdef _ZESP8266_H_
		ZWifi.write(msg);
	#else
		Serial.print(msg);
	#endif
	}
}

//打印定时列表
void MessengerClass::printTimingList(uint8_t* timeList, uint8_t len)
{
	#ifndef _ZESP8266_H_
		byte buf[len+6];
		buf[0] = 0xFE;
		buf[1] = 0x7C;
		buf[2] = 0xFF;
		buf[3] = 0xDB;
		buf[4] = 0x01;
		buf[5] = 0x04;
		//memcpy(buf+6,timeList,len);
		uint8_t i = 0;
		while(i < len)
		{
			*(buf+6+i) = *(timeList+i)==0 ? 0xFF : *(timeList+i);
			i++;
		}
		Serial.write(buf,len+6);
	#else
		ZWifi.printTimingList(timeList,len);
	#endif
}


void MessengerClass::parseTimingList(char* input, uint8_t* timeArray, uint8_t* len){}


MessengerClass Messenger;