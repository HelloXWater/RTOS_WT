#ifndef T_TASK_H
#define T_TASK_H

#include "stdint.h"
#include "tLib.h"
/* Task States (任务状态宏) ----------------------------------------------------*/
#define TINYOS_TASK_READY   0
#define TINYOS_TASK_STATE_DESTORTED (1 << 1) // 任务已销毁
#define TINYOS_TASK_DELAY   (1 << 2)
#define TINYOS_TASK_SUSPEND (1 << 3)

//Task 中state字段的等待事件掩码 (高 16 位)
#define TINYOS_TASK_WAIT_MASK  (0xFF << 16)

struct _tEventControlBlock; // 前向引用的方式，避免循环依赖

/* Type Definitions (类型定义) -------------------------------------------------*/
typedef uint32_t tTaskStack;

//等待事件错误码
typedef enum _tError {
    tErrorNone = 0,
    tErrorTimeout,//超时
    tErrorInvalidParam,
    tErrorResourceUnavailable,//资源不可用
    tErrorDel,
    tErrorResourceFull,//资源满了
} tError;

typedef struct _tTask {
    tTaskStack* stack;       // 任务栈顶指针 (注：必须是结构体第一个元素，底层汇编依赖此偏移量)
    tNode linkNode;          // 用于优先级队列的任务节点(存放在哪个优先级)
    uint32_t delayTicks;     // 延时节拍数
    tNode delayNode;         // 延时链表节点
    uint32_t prio;           // 任务优先级
    uint32_t state;          // 任务当前状态
    uint32_t timeSlice;       // 时间片剩余滴答数
    uint32_t suspendCount;     // 任务被挂起的次数

    //删除相关
    void (*clear) (void* param);
    void* cleanparam;
    uint8_t requestDeleteFlag;

    //事件相关字段
    struct _tEventControlBlock* waitEvent;
    void* eventMsg;//等待事件数据存放的位置
    uint32_t waitEventResult;//等待事件结果

} tTask;

typedef struct _tTaskInfo{
    uint32_t prio;           // 任务优先级
    uint32_t delayTicks;     // 延时节拍数
    uint32_t state;          // 任务当前状态
    uint32_t timeSlice;       // 时间片剩余滴答数
    uint32_t suspendCount;     // 任务被挂起的次数
}tTaskInfo;

/**
 * @brief 创建并初始化任务
 */
void tTaskInit(tTask* task, void(*entry)(void*), void* param, uint32_t prio, tTaskStack* stack);

/* 任务恢复函数 */
void tTaskWakeup(tTask* task);
/* 任务挂起函数 */
void tTaskSuspend(tTask* task);

/* 强制删除接口清理回调函数 */
void tTaskSetCleanCallFunc(tTask* task, void (*clean)(void*), void* param);
/* 强制删除函数 */
void tTaskForceDelete(tTask* task);
/* 请求删除接口 */
void tTaskRequestDelete(tTask* task);
/* 检查请求删除标志 */
uint8_t tTaskIsRequestedDelete(tTask* task);
/* 自身删除函数 */
void tTaskSelfDelete(tTask* task);

/* 任务状态查询的接口 */
void tTaskGetInfo(tTask* task, tTaskInfo* info);

#endif
