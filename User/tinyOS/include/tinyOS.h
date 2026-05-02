// #ifndef TINYOS_H
// #define TINYOS_H

// #include <stdint.h>
// #include "tlib.h"
// #include "fconfig.h"


// #define TINYOS_TASK_READY 0
// #define TINYOS_TASK_DELAY (1 << 1)

// typedef uint32_t tTaskStack ;

// typedef struct _tTask{
//     tTaskStack* stack;
//     uint32_t delayTicks;  
//     tNode delayNode; //添加延时链表节点
//     uint32_t prio; //添加优先级字段  
//     uint32_t state; //添加任务状态字段
// }tTask;

// extern tTask * currentTask;
// extern tTask * nextTask;

// void tTaskInit(tTask * task, void(*entry)(void *), void* param, uint32_t prio, tTaskStack* stack);


// uint32_t tTaskEnterCritical(void);
// void tTaskExitCritical(uint32_t status);

// void tTaskRunFirst(void);
// void tTaskSwitch(void);

// void tTaskSchedInit(void);
// void tTaskShedDisable(void);
// void tTaskShedEnable(void);
// void tTaskShed(void);

// tTask* tTaskHighestReady(void);


// #endif
/**
  ******************************************************************************
  * @file    tinyOS.h
  * @author  wt
  * @brief   tinyOS 操作系统核心头文件
  ******************************************************************************
  */
#ifndef __TINYOS_H
#define __TINYOS_H

#include <stdint.h>
#include "tlib.h"
#include "fconfig.h"

/* Task States (任务状态宏) ----------------------------------------------------*/
#define TINYOS_TASK_READY   0
#define TINYOS_TASK_DELAY   (1 << 1)

/* Type Definitions (类型定义) -------------------------------------------------*/
typedef uint32_t tTaskStack;

typedef struct _tTask {
    tTaskStack* stack;       // 任务栈顶指针 (注：必须是结构体第一个元素，底层汇编依赖此偏移量)
    uint32_t delayTicks;     // 延时节拍数
    tNode delayNode;         // 延时链表节点
    uint32_t prio;           // 任务优先级
    uint32_t state;          // 任务当前状态
} tTask;

/* Global Variables (暴露给系统的全局变量) ---------------------------------------*/
extern tTask* currentTask;
extern tTask* nextTask;


/* ========================================================================== */
/*                           User API (应用层接口)                            */
/* ========================================================================== */

/**
 * @brief 系统整体初始化 (包含核心组件与Idle任务的创建)
 */
void tinyOS_Init(void);

/**
 * @brief 创建并初始化任务
 */
void tTaskInit(tTask* task, void(*entry)(void*), void* param, uint32_t prio, tTaskStack* stack);

/**
 * @brief 任务阻塞延时
 * @param delay 延时的系统滴答(Tick)数
 */
void tTaskDelay(uint32_t delay);

/**
 * @brief 调度器上锁与解锁
 */
void tTaskLock(void);
void tTaskUnlock(void);


/* ========================================================================== */
/*                         Kernel API (内核层与中断接口)                      */
/* ========================================================================== */

/**
 * @brief 临界区管理 (进出中断屏蔽)
 */
uint32_t tTaskEnterCritical(void);
void tTaskExitCritical(uint32_t status);

/**
 * @brief 系统启动与底层调度
 */
void tTaskRunFirst(void);
void tTaskSwitch(void);
void tTaskShed(void);

/**
 * @brief 查找当前最高优先级就绪任务
 */
tTask* tTaskHighestReady(void);

/**
 * @brief 核心心跳处理 (必须在 SysTick_Handler 中调用)
 */
void tTaskSystemTickHandler(void);


#endif /* __TINYOS_H */
