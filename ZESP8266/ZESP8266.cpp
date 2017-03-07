#include "ZESP8266.h"

//ESP8266WiFiMulti wifiMulti;
static WiFiServer server(TCP_PORT);
static IPAddress udp_broadcast_addr(255, 255, 255, 255);

ZESP8266::ZESP8266()
{
 	//server = WiFiServer((uint16_t) TCP_PORT);
 	//udp_broadcast_addr = IPAddress(255, 255, 255, 255);
}
bool ZESP8266::checkWifiClient(uint8_t idx)
{
	bool flag = false;
	if(0 <= idx && idx < MAX_WIFI_CLIENTS+reserve)
	{
		if(wifiClients[idx]&&wifiClients[idx].connected())
		{
			flag = true;
		}
	}
	return flag;
}
void ZESP8266::begin(void)
{
	EEPROM.begin(100);
	//init
	//server = WiFiServer(TCP_PORT);
	//udp_broadcast_addr = IPAddress(255, 255, 255, 255);

	delay(1000);
	size_t len = Serial.available();
	if(len>0)
	{
	    byte buf[len];
	    Serial.readBytes(buf, len);
	    checkLocalCmd(buf);
	}
	CLIENT_ID = EEPROM.read(0);
	#ifdef _DEBUG_MODE_
	if(Serial)
	{
		Serial.print("CLIENT_ID=");
		Serial.println(CLIENT_ID,DEC);
	}
	#endif
	if (CLIENT_ID != 0xFF || CLIENT_ID != 0)
	{
		delay(1000);
		char hostname[7];
    	sprintf(hostname, "miko-%02d", CLIENT_ID);
  		WiFi.hostname(hostname);
		this->initWifi(CLIENT_ID);
		this->printIP();
		this->printMAC();
		
		MDNS.begin(hostname);
		#ifdef _DEBUG_MODE_
		Serial.print("MDNS:");
		Serial.println(hostname);
		#endif
		server.begin();
		server.setNoDelay(true);  //加上后才正常些
		udp.begin(UDP_PORT);
		MDNS.addService("http", "tcp", 80);
		MDNS.addService("http", "tcp", TCP_PORT);
		ZClock.begin();
		#ifdef _USING_OTA_
			this->OTAinit();
		#endif
		delay(500);
	}
	else
	{
		#ifdef _DEBUG_MODE_
			if(Serial) Serial.print("Connected Fail.");
		#endif
	}
	for(uint8_t i=0;i<CLIENT_POOL_SIZE;i++)
	{
		pool_client_ip[i][1] = 0xFF;
	}
}

void ZESP8266::loop(void)
{
	if(OTA_enable) 
	{
		#ifdef _USING_OTA_
			ArduinoOTA.handle();
			if(debugClientIdx != 0xFF)
			{
				this->println("OTA stand by...");
				delay(1000);
			}
		#endif
	}
	else
	{
		//check if there are any new clients
		if (server.hasClient())
		{
			for (uint8_t i = reserve; i < MAX_WIFI_CLIENTS + reserve; i++) {
			//find free/disconnected spot
				if (!wifiClients[i] || !wifiClients[i].connected()) {
					if (wifiClients[i])
					{
						wifiClients[i].stop();
						if(i==debugClientIdx)
						{
							debugClientIdx = 0xFF;
						}
					}
					wifiClients[i] = server.available();
					wifiClientStartConnectTime[i] = millis();
					continue;
				}
			}
			//no free/disconnected spot so reject
			WiFiClient serverClient = server.available();
			serverClient.stop();
		}
		for (uint8_t i = reserve; i < MAX_WIFI_CLIENTS + reserve; i++) {
			if (millis() - wifiClientStartConnectTime[i] > ALIVETIMEOUT)
			{
				if (wifiClients[i])
				{
					wifiClients[i].stop();
					if(i==debugClientIdx)
					{
						debugClientIdx = 0xFF;
					}
				}
			}
		}
		if (millis() - heartBeatStartTime > HEARTBEATTIMEOUT)
		{
			heartBeatStartTime = millis();
			for (uint8_t i = 0; i < MAX_WIFI_CLIENTS + reserve; i++) {
				if (checkWifiClient(i))
				{
					wifiClients[i].write(HEART_BEAT_CMD,9);//Heart Beat
					delay(100);
				}
			}
		}
	}
}
#ifdef _USING_OTA_
void ZESP8266::OTAinit(void)
{
	// Port defaults to 8266
	ArduinoOTA.setPort(7777);
	// Hostname defaults to esp8266-[ChipID]
	char tmp[8];
    sprintf(tmp, "miko-%02d", CLIENT_ID);
	ArduinoOTA.setHostname(tmp);
	// No authentication by default
	//ArduinoOTA.setPassword((const char *)"712");
	ArduinoOTA.onStart([]() {
		if(Serial) Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		if(Serial) Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		if(Serial) Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		if(Serial) 
		{
			Serial.printf("Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
			else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
			else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
			else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
			else if (error == OTA_END_ERROR) Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();
}
#endif
void ZESP8266::send(byte* sbuf, uint8_t len, bool cmdOnly)
{
	
	if(cmdOnly)
	{
		byte buf[len+1];
		byte cmdbuf[len];
		uint8_t i = 0;
		uint8_t k = 0;
		while(i < len)
		{
			if(*(sbuf+i) == 0xFE && (*(sbuf+i+1) == 0x7C || *(sbuf+i+1) == 0x7F))
			{
				uint8_t j = 0;
				while(*(sbuf+i) != 0 && i < len)
				{
					cmdbuf[j++] = *(sbuf+i);
					i++;
				}
				if(cmdbuf[5]==0x04)
				{

					this->printTimingList((uint8_t*) (cmdbuf+6),j-6);
				}
				else
				{
					cmdbuf[j] = 0;
					this->send(cmdbuf, j+1);
				}
			}
			else
			{
				buf[k++] = *(sbuf+i);
				buf[k] = 0;
				i++;
			}
		}
		if (k>0)
		{
			if(!checkLocalCmd(buf))
			{
				char info[k+20];
				sprintf(info,"\nDebug Info# %s\n",buf);
				this->write(info);
			}
		}
	}
	else
	{
		this->send(sbuf, len);
	}
	
}
void ZESP8266::send(byte* sbuf, uint8_t len)
{
	if(!checkLocalCmd(sbuf))
	{
		//send udp
  		sendUdp(sbuf, len);
  		//send tcp
		for (uint8_t i = 0; i < MAX_WIFI_CLIENTS + reserve; i++)
		{
			if (checkWifiClient(i))
			{
				heartBeatStartTime = millis();
				if(i != debugClientIdx) 
				{
					wifiClients[i].write((byte*) sbuf, len);
				}
			}
		}
		this->print("\nSended# ");
		this->printString(sbuf, len);
	}
}

uint8_t ZESP8266::receive(uint8_t* rbuf, uint8_t* rbuflen)
{
	int len = 0;
	IPAddress remoteIP;
	bool fromTCP = true;
	//receivce UDP
	len = udp.parsePacket();
  	if (len>0)
  	{
  		len = *rbuflen > len ? len : *rbuflen;
		udp.read(rbuf, len);
		remoteIP = udp.remoteIP();
		uint16_t remotePort = udp.remotePort();
		updateCleintPool(remoteIP, remotePort, rbuf);
		fromTCP = false;
  	}
  	else
	{
		//receivce TCP	
		for (uint8_t i = 0; i < MAX_WIFI_CLIENTS + reserve; i++)
		{
			len = this->receiveTCP(rbuf, rbuflen, i);
			if(len>0)
			{
				wifiClientStartConnectTime[i] = millis();
				remoteIP = wifiClients[i].remoteIP();
				fromTCP = true;
				break;
			}
		}

	}
	if (len>0 && checkWifiClient(debugClientIdx))
	{
		char info[32];
		char tcp_chr[4] = {'T','C','P',0};
		char udp_chr[4] = {'U','D','P',0};
		char ip_str[16];
		uint8_t ip_len = 0;
		sprintf(ip_str,"%d.%d.%d.%d\0",remoteIP[0],remoteIP[1],remoteIP[2],remoteIP[3]);
		char* prt = fromTCP?tcp_chr:udp_chr;
		sprintf(info,"\nGot [%s:%s]# ",ip_str,prt);
		this->write(info);
		this->printString(rbuf,len);
	}
	//非指令不返回给串口
	if(*rbuf != 0xFE || (*(rbuf + 1) != 0x7C && *(rbuf + 1) != 0x7F) || (*(rbuf + 2) != CLIENT_ID && *(rbuf + 2) != 0xFF))
	{
		len = 0;
	}
	*rbuflen = len;
	return len;
}
uint8_t ZESP8266::receiveTCP(uint8_t* rbuf, uint8_t* rlen, uint8_t clientIdx)
{
	uint8_t ret=0;
	if (checkWifiClient(clientIdx))
	{
		if (wifiClients[clientIdx].available())
		{
			delay(100);
			size_t len = wifiClients[clientIdx].available();
			byte buf[len];
			byte *pr;
			pr = buf;
			while (wifiClients[clientIdx].available())
			{
				byte b = wifiClients[clientIdx].read();
				*pr = b;
				pr++;
			}
			uint8_t j = handleMessage(buf,len,rbuf, clientIdx);		
			//j==0 字节指令
			//j==1 本地拦截处理，不传递出去
			if(j==0)
			{
				ret = *rlen > len ? len : *rlen;
				memcpy(rbuf,buf,ret);
				*rlen = ret;
				
			}
			else if(j>1) //命令行翻译成字节指令
			{
				ret = j;
				*rlen=ret;
			}
		}
	}
	return ret;
}
void ZESP8266::updateCleintPool(IPAddress& ip, uint16_t port, uint8_t* buf)
{
	//FE 7C 02 03 01 FD 01 05
	if(*buf == 0xFE && (*(buf+1) == 0x7C || *(buf+1) == 0x7F) && *(buf+2) == CLIENT_ID)
	{
		bool haveFlag = false;
		uint8_t pool_idx = CLIENT_POOL_SIZE;	
		for(uint8_t i=0;i<CLIENT_POOL_SIZE;i++)
		{
			if(pool_idx == CLIENT_POOL_SIZE && pool_client_ip[i][1] > 1) 
			{
				pool_idx = i;			
			}
			if(pool_client_ip[i][0] == *(buf+3))
			{
				pool_client_ip[i][1] = 0;
				pool_client_ip[i][2] = uint8_t(ip[0]);
				pool_client_ip[i][3] = uint8_t(ip[1]);
				pool_client_ip[i][4] = uint8_t(ip[2]);
				pool_client_ip[i][5] = uint8_t(ip[3]);
				pool_port[i] = port;
				haveFlag = true;
				break;
			}
		}
		if(!haveFlag && pool_idx < CLIENT_POOL_SIZE)
		{			
			pool_client_ip[pool_idx][0] = *(buf+3);
			pool_client_ip[pool_idx][1] = 0;
			pool_client_ip[pool_idx][2] = uint8_t(ip[0]);
			pool_client_ip[pool_idx][3] = uint8_t(ip[1]);
			pool_client_ip[pool_idx][4] = uint8_t(ip[2]);
			pool_client_ip[pool_idx][5] = uint8_t(ip[3]);
			pool_port[pool_idx] = port;
		}
	}
}
void ZESP8266::printString(uint8_t* msg, uint8_t len)
{
	if (checkWifiClient(debugClientIdx))
    {
		if(*msg==0xFE)
		{
			uint8_t size = ((len*3)>117)?117:(len*3);
			char strBuf[size];
			uint8_t p = 0;
			uint8_t k = 0;
			while (p < len && k < size)
			{
				uint8_t b = *(msg+p);
				p++;
				//转16进制字符
				char hb = (b >> 4) + 0x30;  
	        	char lb = (b & 0x0f) + 0x30;
	  			hb = hb > 0x39 ? hb + 0x07 : hb;
	  			lb = lb > 0x39 ? lb + 0x07 : lb;
				strBuf[k++] = hb;
				strBuf[k++] = lb;
				strBuf[k++] = 0x20;	
			}
			strBuf[k] = 0;
			char info[size+11];
			sprintf(info,"[%d B]:[%s]\n",len,strBuf);
			this->write(info);
		}
		else
		{
			char info[len+3];
			memcpy(info,msg,len);
			info[len] = 0x0D;
			info[len+1] = 0x0A;
			info[len+2] = 0;
			this->write(info);
		}
	}
}

// void ZESP8266::configWifi(uint8_t id)
// {
//   #ifndef _HOME_MODE_
//     IPAddress IP(192, 168, 0, id);
//     IPAddress GATEWAY(192, 168, 0, 1);
//   #else
//     IPAddress IP(192, 168, 1, id);
//     IPAddress GATEWAY(192, 168, 1, 1);
//   #endif
//   IPAddress SUBNET(255, 255, 255, 0);
//   WiFi.config(IP, GATEWAY, SUBNET);
// }

void ZESP8266::initWifi(uint8_t id)
{
	//configWifi(id);
	//在这里检测是否成功连接到目标网络，未连接则阻塞。
	uint8_t APIdx = EEPROM.read(1)%4;
	WiFi.mode(WIFI_STA);
	WiFi.begin(AP[APIdx][0], AP[APIdx][1]);
	delay(1000);
	Serial.print("Connecting Wifi ");
	Serial.print(AP[APIdx][0]);
	Serial.println(" ...");
	while (WiFi.waitForConnectResult() != WL_CONNECTED)
	{
		#ifdef _DEBUG_MODE_
		Serial.println("Connection Failed! Rebooting...");
		#endif
		delay(3000);
		ESP.restart();
	}
	#ifdef _DEBUG_MODE_
	Serial.println("WiFi connected.");
	#endif
}

void ZESP8266::printIP()
{
	#ifdef _DEBUG_MODE_
	if(Serial)
	{
		Serial.print("IP address: ");
	 	char ip_str[15];
		IPAddress ip = WiFi.localIP();
		sprintf(ip_str,"%d.%d.%d.%d",ip[0],ip[1],ip[2],ip[3]);
	 	Serial.write(ip_str,15);
	}
	#endif
}
void ZESP8266::printMAC()
{
	#ifdef _DEBUG_MODE_
	if(Serial)
	{
	  byte mac[6];
	  WiFi.macAddress(mac);
	  Serial.print("MAC: ");
	  Serial.print(mac[5], HEX);
	  Serial.print(":");
	  Serial.print(mac[4], HEX);
	  Serial.print(":");
	  Serial.print(mac[3], HEX);
	  Serial.print(":");
	  Serial.print(mac[2], HEX);
	  Serial.print(":");
	  Serial.print(mac[1], HEX);
	  Serial.print(":");
	  Serial.println(mac[0], HEX);
	}
	#endif
}

// void ZESP8266::connectServer()
// {
// 	if (!wifiClients[0] || !wifiClients[0].connected())
// 	{
// 	if (serverStartConnectTime = 0 || millis() - serverStartConnectTime > 5000)
// 	{
// 	  	serverStartConnectTime = millis();
// 	  	if (wifiClients[0]) wifiClients[0].stop();
// 	  	wifiClients[0].connect(host, TCP_PORT);
// 	  	wifiClientStartConnectTime[0] = millis();
// 	    this->print("connect to ");
// 	    this->print(String(host));
// 	    this->print(":");
// 	    this->print(String(TCP_PORT));
// 	    this->println(" ...");
// 	}
// 	delay(500);
// 	}
// }


void ZESP8266::sendUdp(uint8_t *buf, uint8_t len)
{
	bool flag =false;
	for(uint8_t i=0;i<CLIENT_POOL_SIZE;i++)
	{
		//FE 7C 02 03 01 FD 01 05
		if(*buf == 0xFE & (*(buf+1) == 0x7C || *(buf+1) == 0x7F))
		{
			if(pool_client_ip[i][0] == *(buf+2) && pool_client_ip[i][1] < 2 )
			{
				pool_client_ip[i][1]++;
				IPAddress ip(pool_client_ip[i][2], pool_client_ip[i][3], pool_client_ip[i][4], pool_client_ip[i][5]);
				this->sendUdp(buf,len,ip,pool_port[i]);
				flag = true;
			}
		}
	}
	if(!flag)
	{
		//broadcast
		this->sendUdp(buf,len,udp_broadcast_addr,UDP_PORT);
	}
}
void ZESP8266::sendUdp(uint8_t *buf, uint8_t len, IPAddress& ip, uint16_t port)
{
	if(ip[0]!=0)
	{
		udp.beginPacket(ip, port);
  		udp.write(buf, len);
  		udp.endPacket();

		if (checkWifiClient(debugClientIdx))
    	{
	  		char info[32];
	  		sprintf(info,"udp to [%d.%d.%d.%d:%d]\n",ip[0],ip[1],ip[2],ip[3],port);
	  		this->write(info);
	  	}
  	}
}

void ZESP8266::showConnected(WiFiClient& wificlient)
{
	char info[32];
	IPAddress ip = WiFi.localIP();
	sprintf(info,"local ip=[%d.%d.%d.%d]\n",ip[0],ip[1],ip[2],ip[3]);
	uint8_t l = strlen(info);
	wificlient.write((byte*) info,l);
	for (uint8_t i = 0; i < MAX_WIFI_CLIENTS + reserve; i++) 
	{
		if (checkWifiClient(i)) 
		{
			IPAddress rip = wifiClients[i].remoteIP();
			char buf[32];
			sprintf(buf,"client[%d]=[%d.%d.%d.%d:%d]\n",i,rip[0],rip[1],rip[2],rip[3],wifiClients[i].remotePort());
			uint8_t len = strlen(buf);
			wificlient.write((byte*) buf,len);
		}
	}
}

//打印信息
void ZESP8266::print(String msg)
{
	if (checkWifiClient(debugClientIdx))
    {
		wifiClients[debugClientIdx].print(msg);
	}
}
void ZESP8266::println(String msg)
{
	if (checkWifiClient(debugClientIdx))
    {
		wifiClients[debugClientIdx].println(msg);
	}
}
void ZESP8266::write(char* msg)
{
	if (checkWifiClient(debugClientIdx))
    {
    	uint8_t len = strlen(msg);
    	byte buf[len+1];
    	buf[len] = 0;
    	memcpy(buf,msg,len);
		wifiClients[debugClientIdx].write(buf,len+1);
	}
}
uint8_t ZESP8266::checkLocalCmd(byte* buf)
{
	uint8_t ret = 0; 
	//debug
	if (*buf == 0x64 && *(buf + 1) == 0x65 && *(buf + 2) == 0x62 && *(buf + 3) == 0x75 && *(buf + 4) == 0x67)
    {
		if (*(buf + 6) == 0x30)
		{
			uint8_t id = *(buf + 8);
			if (id > 0 && id < 0xFF)
			{
				EEPROM.write(0,id);
				EEPROM.commit();
				CLIENT_ID = id;
				if(Serial)
				{
					Serial.print("Update CLIENT_ID=");
					Serial.println(id,DEC);
				}
				ret = 0x30;	
			}
		}
	 	else if (*(buf + 6) == 0x31)
		{
			uint8_t idx = *(buf + 8);
			if (idx > 0 && idx < 0xFF)
			{
				idx = (idx-0x30)%4;
				EEPROM.write(1,idx);
				EEPROM.commit();
				if(Serial)
				{
					Serial.print("Use AP: ");
					Serial.println(AP[idx][0]);
				}
				ret = 0x31;		
			}
		}
		else if(0x30 < *(buf + 6) &&  *(buf + 6) < 0x40) return *(buf + 6);
		else ret = 1;
    }
	return ret;
}

uint8_t ZESP8266::handleMessage(uint8_t* buf, size_t len, uint8_t* rbuf, uint8_t clientIdx)
{
	uint8_t rlen = 0;
	uint8_t cmd = 0;
	if (*buf != 0xFE || (*(buf + 1) != 0x7C && *(buf + 1) != 0x7F))
	{
		if(cmd = checkLocalCmd(buf))
		{	
			debugClientIdx = clientIdx;
			rlen = 1;
			if (cmd == 0x30)
			{
				uint8_t id = *(buf + 8);
				if (id > 0 && id < 0xFF)
				{
					memcpy(rbuf, CMD, 8);
					//{0xFE,0x7C,0xFF,0xDB,0x01,0x02,0xFC,0,0}
					*(rbuf+2) = 0xFF;
					*(rbuf+5) = 0x01;
					*(rbuf+6) = id;
					*(rbuf+7) = 0;
					rlen = 8;
					char info[20];
					sprintf(info,"Sync CLIENT_ID=%d\n",id);
					this->write(info);
				}
			}
			else if (cmd == 0x31)
			{
				uint8_t idx = *(buf + 8);
				if (idx > 0 && idx < 0xFF)
				{
					idx = (idx-0x30)%4;
					char info[12];
					sprintf(info,"Use AP[%d]: %s\n",idx,AP[idx][0]);
					this->write(info);
				}
			}
			// else if (cmd == 0x32)//DEBUG 2
			// {
			// 	uint8_t i = *(buf + 8) - 0x30;
			// 	if(i==0xFF) i = 0;
			// 	if(i>=0&&i<MAX_WIFI_CLIENTS)
			// 	{
			// 		if (checkWifiClient(i))
			// 		{
			// 			wifiClients[i].print("Hi!Message from ");
			// 			wifiClients[i].println(WiFi.localIP());
			// 			this->print("Sended Message to ");
			// 			this->println(String(wifiClients[i].remoteIP()));
			// 		}
			// 	}
			// }
			else if (cmd == 0x33)//DEBUG 3 OTA
			{
				OTA_enable = true;
			}
			else if (cmd == 1)
			{
				char info[21];
				sprintf(info,"Hi! ESP8266 No.%d\n",CLIENT_ID);
				this->write(info);
			}
		}
		else if(debugClientIdx == clientIdx)
		{
			if(char(*buf)=='H'&&char(*(buf+1))=='i')
			{
				memcpy(rbuf, CMD, 8);
				*(rbuf+2) = 0xFF;
				*(rbuf+5) = 0x01;
				*(rbuf+6) = 0xFC;
				*(rbuf+7) = 0;
				rlen = 8;
			}
			else if(char(*buf)=='l'&&char(*(buf+1))=='l')
			{
				char* space = strchr((char*) buf, char(0x20));//空格分隔
				if (space != 0)
				{
					byte idx = (byte) atoi(space+1);
					if(idx>0&&idx<11)
					{
						memcpy(rbuf, CMD, 8);
						*(rbuf+2) = CLIENT_ID;
						*(rbuf+5) = idx;
						*(rbuf+6) = 0xFC;
						*(rbuf+7) = 0;
						rlen = 8;
					}
				}
				if(rlen==0)
				{
					showConnected(wifiClients[debugClientIdx]);
					delay(100);
					memcpy(rbuf, CMD, 9);
					*(rbuf+2) = CLIENT_ID;
					*(rbuf+5) = 0xFC;
					*(rbuf+6) = 0xDB;
					*(rbuf+7) = 0x1E;
					*(rbuf+8) = 0;
					rlen = 9;
				}
			}
			else if(char(*buf)=='8'&&char(*(buf+1))=='8')
			{
				memcpy(rbuf, CMD, 7);
				*(rbuf+2) = CLIENT_ID;
				*(rbuf+5) = 0xFB;
				*(rbuf+6) = 0;
				rlen = 7;
				delay(100);
				this->println("Bye!");
				if(wifiClients[debugClientIdx])
				{
					wifiClients[debugClientIdx].stop();
				}
				debugClientIdx = 0xFF;
			}
			else if(char(*buf)=='u'&&char(*(buf+1))=='d'&&char(*(buf+2))=='p')
			{
				for(uint8_t i=0;i<CLIENT_POOL_SIZE;i++)
				{
					char info[48];
					sprintf(info,"Miko-%d, %d, [%d.%d.%d.%d:%d]\n",pool_client_ip[i][0],pool_client_ip[i][1],pool_client_ip[i][2],pool_client_ip[i][3],pool_client_ip[i][4],pool_client_ip[i][5],pool_port[i]);
			  		this->write(info);
				}
				rlen = 1;
			}
			else if(char(*buf)=='o'&&char(*(buf+1))=='p'&&char(*(buf+2))=='t')
			{
				char* space = strchr((char*)(buf+4), char(0x20));
				if (space != 0)
				{
					*space = 0;
					uint8_t relay = (uint8_t) atoi((char*)(buf+4));
					uint8_t state = (uint8_t) atoi((char*)(space+1));
					//{0xFE,0x7C,0xFF,0xDB,0x01,0x02,0xFC,0,0}
					memcpy(rbuf, CMD, 9);
					*(rbuf+2) = CLIENT_ID;
					*(rbuf+5) = 0x02;
					*(rbuf+6) = relay;
					*(rbuf+7) = state;
					*(rbuf+8) = 0;
					rlen = 9;
				}
			}
		}
	}
	return rlen;
}
//打印定时列表
void ZESP8266::printTimingList(uint8_t* timeList, uint8_t len)
{
	if (checkWifiClient(debugClientIdx))
    {
		char info[72];
		uint8_t k = 0;
		char sep;
		this->println("\ntiming list:");
		for (uint8_t i = 0; i < len; i++)
		{
			uint8_t val = *(timeList+i);
			if (val == 255) val = 0;
			uint8_t j = i % 5;
			if (j == 4) val = val & 0x7F;
			else if (j % 2 == 0) val = val % 24;
			else val = val % 60;
			if(j==1||j==3) sep = ':';
			else if(j==2) sep = '-';
			else if(j==4) sep = ',';
			if(i>0)
			{
			  if(j==0)
			  {
				//this->println(";");
				info[k++] = ';';
				info[k++] = 0x0D;
				info[k++] = 0x0A;
			  }
			  else info[k++] = sep;//this->print(sep);
			}

		  	char sval[2];
		  	sprintf(sval, j==4?"%02X":"%02d", val);
		  	info[k++] = sval[0];
		  	info[k++] = sval[1];
		  	if(k>=64)
		  	{
		  		info[k] = 0;
		  		this->write(info);
		  		k = 0;
		  	}
		}
		if(k>0)
		{
			info[k++] = 0x0D;
			info[k++] = 0x0A;
			info[k] = 0;
			this->write(info);
		}
	}
}
ZESP8266 ZWifi;