#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "pool.h"

static void *admin_job(void *arg);//�������߳������¶�
static void *worker_job(void *arg);//�����߳������¶�

int pool_init(pool_t **mypool, int capacity)
{
	pool_t *me = NULL;//�洢���ٵ��̳߳ص�ַ
	int err = 0;//��¼������
	int i = 0;//ѭ������

	me = malloc(sizeof(pool_t));//�����̳߳ؿռ�
	if(me == NULL)//�жϿ����̳߳ؿռ��Ƿ�ʧ��
		return -errno;//���ڿ����̳߳ؿռ�ʧ��,��������,���ҷ���-������
	/*int ret = pool_init(); if(ret < 0) strerror(-ret);//��ӡ������Ϣ*/
	
	me->workers = calloc(capacity, sizeof(pthread_t));//��̬���ٹ����߳̽ṹ
	if(me->workers == NULL)//�ж϶�̬���ٹ����߳̽ṹ�Ƿ�ʧ��
	{
		free(me);//�ͷſ��ٵ��̳߳ؿռ�
		return -errno;//���ڿ��ٹ����߳̽ṹʧ��,��������,���ҷ���-������
	}

	queue_init(&me->task_queue, MAXJOB, sizeof(task_t));//��ʼ���������
	me->max_threads = capacity;//�洢�ͻ�ָ��������̸߳���
	me->min_free_threads = MIN_FREE_NR;//ָ����С�����̸߳���
	me->max_free_threads = MAX_FREE_NR;//ָ���������̸߳���
	me->busy_threads = 0;//ָ��busy�߳���Ϊ0
	me->live_threads = me->min_free_threads;//ָ��live�߳���Ϊ��С�����߳���
	me->exit_threads = 0;//ָ����Ҫ��ֹ���߳���Ϊ0
	me->shutdown = 0;//ָ���Ƿ���Ҫ�ر��̳߳صı�־Ϊ0(��������)

	pthread_mutex_init(&me->mut_pool, NULL);//��ʼ�������̳߳صĻ�����
	pthread_mutex_init(&me->mut_busy, NULL);//��ʼ��busy�̵߳Ļ�����
	pthread_cond_init(&me->queue_not_empty, NULL);//��ʼ��������в�Ϊ�յ���������
	pthread_cond_init(&me->queue_not_full, NULL);//��ʼ��������в�Ϊ������������

	err = pthread_create(&me->admin_tid, NULL, admin_job, me);//�����������߳�
	if(err != 0)//�жϴ����������߳��Ƿ�ʧ��
	{
		free(me->workers);//�ͷŹ����߳̽ṹ
		free(me);//�ͷ��̳߳�
		return -err;//���ڴ����������߳�ʧ��,��������,���ҷ���-������
	}

	for(i = 0; i < me->min_free_threads; i++)//ѭ��
	{//���������߳�
		err = pthread_create(me->workers + i, NULL, worker_job, me);
		if(err != 0)//�жϴ��������߳��Ƿ�ʧ��
		{
			free(me->workers);//�ͷŹ����߳̽ṹ
			free(me);//�ͷ��̳߳�
			return -err;//���ڴ��������߳�ʧ��,��������,���ҷ���-������
		}
		pthread_detach((me->workers)[i]);//�̷߳���(�������Բ�����ʬ��(join))
	}

	*mypool = me;//�Ѵ����ɹ����̳߳ص�ַ����

	return 0;
}

static int __get_free_pos(pthread_t *jobs, int n)
{
	int i = 0;//ѭ������

	for(i = 0; i < n; i++)//ѭ������
	{
		if(pthread_kill(jobs[i], 0) == 0)//����߳��Ƿ����
			return i;
	}
	return -1;
}

static void *admin_job(void *arg)//�������߳������¶�
{
	/*������(1s)�鿴�̳߳���busy�̵߳ĸ�����free�ĸ���,�������ӻ����*/
	pool_t *mypool = arg;//���βε��̳߳ص�ַ��浽mypool��
	int busy_cnt = 0;//��¼busy�̵߳ĸ���
	int free_cnt = 0;//��¼free�̵߳ĸ���
	int i = 0;//ѭ������
	int pos = 0;//��ǿ���λ��

	while(1)
	{
		pthread_mutex_lock(&mypool->mut_pool);//�������̳߳صĻ�����
		if(mypool->shutdown)//�ж��̳߳��رձ�׼�Ƿ�Ϊ��
		{
			pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����
			break;//����ѭ��
		}
		pthread_mutex_lock(&mypool->mut_busy);//��busy�̵߳Ļ�����
		busy_cnt = mypool->busy_threads;//��ȡ�ж��ٸ�busy�߳�
		pthread_mutex_unlock(&mypool->mut_busy);//��busy�̵߳Ļ�����
		free_cnt = mypool->live_threads - busy_cnt;//������е��߳�����

		//������е��߳����� ���� �������߳��� + ����--->�����߳�����
		if(free_cnt >= mypool->max_free_threads + STEP)
		{
			mypool->exit_threads = STEP;//ÿ�μ���STEP���߳�
			for(i = 0; i < STEP; i++)//ѭ������
			{
				pthread_cond_signal(&mypool->queue_not_empty);
				//��ƭ(ƭ�����̶߳��в�Ϊ��),������Ŀ����Ϊ���ͷſ����߳�
			}
		}

		//�������busy���̺߳�live���߳�һ����,���Ҳ��ܴﵽ����-->�����߳�
		if(busy_cnt == mypool->live_threads && \
			mypool->live_threads < mypool->max_threads)
		{
			for(i = 0; i < STEP; i++)//ѭ������
			{//��Ҫ���´������̷߳ŵ����ʵ�λ��
				pos = __get_free_pos(mypool->workers, mypool->max_threads);
				//���ҿ��е�λ��
				if(pos == -1)//�ж��Ƿ�û��λ����
				{//��������
					fprintf(stderr, "[%d]__get_free_pos() failed\n", __LINE__);
				}
				pthread_create(mypool->workers + pos, NULL, worker_job, mypool);//�����????
				pthread_detach(mypool->workers[pos]);//�̷߳���(������ʬ)
				mypool->live_threads++;//�����ɹ�������live�߳�����
			}
		}

		pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����
		sleep(1);//��ʱ1s
	}
	pthread_exit(0);//��ֹ�߳�
}

static void *worker_job(void *arg)//�����߳������¶�
{
	pool_t *mypool = arg;////���βε��̳߳ص�ַ��浽mypool��
	task_t mytask;//�洢Ҫ���????��

	while(1)
	{
		pthread_mutex_lock(&mypool->mut_pool);//�������̳߳صĻ�����
		if(queue_is_empty(mypool->task_queue))//�ж���������Ƿ�Ϊ��
		{//�ȴ����в�Ϊ�յ�����(ԭ�Ӳ���)
			pthread_cond_wait(&mypool->queue_not_empty, &mypool->mut_pool);
		}
		if(mypool->shutdown)//�ж��Ƿ��̳߳��ѹر�
		{
			pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����
			break;//������ѭ��
		}
		if(mypool->exit_threads > 0)//�ж��Ƿ������????Ҫ��ֹ
		{
			mypool->exit_threads--;//��ֹ�߳�֮ǰ����Ҫ�????�̵߳ĸ���
			pthread_mutex_unlock(&mypool->mut_pool);//��1�7??���̳߳صĻ�����
			break;//������ѭ��
		}
		//��������
		queue_dep(mypool->task_queue, &mytask);//��������
		pthread_cond_signal(&mypool->queue_not_full);//���Ͷ��в�Ϊ��������
		pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����
		
		//������(free̬�̱߳�Ϊbusy̬�߳�)
		pthread_mutex_lock(&mypool->mut_busy);//��busy�̵߳Ļ�����
		mypool->busy_threads++;//����busy�̵߳�����
		pthread_mutex_unlock(&mypool->mut_busy);//��busy�̵߳Ļ�����
		
		(mytask.job)(mytask.arg);//ִ������
		
		//�1�7??������֮��
		pthread_mutex_lock(&mypool->mut_busy);//��busy�????�Ļ�����
		mypool->busy_threads--;//����busy�̵߳�����
		pthread_mutex_unlock(&mypool->mut_busy);//��busy�̵߳Ļ�����
	}
	pthread_exit(0);//��ֹ�߳�
}

int pool_add_task(pool_t *mypool, const task_t *t)
{
	pthread_mutex_lock(&mypool->mut_pool);//�????���̳߳صĻ�����
	while(queue_is_full(mypool->task_queue))//�ж���������Ƿ�Ϊ��
	{//�ȴ�������в�Ϊ��������
		pthread_cond_wait(&mypool->queue_not_full, &mypool->mut_pool);
	}
	queue_enq(mypool->task_queue, t);//�������
	pthread_cond_signal(&mypool->queue_not_empty);//���Ͷ��в�Ϊ�յ�����
	pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����

	return 0;
}

void pool_destroy(pool_t *mypool)
{
	pthread_mutex_lock(&mypool->mut_pool);//�������̳߳صĻ�����
	mypool->shutdown = 1;//�����????�رձ�־����Ϊ1(�ر��̳߳�)
	pthread_cond_broadcast(&mypool->queue_not_empty);//��ƭ(��ƭ�����߳�)
	pthread_mutex_unlock(&mypool->mut_pool);//�������̳߳صĻ�����

	sleep(1);//�ȴ�1s(�????1sΪ�˵������߳��õ�shutdown = 1��״̬)

	free(mypool->workers);//���ٹ����̵߳Ľṹ
	queue_destroy(mypool->task_queue);//�����������
	pthread_mutex_destroy(&mypool->mut_pool);//�����????�̳߳صĻ�����
	pthread_mutex_destroy(&mypool->mut_busy);//����busy�̵߳Ļ�����
	pthread_cond_destroy(&mypool->queue_not_empty);//����������в�Ϊ�յ�����
	pthread_cond_destroy(&mypool->queue_not_full);//����������в�Ϊ��������
	free(mypool);//���1�7??�����̳߳ؽṹ
}


















