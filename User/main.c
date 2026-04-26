/**
  ******************************************************************************
  * @file    main.c
  * @author  wt
  * @version V1.0
  * @date    2026-04-06
  * @brief   RTOS自主实现
  ******************************************************************************
  * @attention
  *
  * STM32 F429 开发板 
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_armcc.h"
#include "stm32f429xx.h"
#include "stm32f4xx.h"
#include "./led/bsp_led.h"
#include "stm32f4xx_hal.h"
#include "tinyOS.h"
#include <stdint.h>

/* Defines ------------------------------------------------------------------*/
#define MEM32(addr)  *(volatile unsigned long*)(addr)
#define MEM8(addr)   *(volatile unsigned char*)(addr)

/* Global ------------------------------------------------------------------*/
tTask* currentTask;
tTask* nextTask;
tTask* idleTask;
tTask* taskTable[2];

unsigned tickCounter;
/* Static ------------------------------------------------------------------*/
static void SystemClock_Config(void);

/* Function ------------------------------------------------------------------*/

uint32_t tTaskEnterCritical(void)
{
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}


uint32_t tTaskExitCritical(uint32_t status)
{
  __set_PRIMASK(status);
}



/* 任务初始化函数 */
void tTaskInit(tTask * task, void(*entry)(void *), void* param, tTaskStack* stack)
{
  /* 以下为硬件进入/退出PnedSV自动保存的寄存器 */
  *(--stack) = (unsigned long)(1 << 24); /* 必须xpsr中的24位必须置1，否则会进入arm模式 */
  *(--stack) = (unsigned long)entry;
  *(--stack) = (unsigned long)0x14;
  *(--stack) = (unsigned long)0x12;
  *(--stack) = (unsigned long)0x3;
  *(--stack) = (unsigned long)0x2;
  *(--stack) = (unsigned long)0x1;
  *(--stack) = (unsigned long)param;//R0 保存程序的入口参数

  /* 其他寄存器 */
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
}

/* 两个任务 */
tTask tTask1;
tTask tTask2;
tTask tTaskIdle;
/* 任务栈 */
tTaskStack task1EntryEnv[1024];
tTaskStack task2EntryEnv[1024];
tTaskStack idleTaskEnv[1024];

/* 设置系统时钟 */
void SetSysytemClock(uint32_t ms)
{
    // 1. 修正毫秒计算公式：除以 1000
    // 当主频为 180MHz 时，1ms 对应的 tick 数是 180,000
    SysTick->LOAD = ms * (SystemCoreClock / 1000) - 1; 

    // 2. 修正优先级计算：必须是 (1 << 位数) - 1
    // 结果为 15，确保 SysTick 是最低优先级
    NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 1); 

    // 3. 清空当前计数器
    SysTick->VAL = 0; 

    // 4. 使用系统主频，使能中断与定时器
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk; 
}


/* 任务调度函数 */
void tTaskShed()
{
  uint32_t status = tTaskEnterCritical();
  if(currentTask == idleTask)
  {
    if(taskTable[0]->delayTicks == 0)
    {
      nextTask = taskTable[0];
    }
    else if (taskTable[1]->delayTicks == 0) {
      nextTask = taskTable[1];
    }
    else {
    {
      return;
    }
    }
  }else{
    if(currentTask == taskTable[0])
    {
      if(taskTable[1]->delayTicks == 0)
      {
        nextTask = taskTable[1];
      }
      else if (currentTask->delayTicks != 0) {
        nextTask = idleTask;
      }else{
        tTaskExitCritical(status);
        return;
      }
    }
    else if(currentTask == taskTable[1])
    {
      if(taskTable[0]->delayTicks == 0)
      {
        nextTask = taskTable[0];
      }
      else if (currentTask->delayTicks != 0) {
        nextTask = idleTask;
      }else {
        tTaskExitCritical(status);
        return;
      }
    }
  }

  tTaskSwitch();
  tTaskExitCritical(status);
}

void tTaskDelay(uint32_t delay)
{
  uint32_t status = tTaskEnterCritical();
  currentTask->delayTicks = delay*0.1;
  tTaskShed();
  tTaskExitCritical(status);
}


static volatile uint32_t u32Task1Flag = 0;
void task1Entry(void* param)
{
  SetSysytemClock(1);
  for(;;)
  {
    u32Task1Flag = 0;
    tTaskDelay(100);
    u32Task1Flag = 1;
    tTaskDelay(100);
  }
}
static volatile uint32_t u32Task2Flag = 0;
void  task2Entry(void* param)
{
  for(;;)
  {
    u32Task2Flag = 0;
    tTaskDelay(100);
    u32Task2Flag = 1;
    tTaskDelay(100);
  }
}
static volatile uint32_t u32TaskIdleFlag = 0;
void IdleTaskEntry(void* param)
{
  for(;;)
  {
    u32TaskIdleFlag = 1;
    tTaskDelay(100);
    u32TaskIdleFlag = 0;
    tTaskDelay(100);
  }
}

void tTaskSystemTickHandler()
{
  uint32_t status = tTaskEnterCritical();
  int i;
  for(i = 0; i < 2; i++)
  {
    if(taskTable[i]->delayTicks > 0)
    {
      taskTable[i]->delayTicks--;
    }
  }
  tickCounter++;
  tTaskShed();
  tTaskExitCritical(status);
}



void SysTick_Handler(void)
{
    HAL_IncTick();
    tTaskSystemTickHandler();
}

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{
    /* 系统时钟初始化成216 MHz */
    SystemClock_Config();

    /* LED 端口初始化 */
    LED_GPIO_Config();

    taskTable[0] = &tTask1;
    taskTable[1] = &tTask2;
    nextTask = taskTable[0];

    idleTask = &tTaskIdle;

    tTaskInit(&tTask1, task1Entry, (void*)0x11111111, &task1EntryEnv[1024]);
    tTaskInit(&tTask2, task2Entry, (void*)0x22222222, & task2EntryEnv[1024]);
    tTaskInit(&tTaskIdle, IdleTaskEntry, (void*)0x33333333, & idleTaskEnv[1024]);
    
    tTaskRunFirst();
    
    return 0;
}

/**
  * @brief  系统时钟配置 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 12000000
  *            PLL_M                          = 25
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  无
  * @retval 无
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
   /* 使能HSE，配置HSE为PLL的时钟源，配置PLL的各种分频因子M N P Q 
	  * PLLCLK = HSE/M*N/P = 12M / 25 *360 / 2 = 180M
	  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 360;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  
  if( ret != HAL_OK)
  {
    while(1) {}
  }

  /* 激活 OverDrive 模式以达到180M频率 */
  ret =HAL_PWREx_EnableOverDrive();
   if( ret != HAL_OK)
  {
    while(1) {}
  }
 
  /* 选择PLLCLK作为SYSCLK，并配置 HCLK, PCLK1 and PCLK2 的时钟分频因子 
	 * SYSCLK = PLLCLK     = 216M
	 * HCLK   = SYSCLK / 1 = 216M
	 * PCLK2  = SYSCLK / 2 = 108M
	 * PCLK1  = SYSCLK / 4 = 54M
	 */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  
  ret =HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);
  
   if( ret != HAL_OK)
  {
    while(1) {}
  }
}




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
