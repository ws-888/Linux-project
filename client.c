#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include "netradio_protocol.h"
//#include "protocol.h"
#include <signal.h>
#include <setjmp.h>
#define BUFFERSIZE 1024
int flag=0;
//jmp_buf env;

static void handler()
{
	//longjmp(env,1);
	flag=1;
}

int CURSIZE = 0;
int main(int argc,char *argv[])
{
	char *buffer = malloc(BUFFERSIZE);
	int fd[2];//管道描述符
	char a[10]={0};
	int sd;//存储UDP套接字描述符
	char buf[MSG_SIZE];
	struct chn_data_st nmsg;
	socklen_t len;
	struct sockaddr_in addr;//本地地址
	struct sockaddr_in server_addr;//回填服务端地址
	socklen_t serveraddr_len;
	struct ip_mreqn imr;
	int count;
	pid_t pid;
	int ret = 0;//频道选择
	int chosenid;//频道选择
	//struct chn_data_st *chn_data;//频道数据
	struct sockaddr_in raddr;//
	socklen_t raddr_len;
	//创建管道
	if(pipe(fd)==-1)
	{
		perror("pipe()");
		close(sd);
		return -4;
	}
	//创建子进程
	pid=fork();
	if(pid<0)
	{
		perror("fork()");
		close(fd[1]);
		close(fd[0]);
		close(sd);
		return -5;
	}
	//子进程操作调用播放器
	if(pid==0)
	{
		signal(SIGTSTP,SIG_IGN);
		close(fd[1]);
		dup2(fd[0],0);//将fd[0]作为stdin
	ERR_2:
		execl("/usr/bin/mplayer","mplayer","-",NULL);//替换进程，当子进程运行到这里 替换成mplayer播放器进行播放
		if(flag==0 || flag==1)
			goto ERR_2;
		close(fd[0]);
		exit(1);
	}
	//创建套接字
	sd=socket(AF_INET, SOCK_DGRAM, 0);
	if(sd==-1)
	{
		perror("socket()");
		return -1;
	}
	
	//本地地址与套接字绑定
	addr.sin_family=AF_INET;//指定IPV4协议
	addr.sin_port=htons(RECV_PORT);//本地字节序转换为网络字节序(转换结构PORT)
	inet_aton("0.0.0.0",&addr.sin_addr);//点分十进制字符串地址转换成地址结构体(转换本地IP)
	len=sizeof(addr);
	if(bind(sd,(struct sockaddr *)&addr,len)==-1)
	{
		perror("bind()");
		close(sd);
		return -2;
	}
	
	//加入多播组
	inet_aton("0.0.0.0",&imr.imr_address);//转换本地地址
	inet_aton(MULTICAST_ADDR,&imr.imr_multiaddr);//转换组播地址
	imr.imr_ifindex=if_nametoindex(NETCARD_NAME);//转换网卡名
	
	if(setsockopt(sd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&imr,sizeof(imr))==-1)
	{
		perror("setsockopt()");
		close(sd);
		return -3;
	}
	
	//父进程从网络上接收数据包通过管道传给子进程，子进程播放
	if(pid>0)
	{
ERR_1:
		flag=0;
		ret=0;
		chosenid=0;//频道选择
		/*从网络中接收数据，写入管道*/
		//接收
		signal(SIGTSTP,handler);
		struct chn_list_st *chn_list;//频道列表
		chn_list = malloc(200);//给频道列表分配存储空间
		if(chn_list == NULL)
		{
			perror("malloc()chn_list:");
			exit(1);
		}
		close(fd[0]);

		//setjmp(env);
        //频道选择
		flag=0;
		ret=0;
		chosenid=0;//频道选择
		//必须从频道列表开始
		while(1)
		{
			memset(chn_list, '\0', MSG_SIZE);
			count=recvfrom(sd,chn_list,sizeof(struct chn_data_st),0,(void*)&server_addr,&serveraddr_len);//recvfrom：从网络中读取数据包/接受多播组的数据 
			fprintf(stderr,"server_addr:%d\n",server_addr.sin_addr.s_addr);
			if(count < sizeof(struct chn_list_st))
			{//如果接收到的数据包小于频道列表的大小则错误
				fprintf(stderr,"massage is too short.\n");
			} 
			if(chn_list->chnid != CHN_LIST_ID)
			{//判断接收到的数据包是否为频道列表
				fprintf(stderr,"current chnid:%d.\n",chn_list->chnid);
				fprintf(stderr,"chnid is not match.\n");
				continue;
			}
			break;
		}  

		//输出，选择频道
		/*
		1.music
		2.
		3....
		*/
		//接收频道数据包，将其发送给子进程
		struct list_entry *pos;//单个频道的描述结构
		int i = 0;

		for(pos = chn_list->entry;(char *)pos<((char *)chn_list + count);pos = (void*)((char *)pos ) + ntohs(pos->len))
		{//循环打印每个频道
			printf("channel:%d %s",pos->chnid,pos->descr);
		}

		puts("Please input your choose:");
		while(ret < 1)
		{
			//输入要选择的频道ID
			ret=scanf("%d",&chosenid);
			/*memset(a,0,10);
			read(0,a,10);
			ret=atoi(a);
			chosenid=ret;*/
			if(ret != 1)
				exit(1);
		}
		struct chn_data_st chn_data;
		raddr_len = sizeof(raddr);
		char ipstr_raddr[30];//存放收到的地址
		char ipstr_server_addr[30];//存放服务端的地址
		int data_len = 0;

		//接收频道数据
		while(1)
		{
			if(flag==1)
			{
				goto ERR_1;
			}
			memset(chn_data.msg, '\0', MSG_SIZE);
			len = recvfrom(sd,&chn_data,MSG_SIZE+sizeof(chnid_t),0,(void*)&raddr,&raddr_len);//接受多播组的数据（频道数据）	
			if(chn_data.chnid == chosenid)
			{	
				write(fd[1],chn_data.msg,MSG_SIZE);
			}
		}
	}
		close(fd[1]);
		close(sd);
		wait(NULL);
	
		return 0;
}













