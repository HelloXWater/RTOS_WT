#include "tinyOS.h"
#include <stdint.h>
#include "tlib.h"
/**
 * @brief 任务初始化
 */
void tTaskInit(tTask * task, void(*entry)(void *), void* param, uint32_t prio, tTaskStack* stack)
{
    /* 以下为硬件进入/退出 PnedSV 自动保存的寄存器 */
    *(--stack) = (unsigned long)(1 << 24); /* xPSR 的 T 位必须置 1，否则会发生 UsageFault (进入 ARM 模式) */
    *(--stack) = (unsigned long)entry;     /* PC 任务入口地址 */
    *(--stack) = (unsigned long)0x14;      /* LR (返回地址通常是一个错误捕获函数的地址，这里先用魔术数字) */
    *(--stack) = (unsigned long)0x12;      /* R12 */
    *(--stack) = (unsigned long)0x3;       /* R3 */
    *(--stack) = (unsigned long)0x2;       /* R2 */
    *(--stack) = (unsigned long)0x1;       /* R1 */
    *(--stack) = (unsigned long)param;     /* R0 程序的入口参数 */

    /* 其他由软件在 PendSV_Handler 中手动保存的寄存器 (R11 - R4) */
    *(--stack) = (unsigned long)0x11;
    *(--stack) = (unsigned long)0x10;
    *(--stack) = (unsigned long)0x9;
    *(--stack) = (unsigned long)0x8;
    *(--stack) = (unsigned long)0x7;
    *(--stack) = (unsigned long)0x6;
    *(--stack) = (unsigned long)0x5;
    *(--stack) = (unsigned long)0x4;

    task->timeSlice = TINYOS_SLICE_MAX;
    task->stack = stack;
    task->delayTicks = 0;
    task->prio = prio;
    task->state = TINYOS_TASK_READY;
    task->suspendCount = 0;
    task->clear = (void (*)(void*))0;
    task->cleanparam = (void*)0;
    task->requestDeleteFlag = 0;

    // 初始化延时节点
    tNodeInit(&task->delayNode);

    tNodeInit(&task->linkNode); // 初始化优先级链表节点
    //这里是根据任务的优先级 加入到对应的优先级列表中
    // tListAddFirst(&taskTable[task->prio], &task->linkNode);
    tTaskSchedInsert(task);
}

/* 任务挂起函数 */
void tTaskSuspend(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    //只有任务不是延时状态才进行挂起
     if(!(task->state & TINYOS_TASK_DELAY))
    {
        if(++task->suspendCount <= 1) 
        {
            task->state |= TINYOS_TASK_SUSPEND;
            tTaskSchedRemove(task);
            //如果当前任务是当前运行的任务
            if(task == currentTask)
            {
                tTaskShed();
            }
        }
    }
    tTaskExitCritical(status);
}

/* 任务恢复函数 */
void tTaskWakeup(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    if(task->state & TINYOS_TASK_SUSPEND)
    {
        if(--task->suspendCount == 0)
        {
            task->state &= ~TINYOS_TASK_SUSPEND;
            tTaskSchedInsert(task);
            tTaskShed();
        }
    }
    tTaskExitCritical(status);
}

/* 任务从优先级队列中移除 */
void tTaskScheRe(tTask* task)
{
    tListRemoveNode(&taskTable[task->prio], &task->linkNode);
    if(tListCount(&taskTable[task->prio]) == 0)
    {
        tBitmapClear(&taskPrioBitmap, task->prio);
    }
}

/* 从延时队列中移除接口 */
 void tTaskDelayRemove(tTask* task)
{
    tListRemoveNode(&tTaskDelayList, &task->delayNode);
}

/* 强制删除接口清理回调函数 */
void tTaskSetCleanCallFunc(tTask* task, void (*clean)(void*), void* param)
{
    uint32_t status = tTaskEnterCritical();
    task->clear = clean;
    task->cleanparam = param;
    tTaskExitCritical(status);
}

/* 强制删除函数 */
void tTaskForceDelete(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    if(task->state == TINYOS_TASK_DELAY)
    {
        tTaskDelayRemove(task);
    }
    else if(!(task->state & TINYOS_TASK_SUSPEND))
    {
        tTaskSchedRemove(task);
    }
    if(task->clear)
    {
        task->clear(task->cleanparam);
    }
    if(currentTask == task)
    {
        tTaskShed();
    }
    tTaskExitCritical(status);
}
/* 请求删除接口 */
void tTaskRequestDelete(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    task->requestDeleteFlag = 1;
    tTaskExitCritical(status);
}

/* 检查请求删除标志 */
uint8_t tTaskIsRequestedDelete(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    uint8_t flag = task->requestDeleteFlag;
    tTaskExitCritical(status);
    return flag;
}

/* 自身删除函数 */
void tTaskSelfDelete(tTask* task)
{
    uint32_t status = tTaskEnterCritical();
    //首先从就绪列表中移除
    tTaskSchedRemove(currentTask);
    if(currentTask->clear)
    {
        currentTask->clear(currentTask->cleanparam);
    }
    tTaskShed();
    tTaskRequestDelete(task);
}

void tTaskGetInfo(tTask* task, tTaskInfo* info)
{
    uint32_t status = tTaskEnterCritical();
    info->prio = task->prio;
    info->delayTicks = task->delayTicks;
    info->state = task->state;
    info->timeSlice = task->timeSlice;
    info->suspendCount = task->suspendCount;
    tTaskExitCritical(status);
}

