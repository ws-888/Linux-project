#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "pool.h"

static void *admin_job(void *arg);//管理者线程做的事儿
static void *worker_job(void *arg);//工作线程做的事儿

int pool_init(pool_t **mypool, int capacity)
{
	pool_t *me = NULL;//存储开辟的线程池地址
	int err = 0;//记录错误码
	int i = 0;//循环变量

	me = malloc(sizeof(pool_t));//开辟线程池空间
	if(me == NULL)//判断开辟线程池空间是否失败
		return -errno;//由于开辟线程池空间失败,结束函数,并且返回-错误码
	/*int ret = pool_init(); if(ret < 0) strerror(-ret);//打印错误信息*/
	
	me->workers = calloc(capacity, sizeof(pthread_t));//动态开辟工作线程结构
	if(me->workers == NULL)//判断动态开辟工作线程结构是否失败
	{
		free(me);//释放开辟的线程池空间
		return -errno;//由于开辟工作线程结构失败,结束函数,并且返回-错误码
	}

	queue_init(&me->task_queue, MAXJOB, sizeof(task_t));//初始化任务队列
	me->max_threads = capacity;//存储客户指定的最多线程个数
	me->min_free_threads = MIN_FREE_NR;//指定最小空闲线程个数
	me->max_free_threads = MAX_FREE_NR;//指定最大空闲线程个数
	me->busy_threads = 0;//指定busy线程数为0
	me->live_threads = me->min_free_threads;//指定live线程数为最小空闲线程数
	me->exit_threads = 0;//指定需要终止的线程数为0
	me->shutdown = 0;//指定是否需要关闭线程池的标志为0(正常运行)

	pthread_mutex_init(&me->mut_pool, NULL);//初始化整个线程池的互斥量
	pthread_mutex_init(&me->mut_busy, NULL);//初始化busy线程的互斥量
	pthread_cond_init(&me->queue_not_empty, NULL);//初始化任务队列不为空的条件变量
	pthread_cond_init(&me->queue_not_full, NULL);//初始化任务队列不为满的条件变量

	err = pthread_create(&me->admin_tid, NULL, admin_job, me);//创建管理者线程
	if(err != 0)//判断创建管理者线程是否失败
	{
		free(me->workers);//释放工作线程结构
		free(me);//释放线程池
		return -err;//由于创建管理者线程失败,结束函数,并且返回-错误码
	}

	for(i = 0; i < me->min_free_threads; i++)//循环
	{//创建工作线程
		err = pthread_create(me->workers + i, NULL, worker_job, me);
		if(err != 0)//判断创建工作线程是否失败
		{
			free(me->workers);//释放工作线程结构
			free(me);//释放线程池
			return -err;//由于创建工作线程失败,结束函数,并且返回-错误码
		}
		pthread_detach((me->workers)[i]);//线程分离(将来可以不用收尸了(join))
	}

	*mypool = me;//把创建成功的线程池地址回填

	return 0;
}

static int __get_free_pos(pthread_t *jobs, int n)
{
	int i = 0;//循环变量

	for(i = 0; i < n; i++)//循环遍历
	{
		if(pthread_kill(jobs[i], 0) == 0)//检测线程是否存在
			return i;
	}
	return -1;
}

static void *admin_job(void *arg)//管理者线程做的事儿
{
	/*周期性(1s)查看线程池中busy线程的个数和free的个数,适量增加或减少*/
	pool_t *mypool = arg;//把形参的线程池地址另存到mypool中
	int busy_cnt = 0;//记录busy线程的个数
	int free_cnt = 0;//记录free线程的个数
	int i = 0;//循环变量
	int pos = 0;//标记可用位置

	while(1)
	{
		pthread_mutex_lock(&mypool->mut_pool);//抢整个线程池的互斥量
		if(mypool->shutdown)//判断线程出关闭标准是否为真
		{
			pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量
			break;//跳出循环
		}
		pthread_mutex_lock(&mypool->mut_busy);//抢busy线程的互斥量
		busy_cnt = mypool->busy_threads;//获取有多少个busy线程
		pthread_mutex_unlock(&mypool->mut_busy);//解busy线程的互斥量
		free_cnt = mypool->live_threads - busy_cnt;//计算空闲的线程数量

		//如果空闲的线程数量 大于 最大空闲线程数 + 增量--->减少线程数量
		if(free_cnt >= mypool->max_free_threads + STEP)
		{
			mypool->exit_threads = STEP;//每次减少STEP个线程
			for(i = 0; i < STEP; i++)//循环发送
			{
				pthread_cond_signal(&mypool->queue_not_empty);
				//欺骗(骗空闲线程队列不为空),真正的目的是为了释放空闲线程
			}
		}

		//如果现在busy的线程和live的线程一样多,而且不能达到上限-->增加线程
		if(busy_cnt == mypool->live_threads && \
			mypool->live_threads < mypool->max_threads)
		{
			for(i = 0; i < STEP; i++)//循环增加
			{//需要把新创建的线程放到合适的位置
				pos = __get_free_pos(mypool->workers, mypool->max_threads);
				//查找空闲的位置
				if(pos == -1)//判断是否没有位置了
				{//做错误处理
					fprintf(stderr, "[%d]__get_free_pos() failed\n", __LINE__);
				}
				pthread_create(mypool->workers + pos, NULL, worker_job, mypool);//创建绾????
				pthread_detach(mypool->workers[pos]);//线程分离(不用收尸)
				mypool->live_threads++;//创建成功后增加live线程数量
			}
		}

		pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量
		sleep(1);//延时1s
	}
	pthread_exit(0);//终止线程
}

static void *worker_job(void *arg)//工作线程做的事儿
{
	pool_t *mypool = arg;////把形参的线程池地址另存到mypool中
	task_t mytask;//存储要做鐨????务

	while(1)
	{
		pthread_mutex_lock(&mypool->mut_pool);//抢整个线程池的互斥量
		if(queue_is_empty(mypool->task_queue))//判断任务队列是否为空
		{//等待队列不为空的条件(原子操作)
			pthread_cond_wait(&mypool->queue_not_empty, &mypool->mut_pool);
		}
		if(mypool->shutdown)//判断是否线程池已关闭
		{
			pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量
			break;//跳出死循环
		}
		if(mypool->exit_threads > 0)//判断是否有线绋????要终止
		{
			mypool->exit_threads--;//终止线程之前减少要缁????线程的个数
			pthread_mutex_unlock(&mypool->mut_pool);//解�??个线程池的互斥量
			break;//跳出死循环
		}
		//任务来了
		queue_dep(mypool->task_queue, &mytask);//出队任务
		pthread_cond_signal(&mypool->queue_not_full);//发送队列不为满的条件
		pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量
		
		//做任务(free态线程变为busy态线程)
		pthread_mutex_lock(&mypool->mut_busy);//抢busy线程的互斥量
		mypool->busy_threads++;//增加busy线程的数量
		pthread_mutex_unlock(&mypool->mut_busy);//解busy线程的互斥量
		
		(mytask.job)(mytask.arg);//执行任务
		
		//�??完任务之后
		pthread_mutex_lock(&mypool->mut_busy);//抢busy绾????的互斥量
		mypool->busy_threads--;//减少busy线程的数量
		pthread_mutex_unlock(&mypool->mut_busy);//解busy线程的互斥量
	}
	pthread_exit(0);//终止线程
}

int pool_add_task(pool_t *mypool, const task_t *t)
{
	pthread_mutex_lock(&mypool->mut_pool);//鎶????个线程池的互斥量
	while(queue_is_full(mypool->task_queue))//判断任务队列是否为满
	{//等待任务队列不为满的条件
		pthread_cond_wait(&mypool->queue_not_full, &mypool->mut_pool);
	}
	queue_enq(mypool->task_queue, t);//任务入队
	pthread_cond_signal(&mypool->queue_not_empty);//发送队列不为空的条件
	pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量

	return 0;
}

void pool_destroy(pool_t *mypool)
{
	pthread_mutex_lock(&mypool->mut_pool);//抢整个线程池的互斥量
	mypool->shutdown = 1;//把线绋????关闭标志设置为1(关闭线程池)
	pthread_cond_broadcast(&mypool->queue_not_empty);//欺骗(欺骗空闲线程)
	pthread_mutex_unlock(&mypool->mut_pool);//解整个线程池的互斥量

	sleep(1);//等待1s(闃????1s为了等所有线程拿到shutdown = 1的状态)

	free(mypool->workers);//销毁工作线程的结构
	queue_destroy(mypool->task_queue);//销毁任务队列
	pthread_mutex_destroy(&mypool->mut_pool);//销毁鏁????线程池的互斥量
	pthread_mutex_destroy(&mypool->mut_busy);//销毁busy线程的互斥量
	pthread_cond_destroy(&mypool->queue_not_empty);//销毁任务队列不为空的条件
	pthread_cond_destroy(&mypool->queue_not_full);//销毁任务队列不为满的条件
	free(mypool);//销�??整个线程池结构
}


















