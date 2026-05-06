#include "tTask.h"
#include "tinyOS.h"

void tMboxInit(tMbox *mbox, void **msgBuffer, uint32_t maxCount)
{
    //初始化事件控制块
    tEventInit(&(mbox->ECB), tEventTypeMbox);
    
    mbox->maxCount=maxCount;
    mbox->Count=0;
    mbox->read=0;
    mbox->write=0;
    mbox->msgBuffer=msgBuffer;
}

//邮箱等待函数
uint32_t tMboxWait (tMbox * mbox, void **msg, uint32_t waitTicks)
{
    uint32_t status = tTaskEnterCritical();
    
    // 首先检查资源计数是否大于0
    if (mbox->Count > 0)
    {
        // 如果大于0的话，取出一个消息，并且计数减1
        --mbox->Count;
        *msg = mbox->msgBuffer[mbox->read++];
        
        // 同时修改读索引翻转，如果超出边界则回绕
        if (mbox->read >= mbox->maxCount)
        {
            mbox->read = 0;
        }
        tTaskExitCritical(status);
        return tErrorNone;
    }
    else
    {
        // 然后将任务插入事件队列中
        tEventWait(&mbox->ECB, currentTask, (void *)0, tEventTypeMbox, waitTicks);
        tTaskExitCritical(status);
        
        // 最后再执行一次事件调度，以便于切换到其它任务
        tTaskShed();
        
        // 当切换回来时（也就是任务被唤醒时了，再从上一次运行的地方继续运行），从tTask中取出获得的消息
        *msg = currentTask->eventMsg;
        
        // 取出等待结果
        return currentTask->waitEventResult;
    }
}

//获取邮箱中消息
uint32_t tMboxNoWaitGet (tMbox * mbox, void **msg)
{
    uint32_t status = tTaskEnterCritical();
    
    if (mbox->Count > 0)
    {
        --mbox->Count;
        *msg = mbox->msgBuffer[mbox->read++];
        
        if (mbox->read >= mbox->maxCount)
        {
            mbox->read = 0;
        }
        tTaskExitCritical(status);
        return tErrorNone;
    }
    else
    {
        //没有消息就返回了
        tTaskExitCritical(status);
        return tErrorResourceUnavailable;
    }
}

//消息通知函数：最后一个参数为通知选项，可以选择将消息插入到队列头部还是尾部
uint32_t tMboxNotify (tMbox * mbox, void * msg, uint32_t notifyOption)
{
    uint32_t status = tTaskEnterCritical();

    // 检查是否有任务等待
    if (tEventWaitCount(&mbox->ECB) > 0)
    {
        // 如果有的话，则直接唤醒位于队列首部（最先等待）的任务
        tTask * task = tEventWakeUp(&mbox->ECB, (void *)msg, tErrorNone );

        // 如果这个任务的优先级更高，就执行调度，切换过去
        if (task->prio < currentTask->prio)
        {
            tTaskShed();
        }
    }
    else
    {
        // 如果没有任务等待的话，将消息插入到缓冲区中
        if (mbox->Count >= mbox->maxCount)
        {
            tTaskExitCritical(status);
            return tErrorResourceFull;
        }

        // 可以选择将消息插入到头，这样后面任务获取消息的时候，优先获取该消息
        if (notifyOption & tMBOXSendFront)
        {
            if (mbox->read <= 0)
            {
                mbox->read = mbox->maxCount - 1;
            }
            else
            {
                --mbox->read;
            }
            mbox->msgBuffer[mbox->read] = msg;
        }
        else
        {
            mbox->msgBuffer[mbox->write++] = msg;
            if (mbox->write >= mbox->maxCount)
            {
                mbox->write = 0;
            }
        }

        // 增加消息计数
        mbox->Count++;
    }

    tTaskExitCritical(status);
    return tErrorNone;
}

//邮箱的清空函数（消息缓冲区清空）
void tMboxFlush (tMbox * mbox)
{
    uint32_t status = tTaskEnterCritical();

    // 如果队列中有任务等待，说明邮箱已经是空的了，不需要再清空
    if (tEventWaitCount(&mbox->ECB) == 0)
    {
        mbox->read = 0;
        mbox->write = 0;
        mbox->Count = 0;
    }

    tTaskExitCritical(status);
}

//删除邮箱函数（删除任务）
uint32_t tMboxDestroy (tMbox * mbox)
{
    uint32_t status = tTaskEnterCritical();

    // 清空事件控制块中的任务
    uint32_t count = tEventRemoveAll(&mbox->ECB, (void *)0, tErrorDel);

    tTaskExitCritical(status);

    // 清空过程中可能有任务就绪，执行一次调度
    if (count > 0)
    {
        tTaskShed();
    }
    return count;
}
//获取邮箱信息函数  
void tMboxGetInfo(tMbox *mbox, tMboxInfo *info)
{
    uint32_t status = tTaskEnterCritical();
    
    info->Count = mbox->Count;
    info->maxCount = mbox->maxCount;
    info->taskCount = tEventWaitCount(&mbox->ECB);
    
    tTaskExitCritical(status);
}

