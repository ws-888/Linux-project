#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int queue_init(queue_t **q, int capacity, int size)
{
	*q = malloc(sizeof(queue_t));//开辟队列结构空间
	if(*q == NULL)//判断开辟队列结构空间是否失败
		return -1;//由于开辟队列结构空间失败,结束函数,并且返回-1
	(*q)->s = calloc(capacity + 1, size);//开辟队列空间
	if((*q)->s == NULL)//判断开辟队列空间是否失败
	{
		free(*q);//释放队列结构空间
		return -2;//由于开辟队列空间失败,结束函数,并且返回-2
	}
	(*q)->front = (*q)->tail = 0;//队头和队尾标记0的位置
	(*q)->capacity = capacity + 1;//存储用户指定的容量
	(*q)->size = size;//存储用户指定的大小

	return 0;
}

int queue_is_empty(const queue_t *q)
{
	return q->front == q->tail;
}

int queue_is_full(const queue_t *q)
{
	return (q->tail + 1) % q->capacity == q->front;
}

int queue_enq(queue_t *q, const void *data)
{
	if(queue_is_full(q))//判断队列是否为满
		return -1;//由于队列满了,入队失败,结束函数,并且返回-1
	memcpy((char *)q->s + q->tail * q->size, data, q->size);//入队数据
	q->tail = (q->tail + 1) % q->capacity;//队尾偏移

	return 0;
}

int queue_dep(queue_t *q, void *data)
{
	if(queue_is_empty(q))//判断队列是否为空
		return -1;//由于队列空了,出队失败,结束函数,并且返回-1
	memcpy(data, (char *)q->s + q->front * q->size, q->size);//出队数据
	memset((char *)q->s + q->front * q->size, '\0', q->size);//清空出队数据
	q->front = (q->front + 1) % q->capacity;//队头偏移

	return 0;
}

void queue_destroy(queue_t *q)
{
	free(q->s);//释放队列空间
	q->s = NULL;//避免出现野指针
	free(q);//释放队列结构空间
	q = NULL;//避免出现野指针
}
















