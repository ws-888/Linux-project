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

struct sockaddr_in remote_addr;//�Զ˵�ַ

struct ip_mreqn imr;//�鲥ѡ��ṹ��

struct mlib_list_entry *mylib;//�����ȡ����Ƶ���б�����

struct chn_list_st *list_data; //

int size1;

int sd;

static void *task1(void *s); // ����Ƶ���б�
static void *task2(void *s); // ��������
int chn_list_init(struct chn_list_st **list_data, int n, const struct mlib_list_entry *mylib);

int main(void)
{
    int n;//�����ȡ����Ƶ������
    int i = 0;//ѭ������
    pool_t *p = NULL;//�����̳߳����͵�ָ��
    task_t t;//��������ṹ�����
	/*�ػ�����*/
/*	if(daemon(0, 0) < 0)
    { 
        perror("daemon()");//��ӡ������Ϣ
        return -1;
    }
	
*/
    mlib_get_chn_list(&mylib, &n);//��ȡƵ���б�
    pool_init(&p, 1); //�̳߳س�ʼ��

	/************�鲥**********************/
    sd = socket(AF_INET, SOCK_DGRAM, 0);//����UDP�׽���
    if(sd == -1)//�жϴ���UDP�׽����Ƿ�ʧ��
    {
        perror("socket()");//��ӡ������Ϣ
        return -3;//���ڴ���UDP�׽���ʧ��,��������,���ҷ���-3
    }
    inet_aton(MULTICAST_ADDR, &imr.imr_multiaddr);//ת���鲥��ַ
    inet_aton(LOCAL_IP, &imr.imr_address);//ת�����ص�ַ
    imr.imr_ifindex = if_nametoindex(NETCARD_NAME);//ת��������
    if(setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, \
                &imr, sizeof(imr)) == -1)//�ж϶ಥ��ʹ���Ƿ�ʧ��
    {
        perror("setsockopt()");//��ӡ������Ϣ
        close(sd);//�ر��׽���
        return -4;//���ڶಥ��ʹ��ʧ��,��������,���ҷ���-4
    }
    remote_addr.sin_family = AF_INET;//ָ��IPV4Э��
    inet_aton(MULTICAST_ADDR, &remote_addr.sin_addr);//����ಥ���ַ
    remote_addr.sin_port = htons(RECV_PORT);//����˿ں�
	/*************�鲥end********************/
	
    for(i = 0; i < n; i++)//
    {
        printf("[%d]    %s\n", mylib[i].chnid, mylib[i].descr);
    }
	/********������Ƶ���б����ݼ��뵽�̳߳�******************************/
	t.job = task1;//ָ������
	t.arg = NULL;//ָ������Ĳ���
	pool_add_task(p, &t);//�������
	/*******************************************************************/
	chn_list_init(&list_data,n,mylib);//��Ƶ���б�浽list_data �ռ���

//	write(1,list_data,size1);
	printf("-------list---------\n");
	struct list_entry *pos;
	for(i=0,pos = list_data->entry;i<n;pos = (void*)((char *)pos) + ntohs(pos->len),i++)
	{
		printf("channel:%d %s", pos->chnid, pos->descr);
	}
	
	/********������Ƶ�����ݵ�������뵽�̳߳�*****************/
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

//����Ƶ������
static void *task2(void *s)
{
	int n1 = (int)s;//�洢Ƶ����
    int count = 0;
	struct chn_data_st send_msg;  //Ƶ�����ݽṹ��
	send_msg.chnid = n1; 
    while(1)
    {
		memset(send_msg.msg,0,MSG_SIZE);
        count = mlib_read_chn_data(n1, send_msg.msg,MSG_SIZE); // ��Ƶ�������ļ�
//		printf("send====send_msg.chnid=%d\n",send_msg.chnid); //����
		//�鲥
		if(count == 0)
			continue;
        sendto(sd, &send_msg,sizeof(chnid_t)+count, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
//		sleep(1);
		usleep(2500);
	}
}

//����Ƶ���б� ÿ1s����һ��
static void *task1(void *s)
{
	int n = (int)s;
	int i = 0;
	//union chn_recv_st send_list_data;
//	memcpy(&send_list_data,list_data,size1);
	while(1)
    {//size1 ȫ�ֱ��� �洢list_data��С
		sendto(sd,list_data ,size1, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));

		sleep(1);
	}
}

//��Ƶ���б�浽list_data�ռ�
int chn_list_init(struct chn_list_st **list_data,int n,const struct mlib_list_entry *mlib)
{
	int i=0;
	size_t* mem=malloc(sizeof(size_t)*n);//�ļ������ĳ���
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
