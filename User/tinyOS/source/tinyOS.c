/**
  ******************************************************************************
  * @file    tinyOS.c
  * @author  wt
  * @brief   tinyOS 内核核心实现
  ******************************************************************************
  */

#include "tinyOS.h"
#include "fconfig.h"
#include "stm32f4xx.h"
#include "tlib.h"
#include <iso646.h>
#include <stdint.h>

/* Defines ------------------------------------------------------------------*/
#define NVIC_INT_CTRL    0xE000ED04
#define NVIC_PENDSVSET   0x10000000
#define NVIC_SYSPRI2     0xE000ED22
#define NVIC_PENDSV_PRI  0x000000FF

#define MEM32(addr)  *(volatile unsigned long*)(addr)
#define MEM8(addr)   *(volatile unsigned char*)(addr)

/* Global Variables ---------------------------------------------------------*/
/* 任务控制块指针 */
tTask* currentTask;
tTask* nextTask;

/* 调度与就绪表 */
tBitmap taskPrioBitmap;                 // 优先级位图
tList  taskTable[TINYOS_PRO_COUNT];     // 不同优先级链表

/* 系统内部组件 */
uint8_t scheduleLockCount;              // 调度锁计数器
tList tTaskDelayList;                   // 延时队列

/* 空闲任务 (隐藏在内核中，不对外暴露) */
static tTask tTaskIdle;
static tTaskStack idleTaskEnv[1024];

/* Private Functions --------------------------------------------------------*/
/**
 * @brief 系统空闲任务入口函数
 *        绝对不能调用任何可能导致阻塞或挂起的 API (如 tTaskDelay)
 */
uint32_t  u32TaskIdleFlag = 0;
static void IdleTaskEntry(void* param)
{
    for(;;)
    {
        // 纯死循环，保证系统永远有任务可执行
        // 未来可以考虑加入 __WFI() 指令以降低功耗
        u32TaskIdleFlag++;
    }
}

/**
 * @brief 将任务插入对用优先级列表
 */
void tTaskSchedInsert(tTask* task)
{
  tListAddFirst(&(taskTable[task->prio]), &(task->linkNode));
  tBitmapSet(&taskPrioBitmap, task->prio);
}

/**
 * @brief 将任务从对应优先级列表移除
 */
void tTaskSchedRemove(tTask* task)
{
    tListRemoveNode(&(taskTable[task->prio]), &(task->linkNode));
    //移除后如果该优先级列表下没有任务了，清除位图对应位
    if(tListCount(&(taskTable[task->prio])) == 0)
    {
        tBitmapClear(&taskPrioBitmap, task->prio);
    }
}

/**
 * @brief 将任务插入延时队列
 */
void tTaskDelayListInsert(tTask* task, uint32_t delay)
{
    task->delayTicks = delay * 0.1;
    tListAddLast(&tTaskDelayList, &task->delayNode);
    task->state |= TINYOS_TASK_DELAY;
}

/**
 * @brief 将任务从延时队列中移除
 */
void tTaskDelayListRemove(tTask* task)
{
    tListRemoveNode(&tTaskDelayList, &(task->delayNode));
    task->state &= ~TINYOS_TASK_DELAY;
}

/* Critical Section API -----------------------------------------------------*/
uint32_t tTaskEnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void tTaskExitCritical(uint32_t status)
{
    __set_PRIMASK(status);
}

/* Scheduler Lock API -------------------------------------------------------*/
void tTaskLockInit(void)
{
    scheduleLockCount = 0;
}

void tTaskLock(void)
{
    uint32_t status = tTaskEnterCritical();
    if(scheduleLockCount < 255)
    {
        scheduleLockCount++;
    }
    tTaskExitCritical(status);
}

void tTaskUnlock(void)
{
    uint32_t status = tTaskEnterCritical();
    if(scheduleLockCount > 0)
    {
        if(--scheduleLockCount == 0)
        {
            tTaskShed();
        }
    }
    tTaskExitCritical(status);
}

/* Core Kernel API ----------------------------------------------------------*/

/**
 * @brief 延时队列初始化
 */
void tTaskDelayListInit(void)
{
    tListInit(&tTaskDelayList);
}

/**
 * @brief tinyOS 系统级初始化 (提供给 main 调用)
 */
void tinyOS_Init(void)
{
    // 初始化调度器所需的基础组件(位图等)
    scheduleLockCount = 0;
    tBitmapInit(&taskPrioBitmap);

    // 初始化每个优先级的就绪链表
    for(uint32_t i = 0; i < TINYOS_PRO_COUNT; i++)
    {
        tListInit(&taskTable[i]);
    }

    tTaskDelayListInit();

    // 由系统自动创建底层托底任务 (Idle Task)
    tTaskInit(&tTaskIdle, IdleTaskEntry, (void *)0, TINYOS_PRO_COUNT - 1, &idleTaskEnv[1024]);
}

/**
 * @brief 查找就绪表中优先级最高的任务
 */
tTask* tTaskHighestReady(void)
{
    uint32_t highestReadyPrio = tBitmapGetFirstSet(&taskPrioBitmap);
    //取该优先级下对应队列的第一个任务
    // tNode* node = tListFirst(&taskTable[highestReadyPrio]);
    // return tNodeParent(node, tTask, linkNode);
    return tNodeParent(tListFirst(&taskTable[highestReadyPrio]), tTask, linkNode);
}

/**
 * @brief 触发底层上下文切换 (PendSV)
 */
void tTaskSwitch(void)
{
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET; // 设置 PendSV 悬起标志触发中断
}

/**
 * @brief 启动第一个任务
 */
void tTaskRunFirst(void)
{
    __set_PSP(0); // 初始化设置 PSP 为 0，PendSV_Handler 里可通过判断它决定是否保存旧上下文
    MEM8(NVIC_SYSPRI2) = NVIC_PENDSV_PRI; // 设置 PendSV 为最低优先级
    MEM32(NVIC_INT_CTRL) = NVIC_PENDSVSET;// 悬起 PendSV 开始第一次调度
}   

/**
 * @brief 核心调度器逻辑
 */
void tTaskShed(void)
{
    tTask* tempTask;
    uint32_t status = tTaskEnterCritical();

    // 如果调度器被上锁，直接退出不进行调度
    if(scheduleLockCount > 0)
    {
        tTaskExitCritical(status);
        return;
    }

    tempTask = tTaskHighestReady();
    if(tempTask != currentTask)
    {
        nextTask = tempTask;
        tTaskSwitch();
    }
    
    // 注意：你原来的代码这里多写了一次 tTaskSwitch()，我已帮你删除
    // 因为如果没有发生更高优先级抢占 (tempTask == currentTask)，不需要也不应该触发 PendSV
    tTaskExitCritical(status);
}



/**
 * @brief 系统Tick节拍处理 (由SysTick_Handler周期调用)
 */
void tTaskSystemTickHandler(void)
{
    tNode* node;
    uint32_t status = tTaskEnterCritical();
    
    // 扫描延时双向链表，检查是否有任务倒计时归零
    for(node = tTaskDelayList.headNode.nextNode; node != &(tTaskDelayList.headNode); node = node->nextNode)
    {
        // 根据节点倒推回 Task 控制块的基地址
        tTask* task = tNodeParent(node, tTask, delayNode);
        if(--task->delayTicks == 0)
        {
            //如果任务有等待事件
            if(task->waitEvent)
            {
                //从事件的等待队列中移除
                tEventRemoveTask(task, (void*)0, tErrorTimeout);
            }
            // 延时结束，从延时队列移除
            tTaskDelayListRemove(task);
            // 重新挂载回对应优先级列表
            tTaskSchedInsert(task);
        }
    }

    //判断当前任务的时间片有没有用完
    if(--currentTask->timeSlice == 0)
    {
      if(tListCount(&taskTable[currentTask->prio]) > 0)
      {
        tListRemoveFirst(&taskTable[currentTask->prio]);
        tListAddLast(&taskTable[currentTask->prio], &(currentTask->linkNode));

        currentTask->timeSlice = TINYOS_SLICE_MAX; //重置时间片
      }
        
    }
    
    tTaskExitCritical(status);
    
    // 每次时钟节拍都检查一次是否有更高优先级的任务就绪
    tTaskShed();
}
