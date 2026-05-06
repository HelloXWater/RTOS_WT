#include "tinyOS.h"

void tEventInit(tEventControlBlock *ECB, tEventType eventtype)	//The Event Control Block initial function
{
	ECB->type=eventtype;
	tListInit(&(ECB->waitList));
}

void tEventWait(tEventControlBlock *ECB, tTask *task, 
				void *msg, uint32_t state, uint32_t timeout)	//The function of waiting event
{
	uint32_t status=tTaskEnterCritical();
	
	task->waitEvent=ECB;
	task->eventMsg=msg;
	task->state |= state;
	task->waitEventResult=tErrorNone;
	
	tTaskSchedRemove(task);
	tListAddLast(&(ECB->waitList),&(task->linkNode));			//Insert the task into the wautList of ECB
	
	if(timeout!=0)
	{
		tTaskDelayListInsert(task, timeout);
	}
	
	tTaskExitCritical(status);
}

tTask * tEventWakeUp(tEventControlBlock *ECB, 
					 void *msg, uint32_t result)			//The function of waking the ECB up
{
	tNode *node=(tNode *)0;
	tTask *task=(tTask *)0;
	
	uint32_t status=tTaskEnterCritical();
	
	if((node=tListRemoveFirst(&(ECB->waitList))) != (tNode*)0)
	{
		task=tNodeParent(node, tTask, linkNode);
		task->waitEvent=(tEventControlBlock *)0;
		task->eventMsg=msg;
		task->state &= ~TINYOS_TASK_WAIT_MASK;
		task->waitEventResult=result;
	
        //如果当前任务有延时 那么就强行将其从延时队列移除
		if(task->delayTicks!=0)
		{
			tTaskDelayListRemove(task);
		}
		
		tTaskSchedInsert(task);
	}
	
	tTaskExitCritical(status);
	return task;
}

void tEventRemoveTask(tTask *task, void *msg, uint32_t result)		//The function of remove the task from the waitList of its' ECB
{
	uint32_t status=tTaskEnterCritical();
	
	tListRemoveNode(&(task->waitEvent->waitList), &(task->linkNode));
	
	task->waitEvent=(tEventControlBlock *)0;
	task->eventMsg=msg;
	task->state &= ~TINYOS_TASK_WAIT_MASK;
	task->waitEventResult=result;
	
	tTaskExitCritical(status);
}

//事件控制块的清除函数，清除事件控制块中的所有等待任务
uint32_t tEventRemoveAll(tEventControlBlock *ECB, 
					   void *msg, uint32_t result)				//The function of removing all tasks in the waitList of ECB
{
	tNode *node=(tNode *)0;
	tTask *task=(tTask *)0;
	uint32_t count;
	
	uint32_t status=tTaskEnterCritical();
	
	count=tListCount(&(ECB->waitList));
	
	while((node=tListRemoveFirst(&(ECB->waitList))) != (tNode*)0)
	{
		task=tNodeParent(node, tTask, linkNode);
		task->waitEvent=(tEventControlBlock *)0;
		task->eventMsg=msg;
		task->state &= ~TINYOS_TASK_WAIT_MASK;
		task->waitEventResult=result;
	
		if(task->delayTicks!=0)
		{
			tTaskDelayListRemove(task);
		}
		
		tTaskSchedInsert(task);
	}
	
	tTaskExitCritical(status);
	return count;
}
//获取事件控制块中等待任务的数量
uint32_t tEventWaitCount(tEventControlBlock *ECB)					//The function of returning the number of tasks in the waitList of ECB
{
	uint32_t count;
	
	uint32_t status=tTaskEnterCritical();
	
	count=tListCount(&(ECB->waitList));
	
	tTaskExitCritical(status);
	return count;
}
