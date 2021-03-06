#include "ZTiming.h"


void ZTimingClass::begin(void)
{
	this->updateTimingListFromRom(TIMELIST_ROM_ADDR);
	//this->printTimingList();
};
void ZTimingClass::loop(void)
{
	timingMsg[0] = 0x04;
	if(Messenger.listen(timingMsg))
	{
		this->updateTimingList(timingMsg+1);
	}
}
//检查定时开关
boolean ZTimingClass::checkTimingList(uint8_t addr1, uint8_t addr2, uint8_t* date)
{
	OptByCmd defOptByCmdFlag;
	return this->checkTimingList(addr1,addr2,&defOptByCmdFlag,date);
}
boolean ZTimingClass::checkTimingList(uint8_t addr1, uint8_t addr2, OptByCmd* optByCmd, uint8_t* date) 
{
	bool isOn = false;
	bool isTiming = false;
// Serial.print("20");
// Serial.print(date[6]);
// Serial.print("-");
// Serial.print(date[5]);
// Serial.print("-");
// Serial.print(date[4]);
// Serial.print(" ");
// Serial.print(date[2]);
// Serial.print(":");
// Serial.print(date[1]);
// Serial.print(":");
// Serial.print(date[0]);
// Serial.print(", ");
// Serial.println(date[3],DEC);

//Serial.print("(*optByCmd).isOn=");
//Serial.println((*optByCmd).isOn);
//Serial.print(",(*optByCmd).enable=");
//Serial.println((*optByCmd).enable);

	for (uint8_t i = addr1; i < addr2; i++)
	{
		int startTime = timeList[i][0] * 60 + timeList[i][1];
		int endTime =  timeList[i][2] * 60 + timeList[i][3];
		int curTime = date[2] * 60 + date[1];
		if (date[3] < 1 || date[3] > 127) date[3] = 0x7F;
		uint8_t pos = 0;
		if (endTime < startTime) 
		{
			if (curTime < endTime)
			{
				startTime -= 24 * 60;
				pos = (7 + date[3] - 2) % 7;
			}
			else
			{
				endTime += 24 * 60;
				pos = (7 + date[3] - 1) % 7;
			}
		}
		else
		{
			pos = date[3] % 7;
		}
		uint8_t dow = 0x01 << pos;
		if ((timeList[i][4] & dow) && startTime <= curTime  && curTime < endTime)
		{
			isOn = true;
			isTiming = true;
			// Serial.print(timeList[i][0]);
			// Serial.print(":");
			// Serial.print(timeList[i][1]);
			// Serial.print("-");
			// Serial.print(timeList[i][2]);
			// Serial.print(":");
			// Serial.print(timeList[i][3]);
			// Serial.print(",");
			// Serial.print(timeList[i][4]);
			//Serial.println(" turned on by timing.");

			if ((*optByCmd).isOn)
			{
				(*optByCmd).enable = false;
			}
			else if ((*optByCmd).enable)
			{
				int offByCmdTime = (*optByCmd).time[0] * 60 + (*optByCmd).time[1];
				if (startTime <= offByCmdTime  && offByCmdTime < endTime)
				{
					isOn = false;
					//Serial.println("turned off by remote command.");
				}
			}
			break;
		}
	}
	if(!isTiming)
	{
		if (!(*optByCmd).isOn)
		{
			(*optByCmd).enable = false;
		}
		else if ((*optByCmd).enable) 
		{
			isOn = true;
		}
	}
	//Serial.print("isOn=");
	//Serial.println(isOn);
	return isOn;
}

////快速排序
//void quickSort(int *a, uint8_t i, uint8_t j)
//{
//  uint8_t m, n, temp;
//  uint8_t k;
//  m = i;
//  n = j;
//  k = a[(i + j) / 2]; /*选取的参照*/
//  do {
//    while (a[m] < k && m < j) m++; /* 从左到右找比k大的元素*/
//    while (a[n] > k && n > i) n--; /* 从右到左找比k小的元素*/
//    if (m <= n) { /*若找到且满足条件，则交换*/
//      temp = a[m];
//      a[m] = a[n];
//      a[n] = temp;
//      m++;
//      n--;
//    }
//  } while (m <= n);
//  if (m < j) quickSort(a, m, j); /*运用递归*/
//  if (n > i) quickSort(a, i, n);
//}
//从ROM更新时间列表
void ZTimingClass::updateTimingListFromRom(uint8_t romAddr) {

  if (EEPROM.read(romAddr) < 255) {
	uint8_t addr;
	for (addr = 0; addr < TIMELIST_NUMBER*5; addr++) {
	  uint8_t val = EEPROM.read(addr + romAddr);
	  if (val == 255) val = 0;
	  uint8_t i = addr / 5;
	  uint8_t j = addr % 5;
	  if (j == 4) val = val & 0x7F;
	  else if (j % 2 == 0) val = val % 24;
	  else val = val % 60;
	  timeList[i][j] = val;
	  //Serial.println("EEPROM[" + String(addr) + "]=" + val);
	}
  }
}
//获取时间列表串
void ZTimingClass::getTimingListFromRom(uint8_t romAddr, uint8_t* list, uint8_t* dataSize) {
	uint8_t addr;
	uint8_t len = TIMELIST_NUMBER*5;
	for (addr = 0; addr < len && addr < *dataSize; addr++) 
	{
	  uint8_t val = EEPROM.read(addr + romAddr);
	  if (val == 255) val = 0;
	  *(list+addr) = val;
	}
	*dataSize = addr;
}
// 设置操作命令的时间点
//void setOptCmdTime(uint8_t relayId, uint8_t hour, uint8_t minute, bool isOn) {
//	optByCmd[relayId].time[0] = hour;
//	optByCmd[relayId].time[1] = minute;
//	optByCmd[relayId].isOn = isOn;
//}
//把定时时间列表写入EEPROM
void ZTimingClass::writeTimingListToRom(uint8_t romAddr, uint8_t* time) {
	  uint8_t addr = romAddr;
	  while (*time != 0)
	  {
		EEPROM.write(addr, *time == 0xFF ? 0 : *time);
		time++;
		addr++;
	  }
	  #ifdef _ZESP8266_H_
		EEPROM.commit();
	  #endif
	  this->updateTimingListFromRom(romAddr);
}
void ZTimingClass::updateTimingList(uint8_t* addr)
{
	if(*addr != 0xFC)
	{
	  writeTimingListToRom(TIMELIST_ROM_ADDR, addr);
	}
	this->printTimingList();
}
void ZTimingClass::printTimingList(void)
{
	uint8_t timingDataSize = TIMELIST_NUMBER*5;
	uint8_t timingList[timingDataSize+1];
	this->getTimingListFromRom(TIMELIST_ROM_ADDR,timingList, &timingDataSize);
	timingList[timingDataSize+1] = 0;
	Messenger.printTimingList(timingList, timingDataSize);
}

ZTimingClass ZTiming;