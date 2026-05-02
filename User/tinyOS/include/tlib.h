#ifndef TLIB_H
#define TLIB_H

#include <stdint.h>
typedef struct
{
    uint32_t bitmap;
}tBitmap;

void tBitmapInit(tBitmap* bitmap);
uint32_t tBitmapPosCount(void);
void tBitmapSet(tBitmap* bitmap, uint32_t bitIndex);    
void tBitmapClear(tBitmap* bitmap, uint32_t bitIndex);
uint32_t tBitmapGetFirstSet(tBitmap* bitmap);

//节点定义
typedef struct _tNode
{
    struct _tNode* nextNode;
    struct _tNode* preNode;
}tNode;

//链表定义
typedef struct _tList
{
    tNode headNode;
    uint32_t nodeCount;
}tList;

//通过结构体中的一个字段得出结构体的地址
#define tNodeParent(node, parentType, name) \
    ((parentType*)((uint32_t)node - (uint32_t)(&((parentType*)0)->name)))

// 节点初始化
void tNodeInit(tNode* node);

// 链表初始化
void tListInit(tList* list);

// 获取链表节点数量
uint32_t tListCount(tList* list);

// 获取链表第一个节点
tNode* tListFirst(tList* list);

// 获取链表最后一个节点
tNode* tListLast(tList* list);

// 返回指定节点的前一个节点
tNode* tListPreNode(tNode* node);

// 返回指定节点的后一个节点
tNode* tListNextNode(tNode* node);

// 删除链表中所有的节点
void tListRemoveAll(tList* list);

// 链表头部插入节点
void tListAddFirst(tList* list, tNode* node);

// 链表尾部插入节点
void tListAddLast(tList* list, tNode* node);

// 删除头部节点
tNode* tListRemoveFirst(tList* list);

// 在指定的节点后面插入节点
void tListInsertAfter(tList* list, tNode* node, tNode* newNode);

// 从链表中删除指定节点
void tListRemoveNode(tList* list, tNode* node);

#endif
