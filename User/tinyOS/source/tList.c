#include "tlib.h"
void tNodeInit(tNode* node)
{
    node->nextNode = node;
    node->preNode = node;
}

#define firstNode headNode.nextNode
#define lastNode headNode.preNode

void tListInit(tList* list)
{
    list->firstNode = &(list->headNode);
    list->lastNode = &(list->headNode);
    list->nodeCount = 0;
}

uint32_t tListCount(tList* list)
{
    return list->nodeCount;
}

tNode* tListFirst(tList* list)
{
    tNode* node = (tNode*)0;
    if(list->nodeCount > 0)
    {
        node =  list->firstNode;
    }
    return node;
}

tNode* tListLast(tList* list)
{
    tNode* node = (tNode*)0;
    if(list->nodeCount > 0)
    {
        node =  list->lastNode;
    }
    return node;
}
//返回指定节点的前一个节点
tNode* tListPreNode(tNode* node)
{
    if(node->preNode == node)
    {
        return (tNode*)0;
    }
    else
    {
        return node->preNode;
    }
}
//返回指定节点的后一个节点
tNode* tListNextNode(tNode* node)
{
    if(node->nextNode == node)
    {
        return (tNode*)0;
    }
    else
    {
        return node->nextNode;
    }
}

//删除链表中所有的节点
void tListRemoveAll(tList* list)
{
    uint32_t count;
    tNode* nextNode;
    nextNode = list->firstNode;
    for(count = list->nodeCount; count > 0; count--)
    {
        nextNode->preNode->nextNode = nextNode->nextNode;
        nextNode->nextNode->preNode = nextNode->preNode;
        nextNode = nextNode->nextNode;
    }
    list->firstNode = &(list->headNode);
    list->lastNode = &(list->headNode);
    list->nodeCount = 0;
}
//链表头部插入节点
void tListAddFirst(tList* list, tNode* node)
{
    node->preNode = list->firstNode->preNode;
    node->nextNode = list->firstNode;

    list->firstNode->preNode = node;
    list->firstNode = node;
    list->nodeCount++;
}
//链表尾部插入节点
void tListAddLast(tList* list, tNode* node)
{
    node->nextNode = &(list->headNode);
    node->preNode = list->lastNode;

    list->lastNode->nextNode = node;
    list->lastNode = node;
    list->nodeCount++;

}
//删除头部节点
tNode* tListRemoveFirst(tList* list)
{
    tNode* node = (tNode*)0;

    if(list->nodeCount != 0)

    {

        node = list->firstNode;

        node->nextNode->preNode = &(list->headNode);

        list->firstNode = node->nextNode;

        list->nodeCount--;
    }
    return node;
}

//在指定的节点后面插入节点
void tListInsertAfter(tList* list, tNode* node, tNode* newNode)
{
    newNode->preNode = node;
    newNode->nextNode = node->nextNode;

    node->nextNode->preNode = newNode;
    node->nextNode = newNode;
    if(node == list->lastNode)
    {
        list->lastNode = newNode;
    }
    list->nodeCount++;
}



//从链表中删除指定节点
void tListRemoveNode(tList* list, tNode* node)
{
    node->preNode->nextNode = node->nextNode;
    node->nextNode->preNode = node->preNode;
    list->nodeCount--;
}
