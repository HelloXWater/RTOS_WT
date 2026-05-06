#include "tinyOS.h"
#include "stm32f4xx_hal.h"

/**
 * @brief 系统时钟重配置 (供任务内部修改测试使用)
 */
void SetSysytemClock(uint32_t ms)
{
    // 修正后的 SysTick 配置逻辑
    SysTick->LOAD = ms * (SystemCoreClock / 1000) - 1; 
    NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1); 
    SysTick->VAL = 0; 
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk; 
}

void SysTick_Handler(void)
{
    HAL_IncTick();               // 维持 HAL 库原有的时间基准
    tTaskSystemTickHandler();    // 驱动你的 tinyOS 的核心心跳和延时队列
}

