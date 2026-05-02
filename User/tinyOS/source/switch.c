#include "stm32f4xx.h"

__asm void PendSV_Handler(void)
{
    /* 声明外部变量 */
    IMPORT currentTask    // 当前正在运行的任务指针
    IMPORT nextTask       // 即将要切换到的任务指针

    /* 判断是否是第一次启动任务 */
    MRS R0, PSP           // 读取进程堆栈指针到 R0
    CBZ R0, PendSVHandler_nosave  // 如果 R0 (PSP) 为 0，说明系统刚启动，没有“上一个任务”需要保存，直接跳过保存
    STMDB R0!, {R4-R11}

    /* 完成任务状态保存 */
    LDR R1, =currentTask
    LDR R1, [R1]
    STR R0, [R1]

PendSVHandler_nosave
    /* 将当前任务指针指向新任务 */
    LDR R0, =currentTask  // 取出 currentTask 变量的地址
    LDR R1, =nextTask     // 取出 nextTask 变量的地址
    LDR R2, [R1]          // 从 nextTask 地址中读取真正的任务控制块(TCB)指针
    STR R2, [R0]          // 将这个新任务的 TCB 指针写入 currentTask，完成切换

    /* 准备恢复新任务的现场 */
    LDR R0, [R2]          // 假设 TCB 的第一项就是栈指针(SP)，将其加载到 R0
    LDMIA R0!, {R4-R11}   // 从新任务的栈中弹出 R4-R11 寄存器的值（恢复现场）

    /* 更新堆栈指针并准备返回 */
    MSR PSP, R0           // 将恢复后的栈地址更新到 PSP 寄存器
    
    /* 修改 EXC_RETURN 值*/
    // LR 是 Link Register，在中断中它存放的是 EXC_RETURN 值
    // ORR 指令将第 2 位置 1 (0x04)，确保中断返回后使用 PSP 指针并在线程模式运行
    ORR LR, LR, #0x04     
    
    /* 异常返回 */
    BX LR                 // 退出中断。硬件会自动从 PSP 中弹出 R0-R3, R12, PC, xPSR
}


