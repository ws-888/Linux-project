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

//����һ���ṹ������ĳ�Ա��������Ŀ¼��Ҫ�õ�����������
struct chn_context_st
{
	struct mlib_list_entry list;//������chnid[Ƶ����] �� descr[Ƶ������]
	glob_t mp3_path;//������Ƶ�ļ���·��(gl_pathc gl_pathv)
	int fd;//��ǰƵ�������ڶ����ļ����ļ�������	
};

//������һ��ȫ�ֵ�ָ������,�洢����Ƶ���Ľṹ��Ϣ
static struct chn_context_st *all_chns[MAXCHN_NR] = {};

//����ȫ�ֱ���,��¼ȫ��ָ���������ж��ٸ���Ա
static int chn_nr = 0;

static int __parse_media_lib(const char *path);//����ý���
static struct chn_context_st * __parse_chn(const char *path);//����Ƶ��
static int __open_next(glob_t chn_file, int cur_index);//����һ����Ƶ�ļ�

static int cur_idex[MAXCHN_NR] ;// ����ȫ�ֱ�������¼��ǰƵ�����ڴ�mp3�ļ������

static int __parse_media_lib(const char *path)//����ý���
{
	DIR *dp = NULL;//Ŀ¼��ָ��
	struct dirent *entry = NULL;//ָ�����Ŀ¼��ṹ
	char buf[BUFSIZE] = {0};//�洢·��
	struct chn_context_st *ret = NULL;//����Ƶ�������ķ���ֵ

	dp = opendir(path);//��Ŀ¼
	if(dp == NULL)//�жϴ�Ŀ¼�Ƿ�ʧ��
	{
		perror("opendir()");//��ӡ������Ϣ
		return -1;//���ڴ�Ŀ¼ʧ��,��������,���ҷ���-1
	}

	while(1)
	{
		entry = readdir(dp);//��ȡĿ¼
	 	if(entry == NULL)//�ж��Ƿ�����˻��߶�ʧ����
			break;//������ѭ��
		if(!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;//�Թ�. �� ..
		memset(buf, '\0', BUFSIZE);//���������
		strcpy(buf, path);//��Ŀ¼�ṹ�ȿ�����buf��
		strcat(buf, "/");//ƴ��Ŀ¼��
		strcat(buf, entry->d_name);//ƴ�����ļ���
		ret = __parse_chn(buf);//����ÿһ��Ƶ��
		if(ret == NULL)//�жϽ���Ƶ���Ƿ�ʧ��
		{
			fprintf(stderr, "%s parse failed\n", buf);//��ӡ������Ϣ
			continue;//���ĳ��Ƶ������ʧ��,������һ��Ƶ��
		}
		all_chns[ret->list.chnid] = ret;//��Ƶ������Ϊȫ�����ݵ��±�ʹ��
		chn_nr++;//��¼Ƶ����������
	}
	closedir(dp);//�ر�Ŀ¼��

	return 0;
}

static struct chn_context_st * __parse_chn(const char *path)//����Ƶ��
{
	//��path,���� : "/home/zack/media/music"
	//�������ļ�[1]Ƶ�������ļ� [2]xxxxx.mp3
	struct chn_context_st *me = malloc(sizeof(struct chn_context_st));
	//��̬����,���շ���
	static chnid_t cur_id = MIN_CHN_ID;//ʹ�þ�̬�ֲ�������¼���
	char buf[BUFSIZE] = {0};//�洢·��
	FILE *fp = NULL;//�ļ���ָ��
	size_t n = 0;//�洢����Ĵ�С
	int i = 0;//ѭ������

	//[1]��Ƶ�������ļ�
	me->list.chnid = cur_id;//�ѵ�ǰƵ����Ƶ���Ž��и�ֵ
	memset(buf, '\0', BUFSIZE);//���������
	strcpy(buf, path);//��Ŀ¼���ȿ�����buf��
	strcat(buf, "/");//ƴ��Ŀ¼��
	strcat(buf, CHN_DESCR_NAME);//��Ƶ�������ļ�ƴ�ӵ�·����
	fp = fopen(buf, "r");//��Ƶ�������ļ�
	if(fp == NULL)//�ж�Ƶ�������ļ��Ƿ��ʧ��
	{
		fprintf(stderr, "%s have no descriptor\n", buf);//��ӡ������Ϣ
		free(me);//�ͷ�me
		return NULL;//����Ƶ�������ļ���ʧ��,��������,���ҷ���NULL
	}
	me->list.descr = NULL;//׼������Ƶ�������ļ�����Ϣ
	if(getline(&(me->list.descr), &n, fp) == -1)//��ȡƵ�������ļ���һ����Ϣ
	{
		fprintf(stderr, "%s is empty descriptor\n", buf);//��ӡ������Ϣ
		free(me);//�ͷ�me
		return NULL;//���ڻ�ȡƵ�������ļ���Ϣʧ��,��������,���ҷ���NULL
	}
	//[2]����Ƶ�ļ�(��Ҫʹ��glob(3))
	memset(buf, '\0', BUFSIZE);//���������
	strcpy(buf, path);//��Ŀ¼���ȿ�����buf��
	strcat(buf, "/*.mp3");//ƴ��Ŀ¼��
	if(glob(buf, 0, NULL, &me->mp3_path) == GLOB_NOMATCH)
	//�жϽ���.mp3�ļ�ʧ��
	{
		fprintf(stderr, "mp3 file is failed\n");//��ӡ������Ϣ
		free(me);//�ͷ�me
		return NULL;//���ڽ���.mp3�ļ�ʧ��,��������,���ҷ���NULL
	}
	//[3]����Ƶ�ļ�
	i = -1;//�ļ�������û�и���

	while(i < (int)me->mp3_path.gl_pathc) // int  �ں� szie_t
	{
		if(i == -1)//��һ�δ�
		{
			me->fd = __open_next(me->mp3_path, i);
			cur_idex[me->list.chnid] = (i+1)%(me->mp3_path.gl_pathc);  	//��¼��ǰƵ�����ļ���λ��
		}
		else
		{
			close(me->fd);//�ر���һ����Ƶ�ļ�
			me->fd = __open_next(me->mp3_path, i);
			cur_idex[me->list.chnid] = (i+1)%(me->mp3_path.gl_pathc);  //��¼��ǰƵ�����ļ���λ��
		}
		if(me->fd < 0)//�жϴ���Ƶ�ļ��Ƿ�ʧ��
		{
			i++;
			fprintf(stderr, "%s open failed\n", (me->mp3_path.gl_pathv)[i]);
			continue;//��������һ��
		}
		break;
	}
	if(i == me->mp3_path.gl_pathc)
	{
		fprintf(stderr, "all mp3 open failed!\n");
		free(me);
		return NULL;
	}
	cur_id++;//Ƶ��������

	return me;
}

static int __open_next(glob_t chn_file, int cur_index)//����һ����Ƶ�ļ�
{
	static int fd = 0;//�ļ�����

	cur_index = (cur_index + 1) % chn_file.gl_pathc;//ѭ������
	fd = open(chn_file.gl_pathv[cur_index], O_RDONLY);//����һ��Ƶ��

	return fd;
}

int mlib_get_chn_list(struct mlib_list_entry **mlib, int *nmemb)
{
	//��ȡƵ���ṹ
	int i = 0;//ѭ������
	if(__parse_media_lib(MEDIA_LIB_PATH) == -1)//����ý���ʧ��
	{
		fprintf(stderr, "media lib parse failed\n");//��ӡ������Ϣ		
        return -1;
	}

	*mlib = calloc(chn_nr, sizeof(struct mlib_list_entry));//��̬����Ƶ���ṹ
	*nmemb = chn_nr;//�����Ա����

	for(i = 0; i < chn_nr; i++)
	{
		(*mlib)[i] = all_chns[i + MIN_CHN_ID]->list;//����
	}
}

//�ɹ����ض�ȡ���ֽ��� ���󷵻�-1 ��read(2)���ܼ������ 
int mlib_read_chn_data(chnid_t chnid, void *buf, size_t size)
{
	//��ȡƵ������
	//��all_chns[chnid]->fd->����EOF->����һ���ļ�

    int count = 0;
    
    count = read(all_chns[chnid]->fd,buf,size);//���ļ�

    if(count == -1)//����������-1
	{
		close(all_chns[chnid]->fd);
		return -1;
	}

	if(count == 0)//�����ļ���β��i++����һ��
    {
        close(all_chns[chnid]->fd);	 //�ر��ļ�
        all_chns[chnid]->fd = __open_next(all_chns[chnid]->mp3_path, cur_idex[chnid]);//����һ���ļ�
        cur_idex[chnid]=(cur_idex[chnid]+1) % (all_chns[chnid]->mp3_path.gl_pathc); //��¼��ǰƵ�����ļ������
    }
	
    return count;
}














