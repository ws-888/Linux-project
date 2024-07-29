#ifndef __QUEUE_H
#define __QUEUE_H

typedef struct
{
	void *s;//��ʼ��ַ
	int front;//��ͷ
	int tail;//��β
	int capacity;//����
	int size;//��С
}queue_t;

/*
���� : ���г�ʼ��
���� : q ����ٶ��нṹ�Ŀռ�
		capacity �ͻ�ָ��������(���ٸ��洢�ռ�)
		size �ͻ�ָ����һ���ռ�Ĵ�С
����ֵ : �ɹ�����0;ʧ�ܷ���<0
*/
extern int queue_init(queue_t **q, int capacity, int size);

/*
���� : �ж϶����Ƿ�Ϊ��
���� : q ָ��Ҫ�жϵĶ��нṹ�ռ�
����ֵ : Ϊ�շ���1;��Ϊ�շ���0
*/
extern int queue_is_empty(const queue_t *q);

/*
���� : �ж϶����Ƿ�Ϊ��
���� : q ָ��Ҫ�жϵĶ��нṹ�ռ�
����ֵ : Ϊ������1;��Ϊ������0
*/
extern int queue_is_full(const queue_t *q);

/*
���� : ���
���� : q ָ��Ҫ�����Ķ��нṹ�ռ�
		data ָ��Ҫ��ӵ�����
����ֵ : �ɹ�����0;ʧ�ܷ���<0
*/
extern int queue_enq(queue_t *q, const void *data);

/*
���� : ����
���� : q ָ��Ҫ�����Ķ��нṹ�ռ�
		data ָ��洢�ռ�(�洢���ӵ�����)
����ֵ : �ɹ�����0;ʧ�ܷ���<0
*/
extern int queue_dep(queue_t *q, void *data);

/*
���� : �ͷŶ���
���� : q ָ��Ҫ�����Ķ��нṹ�ռ�
����ֵ : ��
*/
extern void queue_destroy(queue_t *q);

#endif











