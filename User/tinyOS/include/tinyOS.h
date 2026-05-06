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
#include "fconfig.h"
#include "tEvent.h"
#include "tTask.h"
#include "tSem.h"
#include "tMbox.h"
#include  "tMemBlock.h"



/* Global Variables (暴露给系统的全局变量) ---------------------------------------*/
extern tTask* currentTask;
extern tTask* nextTask;

extern tList  taskTable[TINYOS_PRO_COUNT];     // 不同优先级链表
extern tBitmap taskPrioBitmap;    
extern tList tTaskDelayList;                  // 优先级位图

/* ========================================================================== */
/*                           User API (应用层接口)                            */
/* ========================================================================== */

/**
 * @brief 系统整体初始化 (包含核心组件与Idle任务的创建)
 */
void tinyOS_Init(void);


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

/**
 * @brief 将任务插入对用优先级列表
 */
void tTaskSchedInsert(tTask* task);


/**
 * @brief 将任务从对应优先级列表移除
 */
void tTaskSchedRemove(tTask* task);

/**
 * @brief 将任务插入
 */
void tTaskDelayListInsert(tTask* task, uint32_t delay);

/**
 * @brief 将任务从延时队列中移除
 */
void tTaskDelayListRemove(tTask* task);

void appInit(void);


/* 从延时队列中移除接口 */
void tTaskDelayRemove(tTask* task);
/* 任务从优先级队列中移除 */
void tTaskScheRe(tTask* task);

#endif /* __TINYOS_H */
