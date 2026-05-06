#include "tinyOS.h"

void tSemInit(tSem *sem, uint32_t startCount, uint32_t maxCount)	//The Counting Semaphores initial function
{
	tEventInit(&(sem->ECB), tEventTypeSem);
	
	sem->maxCount=maxCount;
	if(maxCount==0)
	{
		sem->Count=startCount;
	}
	else
	{
		sem->Count=(startCount>maxCount) ? maxCount:startCount;
	}
}
\
/* 信号量等待函数 */
uint32_t tSemWait(tSem *sem, uint32_t timeout)		//The Counting Semaphores wait function
{
	uint32_t status=tTaskEnterCritical();
	//如果信号量计数大于0，直接获取资源
	if(sem->Count>0)
	{
		sem->Count--;
		tTaskExitCritical(status);
		return tErrorNone;
	}
	//否则进入等待
	else
	{
		tEventWait(&(sem->ECB), currentTask, (void *)0, tEventTypeSem, timeout);
		tTaskExitCritical(status);
		
		tTaskShed();
		return currentTask->waitEventResult;
	}
}
/* 无等待获取信号量函数 */
uint32_t tSemNoWaitGet(tSem *sem)				//Get the status of the Counting Semaphores
{
	uint32_t status=tTaskEnterCritical();
	
	if(sem->Count>0)
	{
		sem->Count--;
		tTaskExitCritical(status);
		return tErrorNone;
	}
	else
	{
		tTaskExitCritical(status);
		return tErrorResourceUnavailable;
	}
}

/* 信号量通知函数 */
void tSemNotify(tSem *sem)						//The Counting Semaphores notify function
{
	tTask *task;
	uint32_t status=tTaskEnterCritical();
	//如果有任务等待，唤醒一个任务（这个时候因为有等待任务 所以不需要对这个信号量进行加加了）
	if(tEventWaitCount(&(sem->ECB))>0)
	{
		task=tEventWakeUp(&(sem->ECB), (void *)0, tErrorNone);
		if(task->prio<currentTask->prio)
		{
			tTaskShed();
		}
	}
	//如果没有任务等待，直接释放一个资源
	else
	{
		sem->Count++;
		if((sem->maxCount)!=0 && (sem->Count)>(sem->maxCount))
		{
			sem->Count=sem->maxCount;
		}
	}
	tTaskExitCritical(status);
}

void tSemGetInfo(tSem *sem, tSemInfo *info)			//The Counting Semaphores's getting information function
{
	uint32_t status=tTaskEnterCritical();
	
	info->Count=sem->Count;
	info->maxCount=sem->maxCount;
	info->taskCount=tEventWaitCount(&(sem->ECB));
	
	tTaskExitCritical(status);
}

//销毁信号量
uint32_t tSemDestroy(tSem *sem)		    			//The Counting Semaphores destroy function
{
	uint32_t status=tTaskEnterCritical();
	
	uint32_t RemovedCount=tEventRemoveAll(&(sem->ECB), (void *)0, tErrorDel);
	sem->Count=0;
	
	tTaskExitCritical(status);
	//如果有任务被移除，触发一次调度
	if(RemovedCount>0)
	{
		tTaskShed();
	}
	return RemovedCount;
}
