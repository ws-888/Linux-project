#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"

int queue_init(queue_t **q, int capacity, int size)
{
	*q = malloc(sizeof(queue_t));//���ٶ��нṹ�ռ�
	if(*q == NULL)//�жϿ��ٶ��нṹ�ռ��Ƿ�ʧ��
		return -1;//���ڿ��ٶ��нṹ�ռ�ʧ��,��������,���ҷ���-1
	(*q)->s = calloc(capacity + 1, size);//���ٶ��пռ�
	if((*q)->s == NULL)//�жϿ��ٶ��пռ��Ƿ�ʧ��
	{
		free(*q);//�ͷŶ��нṹ�ռ�
		return -2;//���ڿ��ٶ��пռ�ʧ��,��������,���ҷ���-2
	}
	(*q)->front = (*q)->tail = 0;//��ͷ�Ͷ�β���0��λ��
	(*q)->capacity = capacity + 1;//�洢�û�ָ��������
	(*q)->size = size;//�洢�û�ָ���Ĵ�С

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
	if(queue_is_full(q))//�ж϶����Ƿ�Ϊ��
		return -1;//���ڶ�������,���ʧ��,��������,���ҷ���-1
	memcpy((char *)q->s + q->tail * q->size, data, q->size);//�������
	q->tail = (q->tail + 1) % q->capacity;//��βƫ��

	return 0;
}

int queue_dep(queue_t *q, void *data)
{
	if(queue_is_empty(q))//�ж϶����Ƿ�Ϊ��
		return -1;//���ڶ��п���,����ʧ��,��������,���ҷ���-1
	memcpy(data, (char *)q->s + q->front * q->size, q->size);//��������
	memset((char *)q->s + q->front * q->size, '\0', q->size);//��ճ�������
	q->front = (q->front + 1) % q->capacity;//��ͷƫ��

	return 0;
}

void queue_destroy(queue_t *q)
{
	free(q->s);//�ͷŶ��пռ�
	q->s = NULL;//�������Ұָ��
	free(q);//�ͷŶ��нṹ�ռ�
	q = NULL;//�������Ұָ��
}
















