#pragma once
#include "sys.h"
/*

#define CMD_UNKNOWN				0x0000	//δ֪
#define CMD_CONNECT				0x0001	//����ָ��
#define CMD_FORWARD				0x0002	//ǰתָ��
#define CMD_PUSH_HEADER			0x0003	//����ͷ
#define CMD_PUSH_SERVANTS		0x0004	//����Servants
#define	CMD_LEAVE				0x0005	//�뿪
#define	CMD_ACTIVE				0x0006	//����
#define CMD_APPLY_DATAS			0x0007	//��������
#define	CMD_PUSH_DATA			0x0008	//��������
#define CMD_REJECT_DATA			0x0009	//�ܾ�����
#define	CMD_CHANGE_MEDIA		0x000A	//ý��ı�	---Ex1�в�֧��


#define CMD_CONNECT_R			0x8001	//����ָ��
#define CMD_FORWARD_R			0x8002	//ǰתָ��
#define CMD_PUSH_HEADER_R		0x8003	//����ͷ
#define CMD_PUSH_SERVANTS_R		0x8004	//����Servants
#define	CMD_LEAVE_R				0x8005	//�뿪
#define	CMD_ACTIVE_R			0x8006	//����
#define CMD_APPLY_DATAS_R		0x8007	//��������
#define	CMD_PUSH_DATA_R			0x8008	//��������
#define CMD_REJECT_DATA_R		0x8009	//�ܾ�����
#define	CMD_CHANGE_MEDIA_R		0x800A	//ý��ı�

typedef struct _CMD_TAG
{
	WORD wCmd;	//Cmd
	WORD wSeq;	//Sequence
	char* buf;	
	WORD buflen;
	sockaddr_in addr;// only for servant manager
} CMD;
enum ConnectResult
{
	Connect_Result_OK ,
	Content_Result_ReDirection ,
	Content_Result_Require_New_Version
};
//Э����
namespace Protocol
{
	
	inline WORD getSeq()
	{
		static WORD _seq =0;
		
		return _seq ++;
		
	};

	CMD getConnect(DWORD sid,const char *ChID);
	//��ȡ���ӵķ�������Result ==Connect_Result_ReDirectionʱ��ip��port������Ч��Result == Connect_Result_Require_New_Versionʱ,ver������Ч
	CMD getConnectR(WORD wSeq,ConnectResult Result,DWORD ip = 0,WORD port =0);

	void parseConnect(const char* buf,int buflen,char** ip,WORD *port,char **ChID);

};*/