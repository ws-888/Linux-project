#ifndef __MEDIA_LIB_H
#define __MEDIA_LIB_H

#include <stdint.h>

//[1]ָ�����Ƶ����
#define MAXCHN_NR		200
//[2]ָ��ý����ļ���·��(����·��)
#define MEDIA_LIB_PATH	"/home/ws/media"
//[3]ָ��Ƶ�������ļ�������
#define CHN_DESCR_NAME	"descr.txt"
//[4]ָ����ǰ��С��Ƶ��ID
#define MIN_CHN_ID		1

typedef uint8_t chnid_t;//ָ��Ƶ��ID����

//[1]����Ƶ���Ľṹ
struct mlib_list_entry
{
	chnid_t chnid;//Ƶ��ID
	char *descr;//Ƶ������ָ��ָ��Ƶ������
};

//[2]���ܽӿڶ���
/*
���� : ��ȡƵ���б�
��Ա : mlib �����ȡ����Ƶ���б�ĵ�ַ
		nmemb �����ȡ����Ƶ���б��Ա����
����ֵ : �ɹ�����0,ʧ�ܷ��� <0
*/
extern int mlib_get_chn_list(struct mlib_list_entry **mlib, int *nmemb);

/*
���� : ��ȡָ��Ƶ��������(���Ʒ�װread(2)����)
���� : chnid ָ����Ƶ��ID
		buf �洢����������
		size ָ���Ķ����ٸ��ֽ�
����ֵ : �ɹ����سɹ���ȡ�����ֽ���,ʧ�ܷ���<0
*/
extern int mlib_read_chn_data(chnid_t chnid, void *buf, size_t size);

#endif











