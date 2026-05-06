#ifndef _T_MEM_BLOCK_H
#define _T_MEM_BLOCK_H

#include "stdint.h"
#include "tLib.h"
#include "tEvent.h"

typedef struct _tMemBlock
{
    // 事件控制块
    tEventControlBlock event;

    // 存储块的首地址
    void * memStart;

    // 每个存储块的大小
    uint32_t blockSize;

    // 总的存储块的个数
    uint32_t maxCount;

    // 存储块列表
    tList blockList;
} tMemBlock;

void tMemBlockInit (tMemBlock * memBlock, uint8_t * memStart, uint32_t blockSize, uint32_t blockCnt);

//获取存储块
uint32_t tMemBlockWait (tMemBlock * memBlock, uint8_t ** mem, uint32_t waitTicks);
//无等待获取存储块
uint32_t tMemBlockNoWaitGet (tMemBlock * memBlock, void ** mem);
//释放存储块
void tMemBlockNotify (tMemBlock * memBlock, uint8_t * mem);
#endif
