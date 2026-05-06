#ifndef TMBOX_H
#define TMBOX_H

#include "tEvent.h"
#include <stdint.h>

#define tMoxSendNormal 0x00 // 消息插入选项：默认插入到队列尾部
#define tMBOXSendFront 0x01 // 消息插入选项：插入到队列头部

typedef struct _tMbox				//The structure of mailbox
{
	tEventControlBlock ECB;
	uint32_t Count;					//Current amount of messages
	uint32_t maxCount;				//Permitted max amount of messages
	uint32_t read;					//读索引
	uint32_t write;					//写索引
	void **msgBuffer;
	
}tMbox;

typedef struct _tMboxInfo{
    uint32_t Count;					//Current amount of messages
    uint32_t maxCount;				//Permitted max amount of messages
    uint32_t taskCount;				//当前等待该邮箱的任务数量
}tMboxInfo;

//邮箱的初始化结构
void tMboxInit(tMbox *mbox, void **msgBuffer, uint32_t maxCount);	//The initial function of mailbox

//邮箱等待函数
uint32_t tMboxWait (tMbox * mbox, void **msg, uint32_t waitTicks);
//获取邮箱中消息
uint32_t tMboxNoWaitGet (tMbox * mbox, void **msg);
//消息通知函数：最后一个参数为通知选项，可以选择将消息插入到队列头部还是尾部
uint32_t tMboxNotify (tMbox * mbox, void * msg, uint32_t notifyOption);
//邮箱的清空函数（消息缓冲区清空）
void tMboxFlush (tMbox * mbox);
//删除邮箱函数（删除任务）
uint32_t tMboxDestroy (tMbox * mbox);
//获取邮箱信息函数
void tMboxGetInfo(tMbox *mbox, tMboxInfo *info);

#endif
