#include "Ttask.h"
#include "tEvent.h"
#include "tMbox.h"
#include "tMemBlock.h"
#include "tSem.h"
#include "tinyOS.h"
#include <stdint.h>
#include <string.h>

/* Global App Variables ------------------------------------------------------*/
/* 用户业务任务与堆栈定义 */
tTask tTask1;
tTask tTask2;
tTask tTask3;
tTask tTask4;

tTaskStack task1EntryEnv[1024];
tTaskStack task2EntryEnv[1024];
tTaskStack task3EntryEnv[1024]; 
tTaskStack task4EntryEnv[1024]; 



volatile uint32_t u32Task1Flag = 0;
volatile uint32_t u32Task2Flag = 0;
volatile uint32_t u32Task3Flag = 0;
volatile uint32_t u32Task4Flag = 0;

typedef uint8_t (*tBlock)[100] ;
uint32_t mem1[500];
tMemBlock memBlock1;

/* 业务任务 1 */
tTaskInfo Task1Info;
int test1;
void task1Entry(void* param)
{
    uint8_t *block[20];
    tMemBlockInit(&memBlock1, (uint8_t *)mem1, 100, 20);
    for(int i = 0; i < 20; i++)
    {
        tMemBlockWait(&memBlock1, &block[i], 0);
    }
    tTaskDelay(2);

//    for(int i = 0; i < 20; i++)
//    {
//        test1++;
//        memset(block[i], i, 100);
//        tMemBlockNotify(&memBlock1, (uint8_t *)block[i]);
//        tTaskDelay(2);
//    }
    // 核心业务循环
    for(;;)
    {
        u32Task1Flag = 0;
        tTaskDelay(100);  // 让出 CPU 10 个 ticks
        u32Task1Flag = 1;
        tTaskDelay(100);
    }
}

void delay()
{
  int i = 0;
  for(i = 0; i < 0xFF; i++){
    ;
  }
}

/* 业务任务 2 */
void task2Entry(void* param)
{
    // 核心业务循环
    for(;;)
    {
        u32Task2Flag = 0;
        tTaskDelay(200);
        u32Task2Flag = 1;
        tTaskDelay(200);
    }
}

/* 业务任务 3 */
void task3Entry(void* param)
{
    // 核心业务循环
    for(;;)
    {
        u32Task3Flag = 0;
        tTaskDelay(200);  // 让出 CPU 10 个 ticks
        u32Task3Flag = 1;
        tTaskDelay(200);
    }
}

/* 业务任务 4 */
void task4Entry(void* param)
{
    // 核心业务循环
    for(;;)
    {
        u32Task4Flag = 0;
        tTaskDelay(100);  // 让出 CPU 10 个 ticks
        u32Task4Flag = 1;
        tTaskDelay(100);
    }
}

/* app初始化函数 */
void appInit(void)
{
    // 初始化业务任务
    tTaskInit(&tTask1, task1Entry, (void*)0x11111111, 0, &task1EntryEnv[1024]);
    tTaskInit(&tTask2, task2Entry, (void*)0x22222222, 1, &task2EntryEnv[1024]);
    tTaskInit(&tTask3, task3Entry, (void*)0x33333333, 1, &task3EntryEnv[1024]);
    tTaskInit(&tTask4, task4Entry, (void*)0x44444444, 1, &task4EntryEnv[1024]);
}
