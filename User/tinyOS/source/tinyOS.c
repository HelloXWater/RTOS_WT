/**
  ******************************************************************************
  * @file    tinyOS.c
  * @author  wt
  * @brief   tinyOS 内核核心实现
  ******************************************************************************
  */

#include "tinyOS.h"
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
tTask* taskTable[TINYOS_PRO_COUNT];     // 就绪表数组

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
 * @brief 将任务插入就绪列表
 */
static void tTaskSchedInsert(tTask* task)
{
    taskTable[task->prio] = task;
    tBitmapSet(&taskPrioBitmap, task->prio);
}

/**
 * @brief 将任务从就绪列表移除
 */
static void tTaskSchedRemove(tTask* task)
{
    taskTable[task->prio] = (tTask*)0;
    tBitmapClear(&taskPrioBitmap, task->prio);
}

/**
 * @brief 将任务插入延时队列
 */
static void tTaskDelayListInsert(tTask* task, uint32_t delay)
{
    task->delayTicks = delay * 0.1;
    tListAddLast(&tTaskDelayList, &task->delayNode);
    task->state |= TINYOS_TASK_DELAY;
}

/**
 * @brief 将任务从延时队列中移除
 */
static void tTaskDelayListRemove(tTask* task)
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

    task->stack = stack;
    task->delayTicks = 0;
    task->prio = prio;
    task->state = TINYOS_TASK_READY;

    // 初始化延时节点
    tNodeInit(&task->delayNode);

    // 默认创建任务后直接插入就绪表
    tTaskSchedInsert(task);
}

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
    // 初始化调度器所需的基础组件
    scheduleLockCount = 0;
    tBitmapInit(&taskPrioBitmap);
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
    return taskTable[highestReadyPrio];
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
 * @brief 任务阻塞延时函数
 */
void tTaskDelay(uint32_t delay)
{
    uint32_t status = tTaskEnterCritical();
    
    // 1. 挂入延时队列并设置滴答数
    tTaskDelayListInsert(currentTask, delay);
    // 2. 从就绪列表中移除，放弃 CPU 运行权
    tTaskSchedRemove(currentTask);
    
    tTaskExitCritical(status);
    
    // 3. 立即触发一次调度，让出 CPU
    tTaskShed();
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
            // 延时结束，从延时队列移除
            tTaskDelayListRemove(task);
            // 重新挂载回就绪表
            tTaskSchedInsert(task);
        }
    }
    
    tTaskExitCritical(status);
    
    // 每次时钟节拍都检查一次是否有更高优先级的任务就绪
    tTaskShed();
}
