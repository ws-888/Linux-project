 #ifndef __NETRADUI_PROTOCOL_H
 #define __NETRADUI_PROTOCOL_H
 
 #include <stdint.h>
 
 //�鲥��ַ
 #define MULTICAST_ADDR "234.3.5.4"
 //���ն˿ں�
 #define RECV_PORT 9529
 //���֧��Ƶ������
 #define CHN_NR 200
 //Ƶ���б���
 #define CHN_LIST_ID 0
 //��СƵ����
 #define MIN_CHN_ID 1
 //���Ƶ����
 #define MAX_CHN_ID CHN_NR + MIN_CHN_ID - 1
 //���ݰ��Ĵ�С
 #define MSG_SIZE 128
 
 #define LOCAL_IP        "0.0.0.0"
 
 #define NETCARD_NAME    "ens33"
 
 typedef uint8_t chnid_t;//����Ƶ��ID����
 typedef uint16_t len_t;//���峤�ȵ�����
 
 /*
 Ƶ���б�
 0 Ƶ���б���
 ID Ƶ������
 1 music
 ��Ϊ��֪��Ҫ�ж��ٸ�Ƶ��
 �����ȳ��󵥸�Ƶ���������ṹ
 ��ȥ��������Ƶ���б�Ľṹ
 */
 //����Ƶ���������ṹ
 //��Ϊ��֪��Ƶ�������ж೤����ʹ�ÿɱ߳��Ľṹ��
 
 struct list_entry
 {
     chnid_t chnid;//��ʾƵ��ID
     len_t len;//�����ṹ�ĳ���
     char descr[0];//��ʾƵ��������
 //	 char *descr;
 }__attribute__((packed));//������Ҫ���ֽڶ���
 
 //����Ƶ���б�ṹ
 //��Ϊ��֪���ж��ٸ�Ƶ������ʹ�ÿɱ䳤�Ľṹ��
 struct chn_list_st
 {
     chnid_t chnid;//��ʾƵ���б��(Ϊ0)
     struct list_entry entry[0];//��ʾƵ��
 }__attribute__((packed));//������Ҫ���ֽڶ���
 
 //Ƶ�����ݽṹ
 struct chn_data_st
 {
     chnid_t chnid;//��ʾƵ��������Դ���ĸ�Ƶ��
     char msg[MSG_SIZE];//Ƶ������
 }__attribute__((packed));//������Ҫ���ֽڶ���
 
 union chn_recv_st
 {
     chnid_t chnid;//���ֵ�ǰ��һ��������Ƶ���б�(=0)����Ƶ������(>0)
     struct chn_list_st list;//Ƶ���б�ṹ
     struct chn_data_st data;//Ƶ�����ݽṹ
 };
 
 
 
 
 
 #endif

