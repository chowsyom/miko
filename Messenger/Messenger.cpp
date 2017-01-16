#include "Messenger.h"

#ifdef _SOFT_SERIAL_DEBUG_MODE_
	SoftwareSerial softSerial(RXPIN, TXPIN);
#endif

void MessengerClass::begin(void)
{
	Serial.begin(115200);
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
	CLIENT_ID = EEPROM.read(0);//����ZWifi.begin();֮��Ҫ��ʼ��EEPROM.begin(100);
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
		this->println(String(timeStr));
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
	uint8_t len = VW_MAX_MESSAGE_LEN;
	uint8_t receiveBuf[len];
	//����յ�Զ��ָ�����ָ��
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
//������Ϣ
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
		this->print("\nSended");
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

//������Ϣ
uint8_t MessengerClass::receiveMsg(uint8_t* buf, uint8_t* buflen)
{
	//#ifdef VirtualWire_h
		//return vw_get_message(buf, buflen);
	#ifdef _ZESP8266_H_
		*buflen = ZWifi.receive(buf, buflen);
	#else
		if (Serial.available())
		{
			delay(100);//��ֹ��Ϣ���ֶζ�ȡ
			size_t len = Serial.available();
			len = len>*buflen?*buflen:len;
			byte sbuf[len];
			Serial.readBytes(sbuf, len);
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
				this->print("\nGot ");
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
		this->print(" B][");
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
		this->println("]");
	}
	else
	{
		this->write((char*) msg);
	}
}
#else
void MessengerClass::printString(uint8_t* msg, uint8_t len){}
#endif

//��ʱ�ط�
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
		if(k & num) flag = true;//�Ӹ�λ����λ����һ��1��ʼ��¼
		if(flag) buf[j++] = (k & num) ? '1':'0';
	}
	if(!flag) buf[j++] = '0'; 
	buf[j] = 0;
}
//�������յ���ָ��
void MessengerClass::handleMessage(uint8_t* receiveBuf, uint8_t buflen)
{
  //����ָ��
  //0xFE|0x7C|���ն��豸��(0xFFȫ��)|���Ͷ��豸��|ָ��ID|ָ��/״̬���(0xFD)|ָ��ֵ(0xFF=0)
  //ָ��/״̬��� : 0x01=���ü̵���״̬,0x02=,0x03=����ϵͳʱ��,0x04=���ö�ʱ�б�,0x05=�������ȴ�����,0x06=��ʪ��,0x07=PM2.5,0x08=CO2,0xFD=�ظ��յ�ָ���
  //0xFE|0x7C|0x01|0xFF|0x01|0x01|0x01~0xFE
  //0xFE|0x7C|0x01|0xFF|0x02|0x02|0x01~0xFE
  //0xFE|0x7C|0x01|0xFF|0x03|0x03|0xFF 0x2F 0x0E 0x04 0x16 0x09 0x10 //��,��,ʱ,��,��,��,�� 2016-09-22 w4 14:47:00
  //0xFE|0x7C|0x01|0xFF|0x04|0x04|0x01,0x0F,0x1F,0x0F,0x2F,...    //0x04���ö�ʱ week1(1111111),hour11,minute11,hour12,minute12,week2(1111111),hour21,minute21,hour22,minute22,...,

  //FE 7C 01 01 01 02 0F
  //FE 7C 01 01 02 03 FF 2F 0E 08 16 09 10
  //FE 7C 01 01 03 04 08:00-10:00,01;00:00-00:00,02;00:00-00:00,03;00:00-00:00,04;08:00-09:00,05;17:00-18:00,06;21:00-22:00,07;22:00-19:30,08
  //FE 7C 01 01 04 FC 01 05 
  if (buflen > 6 && receiveBuf[0] == 0xFE && (receiveBuf[1] == 0x7C || receiveBuf[1] == 0x7F)) //��0xFE, 0x7C|0x7F��ͷ
  {
	//*gotMsg = true;
	//*receiverId = receiveBuf[2];
	//ƥ���豸�ź�ָ��id
	//ͬһ���豸��������ͬ��ָ�������
	//0xDBΪDebugģʽ��������
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
		if(receiveBuf[3] == 0xDB) DEBUG_MODE = true;
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
			//����Ӧ��
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
				//ͨ�ûظ�
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
				//case 0xFD:

					//FE 7C 34 33 01 FD CMD VAL ... 0

					//uint8_t hbyte,lbyte;
					/*if(*addr == 0xFC)
					{
					hbyte = (*(addr + 1)) == 0xFF ? 0 : (*(addr + 1));
					lbyte = (*(addr + 2)) == 0xFF ? 0 : (*(addr + 2));
					this->println(hbyte * 0x100 + lbyte, DEC);
					delay(100);
					}
					else*/
					//if(*addr == 0xFF)//&&receiveBuf[3]==msg_cache[2])//ȷ�Ͻ������Ƿ�ƥ��
					//{
					  	//�жϻظ��յ�ָ����Ƿ���ȷ FE 7C 34 33 1E FD FF 8 0
						// if (*(addr + 1) != sendedSize) {
					 //  		#ifdef _ZESP8266_H_
					 //  			char info[50];
						// 		sprintf(info,"Sended=%d, responsed=%d, Error!resending...\n",sendedSize,*(addr + 1));
						// 		this->write(info);
						// 	#else
						//   		this->print("Sended=");
						//   		this->print(sendedSize,DEC);
						//   		this->print(", responsed=");
						//   		this->print(*(addr + 1),DEC);
						// 		this->println(", Error!resending...");
						// 	#endif
						// 	delay(100);
						// 	this->sendMsg();
						// 	delay(1000);
					 //  	}
					 //  	else {
						// 	replyEnable = false;
						// 	replyRetryTimes = 0;
						// 	this->println("Sended done.");
					 //  	}
					//}
				//break;
				case 0x01:
				  //�����豸���
				  
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
							this->print(*addr,DEC);
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
				  //���ü̵���״̬
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
				  //����ϵͳʱ��
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
					this->print("System Clock:");
					this->println(String(timeStr));
				  }
				  break;
				//case 0x04:
				  //���¶�ʱʱ���б�
				  //this->updateTimingList(addr);
				  //break;
				default:
					//�ⲿ����otherMsgs[MESSAGE_CACHE_NUMBER];
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
			//�����FD�������жϺ���һλ
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
	//ƥ��ָ��{0xFE, 0x7C, 0, 0, 1,0xFD, 0, 0, 0, 0};
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
void MessengerClass::debugInfo(String name,String stateInfo,uint8_t sId)
{
 	if(millis() - relay_start_time  > 500 || infoId == 0xDB)
 	{
 		relay_start_time = millis();
	 	if ((infoId == sId || infoId == 0xDB) && infoReplyTimes > 0)//{0xFE, 0x7C, 0, 0, 1,0xFD, 0xFC, 0xFF, 0, 0};
	    {
			infoReplyTimes--;
			char info[32];
			sprintf(info,"%d.%s=%s\n",sId,name.c_str(),stateInfo.c_str());
			this->write(info);
	    }
		else
		{
			infoId == 0;
		}
	}
}

//��ӡ��Ϣ
void MessengerClass::print(String msg)
{
	if(DEBUG_MODE)
	{
	#ifdef _ZESP8266_H_
		ZWifi.print(msg);
	#else
		Serial.print(msg);
	#endif
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.print(msg);
	#endif
	}

}
void MessengerClass::println(String msg)
{
	if(DEBUG_MODE)
	{
	#ifdef _ZESP8266_H_
		ZWifi.println(msg);
	#else
		Serial.println(msg);
	#endif
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.println(msg);
	#endif
	}

}
void MessengerClass::print(int msg,uint8_t type)
{
	if(DEBUG_MODE)
	{
	#ifndef _ZESP8266_H_
		Serial.print(msg,type);
	#endif
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.print(msg,type);
	#endif
	}

}
void MessengerClass::println(int msg,uint8_t type)
{
	if(DEBUG_MODE)
	{
	#ifndef _ZESP8266_H_
		Serial.println(msg,type);
	#endif
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.println(msg,type);
	#endif
	}
}
void MessengerClass::write(char* msg)
{
	if(DEBUG_MODE)
	{
	#ifdef _ZESP8266_H_
		ZWifi.write(msg);
	#else
		Serial.print(msg);
	#endif
	#ifdef _SOFT_SERIAL_DEBUG_MODE_
		softSerial.print(msg);
	#endif
	}
}

//��ӡ��ʱ�б�
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