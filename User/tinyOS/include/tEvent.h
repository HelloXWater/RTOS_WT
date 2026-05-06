#ifndef TEVENT_H
#define TEVENT_H

#include "tlib.h"
#include "tTask.h"

typedef enum _tEventType								//Type of event
{
	tEventTypeUnknown,
	tEventTypeTimeout,
    tEventTypeSem,//信号量事件
	tEventTypeMbox,//邮箱事件
	tEventTypeMemBlock,//内存块事件	
}tEventType;

typedef struct _tEventControlBlock						//The structure of Event Control Block(ECB)
{
	tEventType type;
	tList waitList;
}tEventControlBlock;

void tEventInit(tEventControlBlock *ECB, tEventType eventtype);	//The Event Control Block initial function
void tEventWait(tEventControlBlock *ECB, tTask *task, 
				void *msg, uint32_t state, uint32_t timeout);	//The function of waiting event
tTask * tEventWakeUp(tEventControlBlock *ECB, 
					 void *msg, uint32_t result);				//The function of waking the ECB up
void tEventRemoveTask(tTask *task, void *msg, uint32_t result);//The function of remove the task from the waitList of its' ECB

//事件控制块的清除函数，清除事件控制块中的所有等待任务
uint32_t tEventRemoveAll(tEventControlBlock *ECB, 
					   void *msg, uint32_t result);				//The function of removing all tasks in the waitList of ECB
//获取事件控制块中等待任务的数量
uint32_t tEventWaitCount(tEventControlBlock *ECB);
#endif
