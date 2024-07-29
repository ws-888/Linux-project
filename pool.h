#ifndef __POOL_H
#define __POOL_H

#include <pthread.h>
#include "queue.h"

//[1]各种宏定义
#define MAXJOB		200//最大任务数
//#define MAXJOB		11//最大任务数
#define MIN_FREE_NR	6//最小空闲的线程个数
#define MAX_FREE_NR	10//最大空闲的线程个数
#define STEP		2//增量(增加或减少线程数量)

//[2]定义线程池的结构体
typedef struct
{
	pthread_t *workers;//工作线程结构的起始地址(动态开辟)
	pthread_t admin_tid;//管理者线程
	queue_t *task_queue;//任务队列(把之前实现的队列拿来用的)
	//线程池结构
	int max_threads;//最多线程容量
	int min_free_threads;//最少空闲线程个数
	int max_free_threads;//最多空闲线程个数
	int busy_threads;//busy线程数
	int live_threads;//live线程数
	int exit_threads;//exit线程数(记录需要释放的线程数)
	int shutdown;//标记是否关闭线程池(1关闭线程池 0正常运行)
	pthread_mutex_t mut_pool;//整个线程池的互斥量
	pthread_mutex_t mut_busy;//busy线程的互斥量
	pthread_cond_t queue_not_empty;//如果队列不为空,发送取任务的通知
	pthread_cond_t queue_not_full;//如果队列不为满,发送增加任务的通知
}pool_t;

//[3]线程工作任务的结构
typedef struct
{
	void *(*job)(void *s);//函数指针<存储执行任务(工作)的地址>
	void *arg;//执行任务的参数
}task_t;

//[4]封装函数接口
/*
功能 : 初始化线程池
参数 : mypool 把创建成功的线程池回填
		capacity 用户指定的容量(最多线程的容量)
返回值 : 成功返回0;失败返回<0
*/
extern int pool_init(pool_t **mypool, int capacity);

/*
功能 : 往线程池中添加任务
参数 : mypool 要操作的线程池
		t 要添加的任务
返回值 : 成功返回0;失败返回<0
*/
extern int pool_add_task(pool_t *mypool, const task_t *t);

/*
功能 : 销毁线程池
参数 : mypool 要销毁的线程池
返回值 : 空
*/
extern void pool_destroy(pool_t *mypool);

#endif









