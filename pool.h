#ifndef __POOL_H
#define __POOL_H

#include <pthread.h>
#include "queue.h"

//[1]���ֺ궨��
#define MAXJOB		200//���������
//#define MAXJOB		11//���������
#define MIN_FREE_NR	6//��С���е��̸߳���
#define MAX_FREE_NR	10//�����е��̸߳���
#define STEP		2//����(���ӻ�����߳�����)

//[2]�����̳߳صĽṹ��
typedef struct
{
	pthread_t *workers;//�����߳̽ṹ����ʼ��ַ(��̬����)
	pthread_t admin_tid;//�������߳�
	queue_t *task_queue;//�������(��֮ǰʵ�ֵĶ��������õ�)
	//�̳߳ؽṹ
	int max_threads;//����߳�����
	int min_free_threads;//���ٿ����̸߳���
	int max_free_threads;//�������̸߳���
	int busy_threads;//busy�߳���
	int live_threads;//live�߳���
	int exit_threads;//exit�߳���(��¼��Ҫ�ͷŵ��߳���)
	int shutdown;//����Ƿ�ر��̳߳�(1�ر��̳߳� 0��������)
	pthread_mutex_t mut_pool;//�����̳߳صĻ�����
	pthread_mutex_t mut_busy;//busy�̵߳Ļ�����
	pthread_cond_t queue_not_empty;//������в�Ϊ��,����ȡ�����֪ͨ
	pthread_cond_t queue_not_full;//������в�Ϊ��,�������������֪ͨ
}pool_t;

//[3]�̹߳�������Ľṹ
typedef struct
{
	void *(*job)(void *s);//����ָ��<�洢ִ������(����)�ĵ�ַ>
	void *arg;//ִ������Ĳ���
}task_t;

//[4]��װ�����ӿ�
/*
���� : ��ʼ���̳߳�
���� : mypool �Ѵ����ɹ����̳߳ػ���
		capacity �û�ָ��������(����̵߳�����)
����ֵ : �ɹ�����0;ʧ�ܷ���<0
*/
extern int pool_init(pool_t **mypool, int capacity);

/*
���� : ���̳߳����������
���� : mypool Ҫ�������̳߳�
		t Ҫ��ӵ�����
����ֵ : �ɹ�����0;ʧ�ܷ���<0
*/
extern int pool_add_task(pool_t *mypool, const task_t *t);

/*
���� : �����̳߳�
���� : mypool Ҫ���ٵ��̳߳�
����ֵ : ��
*/
extern void pool_destroy(pool_t *mypool);

#endif









