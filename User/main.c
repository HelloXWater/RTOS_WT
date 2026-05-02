// /************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/**
  ******************************************************************************
  * @file    main.c
  * @author  wt
  * @date    2026-04-06
  * @brief   RTOS 应用层入口 (业务逻辑)
  ******************************************************************************
  * @attention
  *
  * STM32 F429 开发板
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_hal.h"
#include "./led/bsp_led.h"
#include "tinyOS.h"

/* Global App Variables ------------------------------------------------------*/
/* 用户业务任务与堆栈定义 */
tTask tTask1;
tTask tTask2;
tTaskStack task1EntryEnv[1024];
tTaskStack task2EntryEnv[1024];

/* Private Function Prototypes -----------------------------------------------*/
static void SystemClock_Config(void);

/* Business Logic (Tasks) ----------------------------------------------------*/

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

/* 业务任务 1 */
int firstSet;
static volatile uint32_t u32Task1Flag = 0;

void task1Entry(void* param)
{
    // 测试代码：位图操作演示
    tBitmap bitmap;
    tBitmapInit(&bitmap);
    for(int i = tBitmapPosCount() - 1; i >= 0; i--)
    {
        tBitmapSet(&bitmap, i);
        firstSet = tBitmapGetFirstSet(&bitmap);
    }
    for(int i = 0; i < tBitmapPosCount(); i++)
    {
        tBitmapClear(&bitmap, i);
        firstSet = tBitmapGetFirstSet(&bitmap);
    }

    SetSysytemClock(1); // 初始化系统节拍(1ms)

    // 核心业务循环
    for(;;)
    {
        u32Task1Flag = 0;
        tTaskDelay(100);  // 让出 CPU 10 个 ticks

        u32Task1Flag = 1;
        tTaskDelay(100);
    }
}

/* 业务任务 2 */
static volatile uint32_t u32Task2Flag = 0;

void task2Entry(void* param)
{
    // 核心业务循环
    for(;;)
    {
        u32Task2Flag = 0;
        tTaskDelay(100);  // 让出 CPU 10 个 ticks

        u32Task2Flag = 1;
        tTaskDelay(100);
    }
}


/* Main Entry ----------------------------------------------------------------*/

/**
  * @brief  主函数
  */
int main(void)
{
    /* 1. 裸机硬件初始化 */
    SystemClock_Config(); // 配置主频为 180MHz (或216MHz)
    LED_GPIO_Config();    // LED 端口初始化

    /* 2. 操作系统内核初始化 */
    // 一键唤醒 OS 的延时队列、位图和底层空闲任务
    tinyOS_Init();

    /* 3. 注册用户业务任务 */
    // 初始化并自动挂载到就绪表
    tTaskInit(&tTask1, task1Entry, (void*)0x11111111, 0, &task1EntryEnv[1024]);
    tTaskInit(&tTask2, task2Entry, (void*)0x22222222, 1, &task2EntryEnv[1024]);
 
    /* 4. 启动操作系统调度 */
    // 找出当前最高优先级任务 (此处应为 prio=0 的 tTask1) 赋值给 nextTask
    nextTask = tTaskHighestReady();
    
    // 悬起 PendSV 异常，CPU 流程将正式移交给操作系统，一去不回头
    tTaskRunFirst();
    
    // 正常情况下，代码永远不会执行到这里
    return 0; 
}


/* BSP Functions -------------------------------------------------------------*/
/**
  * @brief  系统时钟配置 (完全保留你原来的逻辑)
  *         System Clock source            = PLL (HSE)
  *         SYSCLK(Hz)                     = 180000000
  *         ...
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  if( ret != HAL_OK) { while(1) {} }

  ret = HAL_PWREx_EnableOverDrive();
  if( ret != HAL_OK) { while(1) {} }
 
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
  
  if( ret != HAL_OK) { while(1) {} }
}

void SysTick_Handler(void)
{
    HAL_IncTick();               // 维持 HAL 库原有的时间基准
    tTaskSystemTickHandler();    // 驱动你的 tinyOS 的核心心跳和延时队列
}
