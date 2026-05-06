#include "tinyOS.h"

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
