#pragma once
#include "sys.h"
#include <time.h>

#define CMD_UNKNOWN				0x0000	//δ֪
#define CMD_JOIN				0x0001	//����ָ��
//#define CMD_FORWARD				0x0002	//ǰתָ��			'Forwardָ��������?
#define CMD_PUSH_HEADER			0x0003	//����ͷ
#define CMD_PUSH_SERVANTS		0x0004	//����Servants  
#define	CMD_LEAVE				0x0005	//�뿪
#define	CMD_EXCHANGE_BM			0x0006	//BufferMap������Ϣ
#define CMD_APPLY_DATAS			0x0007	//��������
#define	CMD_PUSH_DATA			0x0008	//��������
#define CMD_REJECT_DATA			0x0009	//�ܾ�����	---Ex1�в�֧��
#define	CMD_CHANGE_MEDIA		0x000A	//ý��ı�	---Ex1�в�֧��
#define CMD_PING				0x000B	//Ping	���ڸ���mCache			'Ҫ��Ҫ��UDPЭ��?
#define CMD_CONNECT				0x000C	//��������

#define CMD_APPLY_PACKET		0x000E	//�����ض������ݰ�


#define CMD_JOIN_R				0x8001	//����ָ��
//#define CMD_FORWARD_R			0x8002	//ǰתָ��
#define CMD_PUSH_HEADER_R		0x8003	//����ͷ
#define CMD_PUSH_SERVANTS_R		0x8004	//����Servants
#define	CMD_LEAVE_R				0x8005	//�뿪
#define	CMD_EXCHANGE_BM_R		0x8006	//����
#define CMD_APPLY_DATAS_R		0x8007	//��������
#define	CMD_PUSH_DATA_R			0x8008	//��������
#define CMD_REJECT_DATA_R		0x8009	//�ܾ�����
#define	CMD_CHANGE_MEDIA_R		0x800A	//ý��ı�
#define CMD_CONNECT_R			0x800C	//������Ӧ
//



//////////////////////////////////////////////////////////////////////////
//Proxy Protocol
#define PXY_UNKNOWN				0x0000	//δ֪
#define PXY_HELLO				0x0001	//Shakehands
//#define PXY_PING				0x0002	//����
#define PXY_CALLBACK			0x0003	//call back request
#define PXY_LEAVE				0x0004	
#define CMD_SYNC_ID				0x000D	//ͬ��SegmentID					'ͨ��UDP�����ٷ�����ѹ��

#define PXY_HELLO_R				0x8001	//Shakehands
//#define PXY_PING_R				0x8002	//����
#define PXY_CALLBACK_R			0x8003	//call back request
#define CMD_SYNC_ID_R			0x800D	//ͬ��SegmentID����Ӧ


#define VERSION 0x0100



////////////////////////////////////////////////////////////////////////////////////////////////////

enum NetworkType
{
	NetType_Public,NetType_Private
};

class SocketObj;
////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ServantInfo_TAG
{
	//�˽ڵ������ַ
	sockaddr_in addr;
	DWORD dwSID;
	//WORD wSeqID;
	//time_t tLastUpdate,tLastPing; move to SocketObj
	SocketObj *lpSktObj ;
	bool bIsMySelf;
	volatile DWORD dwRecvCount;			//���յ���Segment����
	volatile DWORD dwSentCount;			//���ͳɹ���Segment����

	ServantInfo_TAG()
	{
		dwSID = 0;
		//wSeqID = 0;
		lpSktObj = NULL;
		bIsMySelf = false;
		dwRecvCount = dwSentCount =0;
	}
public:
	//����=�����
	bool operator =(const  ServantInfo_TAG &p) const
	{
		return dwSID == p.dwSID;
	};

	//����<�����
	bool operator <(const  ServantInfo_TAG &p) const
	{
		return max(dwRecvCount,dwSentCount) < max(p.dwRecvCount,p.dwSentCount) ;
	};

} ServantInfo;


////////////////////////////////////////////////////////////////////////////////////////////////////

class MessageBase
{
public:

	MessageBase(void)
		:wCMD(CMD_UNKNOWN)
		,wSeqID(0)

	{
		lpBuf = NULL;
		bNeedConfirm=false;

		bIsControl = false;
		dwChID = 0;
	};

	
	MessageBase(WORD cmd)
		:wCMD(cmd)
		,wSeqID(0)

	{
		lpBuf = NULL;
		bNeedConfirm=false;
		
		bIsControl = false;
		dwChID = 0;
	};

	MessageBase(const char* p,const int len)		
	{
		fromBytes(p,len);
		lpBuf = NULL;
		bNeedConfirm=false;
		
		bIsControl = false;
		dwChID = 0;
	};
	virtual ~MessageBase(void)
	{
		if(NULL != lpBuf)
			delete[] lpBuf;
		
	};

private:
	static WLock *m_slock;
	static WORD	m_sSeqID;
public:
	WORD wCMD;		//������
	WORD wSeqID;	//���к�
	DWORD dwChID;	//Ƶ��ID	
	
	char *lpBuf;		//Buf
	int	 bufLen;		//BufLen
	

	bool bNeedConfirm ; //�Ƿ���Ҫ����ȷ����Ϣ
	bool bIsControl;


	//���ֽ�����ָ�����
	virtual void fromBytes(const char* p,const int len) = 0;

	//������ת��Ϊ�ֽ�����
	virtual void toBytes(void) =0;

};

/************************************************************************/
/* Reliable Message Base.                                               */
/************************************************************************/
//struct rtt_info;

class ReliableMessageBase :
	public MessageBase
{
public:
	DWORD dwRetryTime;
	time_t	tSendTime;

public:
	ReliableMessageBase(void):MessageBase()
	{
		bNeedConfirm =  true;
		dwRetryTime = 0;
			
	};

	ReliableMessageBase(WORD cmd):MessageBase(cmd)
	{
		bNeedConfirm = true;
		dwRetryTime = 0;
	};

	ReliableMessageBase(const char* p,const int len):MessageBase(p,len)
	{
		bNeedConfirm = true;
		dwRetryTime = 0;
	};

	virtual ~ReliableMessageBase(void)
	{
	};

	//���ֽ�����ָ�����
	virtual void fromBytes(const char* p,const int len) = 0;

	//������ת��Ϊ�ֽ�����
	virtual void toBytes(void) =0;

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ACKMessageBase :
	public MessageBase
{

public:
	DWORD dwRetryTime;
	time_t	tSendTime;
public:
	ACKMessageBase(void):MessageBase()
	{
		bNeedConfirm =  false;
		dwRetryTime = 0;
	};

	ACKMessageBase(WORD cmd):MessageBase(cmd)
	{
		bNeedConfirm = false;
		dwRetryTime = 0;
	};

	ACKMessageBase(const char* p,const int len):MessageBase(p,len)
	{
		bNeedConfirm = false;
		dwRetryTime = 0;
	};

	virtual ~ACKMessageBase(void)
	{
	};

		//���ֽ�����ָ�����
	virtual void fromBytes(const char* p,const int len) = 0;

	//������ת��Ϊ�ֽ�����
	virtual void toBytes(void) =0;

};
