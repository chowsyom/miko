#include "ZHCRS501.h"

//unsigned long hcsr501_startTime = 0 - 5 - HCSR501_FQ_DELAY;

static unsigned long ACTIVE_TIMEOUT = 30000;   //毫秒
static bool hcrs501_sensor_state[HCSR501_NUMBERS];
static unsigned long deactive_startTime[HCSR501_NUMBERS];
static unsigned long deactive_endTime[HCSR501_NUMBERS];
static unsigned long active_startTime[HCSR501_NUMBERS];

//uint8_t i;
//for(i=0;i<HCSR501_NUMBERS;i++)
//{
//	deactive_startTime[i] = -1 - ACTIVE_TIMEOUT;
//	deactive_endTime[i] = 0;
//	active_startTime[i] = 0;
//}

extern "C"
{

	bool hcrs501_isFired(uint8_t sensorId, uint8_t HCSR501_PIN)
	{
		//bool hcsr501_flag;
		//if (millis() - hcsr501_startTime > HCSR501_FQ_DELAY) {
		bool val;
		//读取HC-RS501释热传感器状态
		val = digitalRead(HCSR501_PIN);
		//Messenger.debugInfo("HCSR", val?"1":"0");
		return val;
	}
	bool hcrs501_continue_active(uint8_t id, bool isFired)
	{
		bool isContinueActive = false;
		if (hcrs501_sensor_state[id] != isFired)
		{
			hcrs501_sensor_state[id] = isFired;
			if (hcrs501_sensor_state[id])
			{
				//新的激活开始，记录激活开始时间，非激活状态结束时间
				deactive_endTime[id] = millis();
				active_startTime[id] = millis();
			}
			else
			{
				//激活结束，记录非激活开始时间，消失非激活结束和激活开始状态
				deactive_startTime[id] = millis();
				deactive_endTime[id] = 0;
				active_startTime[id] = 0;
			}
		}
		//以下条件有一个成立则认为持续活动：
		//在非激活状态下，非激活持续时间小于ACTIVE_TIMEOUT
		//在激活状态下，激活持续时间大于ACTIVE_TIMEOUT
		if ((deactive_endTime[id] > 0 && (deactive_endTime[id] - deactive_startTime[id] < ACTIVE_TIMEOUT)) || (active_startTime[id] > 0 && (millis() - active_startTime[id] > ACTIVE_TIMEOUT)))
		{
			isContinueActive = true;
			deactive_startTime[id] = 0;
			deactive_endTime[id] = 0;
			active_startTime[id] = 0;
		}
		return isContinueActive;
	}
}