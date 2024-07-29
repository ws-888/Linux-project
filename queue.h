#ifndef __QUEUE_H
#define __QUEUE_H

typedef struct
{
	void *s;//起始地址
	int front;//队头
	int tail;//队尾
	int capacity;//容量
	int size;//大小
}queue_t;

/*
功能 : 队列初始化
参数 : q 回填开辟队列结构的空间
		capacity 客户指定的容量(多少个存储空间)
		size 客户指定的一个空间的大小
返回值 : 成功返回0;失败返回<0
*/
extern int queue_init(queue_t **q, int capacity, int size);

/*
功能 : 判断队列是否为空
参数 : q 指向要判断的队列结构空间
返回值 : 为空返回1;不为空返回0
*/
extern int queue_is_empty(const queue_t *q);

/*
功能 : 判断队列是否为满
参数 : q 指向要判断的队列结构空间
返回值 : 为满返回1;不为满返回0
*/
extern int queue_is_full(const queue_t *q);

/*
功能 : 入队
参数 : q 指向要操作的队列结构空间
		data 指向要入队的数据
返回值 : 成功返回0;失败返回<0
*/
extern int queue_enq(queue_t *q, const void *data);

/*
功能 : 出队
参数 : q 指向要操作的队列结构空间
		data 指向存储空间(存储出队的数据)
返回值 : 成功返回0;失败返回<0
*/
extern int queue_dep(queue_t *q, void *data);

/*
功能 : 释放队列
参数 : q 指向要操作的队列结构空间
返回值 : 空
*/
extern void queue_destroy(queue_t *q);

#endif











