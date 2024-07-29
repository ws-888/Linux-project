#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "pool.h"
#include "media_lib.h"
#include "netradio_protocol.h"
//#include "chn_data.h"

#define BUFSIZE 100

struct sockaddr_in remote_addr;//对端地址

struct ip_mreqn imr;//组播选项结构体

struct mlib_list_entry *mylib;//保存获取到的频道列表数组

struct chn_list_st *list_data; //

int size1;

int sd;

static void *task1(void *s); // 发送频道列表
static void *task2(void *s); // 发送音乐
int chn_list_init(struct chn_list_st **list_data, int n, const struct mlib_list_entry *mylib);

int main(void)
{
    int n;//保存获取到的频道个数
    int i = 0;//循环变量
    pool_t *p = NULL;//定义线程池类型的指针
    task_t t;//定义任务结构体变量
	/*守护进程*/
/*	if(daemon(0, 0) < 0)
    { 
        perror("daemon()");//打印错误信息
        return -1;
    }
	
*/
    mlib_get_chn_list(&mylib, &n);//获取频道列表
    pool_init(&p, 1); //线程池初始化

	/************组播**********************/
    sd = socket(AF_INET, SOCK_DGRAM, 0);//创建UDP套接字
    if(sd == -1)//判断创建UDP套接字是否失败
    {
        perror("socket()");//打印错误信息
        return -3;//由于创建UDP套接字失败,结束程序,并且返回-3
    }
    inet_aton(MULTICAST_ADDR, &imr.imr_multiaddr);//转换组播地址
    inet_aton(LOCAL_IP, &imr.imr_address);//转换本地地址
    imr.imr_ifindex = if_nametoindex(NETCARD_NAME);//转换网卡名
    if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, \
                &imr, sizeof(imr)) == -1)//判断多播组使能是否失败
    {
        perror("setsockopt()");//打印错误信息
        close(sd);//关闭套接字
        return -4;//由于多播组使能失败,结束程序,并且返回-4
    }
    remote_addr.sin_family = AF_INET;//指定IPV4协议
    inet_aton(MULTICAST_ADDR, &remote_addr.sin_addr);//填入多播组地址
    remote_addr.sin_port = htons(RECV_PORT);//填入端口号
	/*************组播end********************/
	
    for(i = 0; i < n; i++)//
    {
        printf("[%d]    %s\n", mylib[i].chnid, mylib[i].descr);
    }
	/********将发送频道列表数据加入到线程池******************************/
	t.job = task1;//指定任务
	t.arg = NULL;//指定任务的参数
	pool_add_task(p, &t);//添加任务
	/*******************************************************************/
	chn_list_init(&list_data,n,mylib);//将频道列表存到list_data 空间中

//	write(1,list_data,size1);
	printf("-------list---------\n");
	struct list_entry *pos;
	for(i=0,pos = list_data->entry;i<n;pos = (void*)((char *)pos) + ntohs(pos->len),i++)
	{
		printf("channel:%d %s", pos->chnid, pos->descr);
	}
	
	/********将发送频道数据的任务加入到线程池*****************/
	for(i = 0;i<n;i++)
	{
		t.job = task2;
		t.arg = (void *)(mylib[i].chnid);
		pool_add_task(p, &t);
	}
	/*****************************************************/

	while(1);

    return 0;
}

//发送频道数据
static void *task2(void *s)
{
	int n1 = (int)s;//存储频道号
    int count = 0;
	struct chn_data_st send_msg;  //频道数据结构体
	send_msg.chnid = n1; 
    while(1)
    {
		memset(send_msg.msg,0,MSG_SIZE);
        count = mlib_read_chn_data(n1, send_msg.msg,MSG_SIZE); // 读频道音乐文件
//		printf("send====send_msg.chnid=%d\n",send_msg.chnid); //测试
		//组播
		if(count == 0)
			continue;
        sendto(sd, &send_msg,sizeof(chnid_t)+count, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
//		sleep(1);
		usleep(2500);
	}
}

//发送频道列表 每1s发送一次
static void *task1(void *s)
{
	int n = (int)s;
	int i = 0;
	//union chn_recv_st send_list_data;
//	memcpy(&send_list_data,list_data,size1);
	while(1)
    {//size1 全局变量 存储list_data大小
		sendto(sd,list_data ,size1, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));

		sleep(1);
	}
}

//将频道列表存到list_data空间
int chn_list_init(struct chn_list_st **list_data,int n,const struct mlib_list_entry *mlib)
{
	int i=0;
	size_t* mem=malloc(sizeof(size_t)*n);//文件描述的长度
	int sum=0;
	for(i=0;i<n;i++)
	{
		mem[i]=strlen(mlib[i].descr);
		sum+=mem[i];
	}
	*list_data=malloc(sizeof(chnid_t)+n*(sizeof(chnid_t)+sizeof(len_t))+sum);
	(*list_data)->chnid=0;
	size1 = sizeof(chnid_t)+n*(sizeof(chnid_t)+sizeof(len_t))+sum;
	struct list_entry  *ptr = (*list_data)->entry;
	for(i=0;i<n;i++)
	{
		ptr->chnid=mlib[i].chnid;
		ptr->len=htons(mem[i]+sizeof(struct list_entry));
		memcpy(ptr->descr,mlib[i].descr,mem[i]);
		//strcpy(ptr->descr,mlib[i].descr);
		ptr = (void*)((char*)ptr + mem[i]+sizeof(struct list_entry));
	}
	free(mem);
	return 0;
}
