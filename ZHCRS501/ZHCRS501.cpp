#include "ZHCRS501.h"

//unsigned long hcsr501_startTime = 0 - 5 - HCSR501_FQ_DELAY;

static unsigned long ACTIVE_TIMEOUT = 30000;   //����
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
		//��ȡHC-RS501���ȴ�����״̬
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
				//�µļ��ʼ����¼���ʼʱ�䣬�Ǽ���״̬����ʱ��
				deactive_endTime[id] = millis();
				active_startTime[id] = millis();
			}
			else
			{
				//�����������¼�Ǽ��ʼʱ�䣬��ʧ�Ǽ�������ͼ��ʼ״̬
				deactive_startTime[id] = millis();
				deactive_endTime[id] = 0;
				active_startTime[id] = 0;
			}
		}
		//����������һ����������Ϊ�������
		//�ڷǼ���״̬�£��Ǽ������ʱ��С��ACTIVE_TIMEOUT
		//�ڼ���״̬�£��������ʱ�����ACTIVE_TIMEOUT
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