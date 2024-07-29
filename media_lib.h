#ifndef __MEDIA_LIB_H
#define __MEDIA_LIB_H

#include <stdint.h>

//[1]指定最多频道数
#define MAXCHN_NR		200
//[2]指定媒体库文件的路径(绝对路径)
#define MEDIA_LIB_PATH	"/home/ws/media"
//[3]指定频道描述文件的名字
#define CHN_DESCR_NAME	"descr.txt"
//[4]指定当前最小的频道ID
#define MIN_CHN_ID		1

typedef uint8_t chnid_t;//指定频道ID类型

//[1]单个频道的结构
struct mlib_list_entry
{
	chnid_t chnid;//频道ID
	char *descr;//频道描述指针指向频道描述
};

//[2]功能接口定义
/*
功能 : 获取频道列表
成员 : mlib 回填获取到的频道列表的地址
		nmemb 回填获取到的频道列表成员个数
返回值 : 成功返回0,失败返回 <0
*/
extern int mlib_get_chn_list(struct mlib_list_entry **mlib, int *nmemb);

/*
功能 : 读取指定频道的数据(类似封装read(2)函数)
参数 : chnid 指定的频道ID
		buf 存储读到的数据
		size 指定的读多少个字节
返回值 : 成功返回成功读取到的字节数,失败返回<0
*/
extern int mlib_read_chn_data(chnid_t chnid, void *buf, size_t size);

#endif











