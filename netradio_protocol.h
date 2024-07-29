 #ifndef __NETRADUI_PROTOCOL_H
 #define __NETRADUI_PROTOCOL_H
 
 #include <stdint.h>
 
 //组播地址
 #define MULTICAST_ADDR "234.3.5.4"
 //接收端口号
 #define RECV_PORT 9529
 //最大支持频道个数
 #define CHN_NR 200
 //频道列表编号
 #define CHN_LIST_ID 0
 //最小频道号
 #define MIN_CHN_ID 1
 //最大频道号
 #define MAX_CHN_ID CHN_NR + MIN_CHN_ID - 1
 //数据包的大小
 #define MSG_SIZE 128
 
 #define LOCAL_IP        "0.0.0.0"
 
 #define NETCARD_NAME    "ens33"
 
 typedef uint8_t chnid_t;//定义频道ID类型
 typedef uint16_t len_t;//定义长度的类型
 
 /*
 频道列表
 0 频道列表编号
 ID 频道描述
 1 music
 因为不知道要有多少个频道
 所以先抽象单个频道的描述结构
 再去抽象整个频道列表的结构
 */
 //单个频道的描述结构
 //因为不知道频道描述有多长所以使用可边长的结构体
 
 struct list_entry
 {
     chnid_t chnid;//表示频道ID
     len_t len;//自述结构的长度
     char descr[0];//表示频道的描述
 //	 char *descr;
 }__attribute__((packed));//跨主机要单字节对齐
 
 //整个频道列表结构
 //因为不知道有多少个频道所以使用可变长的结构体
 struct chn_list_st
 {
     chnid_t chnid;//表示频道列表编(为0)
     struct list_entry entry[0];//表示频道
 }__attribute__((packed));//跨主机要单字节对齐
 
 //频道数据结构
 struct chn_data_st
 {
     chnid_t chnid;//表示频道数据来源于哪个频道
     char msg[MSG_SIZE];//频道数据
 }__attribute__((packed));//跨主机要单字节对齐
 
 union chn_recv_st
 {
     chnid_t chnid;//区分当前这一包数据是频道列表(=0)还是频道数据(>0)
     struct chn_list_st list;//频道列表结构
     struct chn_data_st data;//频道数据结构
 };
 
 
 
 
 
 #endif

