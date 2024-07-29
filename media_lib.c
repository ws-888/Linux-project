#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <glob.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "media_lib.h"

#define BUFSIZE 128

//定义一个结构体里面的成员包含解析目录想要得到的所有内容
struct chn_context_st
{
	struct mlib_list_entry list;//里面有chnid[频道号] 和 descr[频道描述]
	glob_t mp3_path;//管理音频文件的路径(gl_pathc gl_pathv)
	int fd;//当前频道中正在读的文件的文件描述符	
};

//定义了一个全局的指针数据,存储所有频道的结构信息
static struct chn_context_st *all_chns[MAXCHN_NR] = {};

//定义全局变量,记录全局指针数组中有多少个成员
static int chn_nr = 0;

static int __parse_media_lib(const char *path);//解析媒体库
static struct chn_context_st * __parse_chn(const char *path);//解析频道
static int __open_next(glob_t chn_file, int cur_index);//打开下一个音频文件

static int cur_idex[MAXCHN_NR] ;// 定义全局变量，记录当前频道正在打开mp3文件的序号

static int __parse_media_lib(const char *path)//解析媒体库
{
	DIR *dp = NULL;//目录流指针
	struct dirent *entry = NULL;//指向读到目录项结构
	char buf[BUFSIZE] = {0};//存储路径
	struct chn_context_st *ret = NULL;//接收频道解析的返回值

	dp = opendir(path);//打开目录
	if(dp == NULL)//判断打开目录是否失败
	{
		perror("opendir()");//打印错误信息
		return -1;//由于打开目录失败,结束函数,并且返回-1
	}

	while(1)
	{
		entry = readdir(dp);//读取目录
	 	if(entry == NULL)//判断是否读完了或者读失败了
			break;//跳出死循环
		if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;//略过. 和 ..
		memset(buf, '\0', BUFSIZE);//清空脏数据
		strcpy(buf, path);//把目录结构先拷贝到buf中
		strcat(buf, "/");//拼接目录下
		strcat(buf, entry->d_name);//拼接子文件名
		ret = __parse_chn(buf);//解析每一个频道
		if(ret == NULL)//判断解析频道是否失败
		{
			fprintf(stderr, "%s parse failed\n", buf);//打印错误信息
			continue;//如果某个频道解析失败,解析下一个频道
		}
		all_chns[ret->list.chnid] = ret;//把频道好作为全局数据的下标使用
		chn_nr++;//记录频道个数自增
	}
	closedir(dp);//关闭目录流

	return 0;
}

static struct chn_context_st * __parse_chn(const char *path)//解析频道
{
	//读path,例如 : "/home/zack/media/music"
	//有两类文件[1]频道描述文件 [2]xxxxx.mp3
	struct chn_context_st *me = malloc(sizeof(struct chn_context_st));
	//动态开辟,最终返回
	static chnid_t cur_id = MIN_CHN_ID;//使用静态局部变量记录编号
	char buf[BUFSIZE] = {0};//存储路径
	FILE *fp = NULL;//文件流指针
	size_t n = 0;//存储回填的大小
	int i = 0;//循环变量

	//[1]读频道描述文件
	me->list.chnid = cur_id;//把当前频道的频道号进行赋值
	memset(buf, '\0', BUFSIZE);//清空脏数据
	strcpy(buf, path);//把目录名先拷贝到buf中
	strcat(buf, "/");//拼接目录下
	strcat(buf, CHN_DESCR_NAME);//把频道描述文件拼接到路径中
	fp = fopen(buf, "r");//打开频道描述文件
	if(fp == NULL)//判断频道描述文件是否打开失败
	{
		fprintf(stderr, "%s have no descriptor\n", buf);//打印错误信息
		free(me);//释放me
		return NULL;//由于频道描述文件打开失败,结束函数,并且返回NULL
	}
	me->list.descr = NULL;//准备接收频道描述文件的信息
	if(getline(&(me->list.descr), &n, fp) == -1)//获取频道描述文件的一行信息
	{
		fprintf(stderr, "%s is empty descriptor\n", buf);//打印错误信息
		free(me);//释放me
		return NULL;//由于获取频道描述文件信息失败,结束函数,并且返回NULL
	}
	//[2]读音频文件(需要使用glob(3))
	memset(buf, '\0', BUFSIZE);//清空脏数据
	strcpy(buf, path);//把目录名先拷贝到buf中
	strcat(buf, "/*.mp3");//拼接目录下
	if(glob(buf, 0, NULL, &me->mp3_path) == GLOB_NOMATCH)
	//判断解析.mp3文件失败
	{
		fprintf(stderr, "mp3 file is failed\n");//打印错误信息
		free(me);//释放me
		return NULL;//由于解析.mp3文件失败,结束函数,并且返回NULL
	}
	//[3]打开音频文件
	i = -1;//文件描述符没有负的

	while(i < (int)me->mp3_path.gl_pathc) // int  在和 szie_t
	{
		if(i == -1)//第一次打开
		{
			me->fd = __open_next(me->mp3_path, i);
			cur_idex[me->list.chnid] = (i+1)%(me->mp3_path.gl_pathc);  	//记录当前频道打开文件的位置
		}
		else
		{
			close(me->fd);//关闭上一个音频文件
			me->fd = __open_next(me->mp3_path, i);
			cur_idex[me->list.chnid] = (i+1)%(me->mp3_path.gl_pathc);  //记录当前频道打开文件的位置
		}
		if(me->fd < 0)//判断打开音频文件是否失败
		{
			i++;
			fprintf(stderr, "%s open failed\n", (me->mp3_path.gl_pathv)[i]);
			continue;//继续打开下一个
		}
		break;
	}
	if(i == me->mp3_path.gl_pathc)
	{
		fprintf(stderr, "all mp3 open failed!\n");
		free(me);
		return NULL;
	}
	cur_id++;//频道号自增

	return me;
}

static int __open_next(glob_t chn_file, int cur_index)//打开下一个音频文件
{
	static int fd = 0;//文件描述

	cur_index = (cur_index + 1) % chn_file.gl_pathc;//循环播放
	fd = open(chn_file.gl_pathv[cur_index], O_RDONLY);//打开下一个频道

	return fd;
}

int mlib_get_chn_list(struct mlib_list_entry **mlib, int *nmemb)
{
	//获取频道结构
	int i = 0;//循环变量
	if(__parse_media_lib(MEDIA_LIB_PATH) == -1)//解析媒体库失败
	{
		fprintf(stderr, "media lib parse failed\n");//打印错误信息		
        return -1;
	}

	*mlib = calloc(chn_nr, sizeof(struct mlib_list_entry));//动态开辟频道结构
	*nmemb = chn_nr;//回填成员个数

	for(i = 0; i < chn_nr; i++)
	{
		(*mlib)[i] = all_chns[i + MIN_CHN_ID]->list;//回填
	}
}

//成功返回读取的字节数 错误返回-1 和read(2)功能几乎差不多 
int mlib_read_chn_data(chnid_t chnid, void *buf, size_t size)
{
	//读取频道数据
	//读all_chns[chnid]->fd->读到EOF->读下一个文件

    int count = 0;
    
    count = read(all_chns[chnid]->fd,buf,size);//读文件

    if(count == -1)//读出错，返回-1
	{
		close(all_chns[chnid]->fd);
		return -1;
	}

	if(count == 0)//读到文件结尾，i++读下一个
    {
        close(all_chns[chnid]->fd);	 //关闭文件
        all_chns[chnid]->fd = __open_next(all_chns[chnid]->mp3_path, cur_idex[chnid]);//打开下一个文件
        cur_idex[chnid]=(cur_idex[chnid]+1) % (all_chns[chnid]->mp3_path.gl_pathc); //记录当前频道打开文件的序号
    }
	
    return count;
}














