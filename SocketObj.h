#pragma once
#include "sys.h"
#include "messages.h"
#include <vector>
#include "common.h"
#include <queue>

/**********************************************************************
 *
 * Socket����
 *
 *********************************************************************/
#define SEND_WND_SIZE	16	//���ʹ��ڵĳߴ�
class Servant;
class Queue;

using namespace std;
typedef struct StatisticsTag
{
public :
	volatile LONG BytesRead,
              BytesSent,
              StartTime,
              BytesReadLast,
              BytesSentLast,
              StartTimeLast;
	volatile LONG AverageSentKBps,AverageReadKBps,CurrentSentKBps,CurrentReadKBps;

	//initialize 
	void Init()
	{
		BytesRead = BytesSent,BytesReadLast,BytesSentLast = 0;
		StartTimeLast = StartTime = GetTickCount();
		AverageSentKBps = 0,AverageReadKBps = 0,CurrentSentKBps = 0,CurrentReadKBps = 0;
	}

	//Calculate statistic data
	void Calculate()
	{
		ULONG   tick, elapsed;

		tick = GetTickCount();

		elapsed = (tick - StartTime) / 1000;

		if (elapsed == 0)
			return;
		
		//AverageSentKBps = BytesSent / elapsed /1024;
		InterlockedExchange(&AverageSentKBps,BytesSent / elapsed /1024);
		//printf("Average BPS sent: %lu [%lu]\n", bps, gBytesSent);

		//AverageReadKBps = BytesRead / elapsed /1024;
		InterlockedExchange(&AverageReadKBps,BytesRead / elapsed /1024);
		//printf("Average BPS read: %lu [%lu]\n", bps, gBytesRead);

		elapsed = (tick - StartTimeLast) / 1000;

		if (elapsed == 0)
			return;

		//CurrentSentKBps = BytesSentLast / elapsed /1024;
		InterlockedExchange(&CurrentSentKBps, BytesSentLast / elapsed /1024);
		//printf("Current BPS sent: %lu\n", bps);

		//CurrentReadKBps = BytesReadLast / elapsed /1024;
		InterlockedExchange(&CurrentReadKBps,BytesReadLast / elapsed /1024);
		//printf("Current BPS read: %lu\n", bps);

	    

		InterlockedExchange(&BytesSentLast, 0);
		InterlockedExchange(&BytesReadLast, 0);

		StartTimeLast = tick;
	}

} Statistics;

class SocketObj
{
public:
	SocketObj(bool);
	~SocketObj(void);

	SOCKET skt;
	Servant* lpServant;
	bool bIsListening ;
	ServantInfo* lpmCacheNode;
	sockaddr_in addr;
	int addrlen;
	bool bCanWrite;
	bool bIsConnected;
	bool bRunAsTCP;
	time_t tLastUpdate,tLastPing;
	DWORD dwLastSendTick;
	PEERID sid;
	DWORD dwChID;
	
	 WLock *lpSendLock ;//= new WLock();	
	 			//���ͻ�������
	 deque<MessageBase *> m_sendBuf; //���Ͷ���

	 Statistics pStatistics;
	 static Statistics gStatistics;
	 
	 SEGMENTID wSndSegID;		//���ڷ��͵�SegmentID�������ǰ����Ĳ���PushDataָ����ֵΪ0
private:
	//char *lpSndBuf;		//���ͻ��� 
	//int iSizeToSnd;		//�������ֽ�
	int iSndOffset;		//�ѷ����ֽ�
	int iSndID;			//���ڷ��͵���ϢID

#define iSizeOfRecvBuf (32 * 1024) 	//���ջ�������С���������������û���	
	char lpRecvBuf[iSizeOfRecvBuf];	//���ջ���		32kb

	int	iSizeToRecv;	//�������ֽ�
	int	iRecvOffset;	//�ѽ����ֽ�
	
	//���ڷ��͵���Ϣ
	MessageBase *lpMsg ;
	ReliableMessageBase *lpRlbMsg;
public:

	int SendTo();	//��������
	void SendTo(MessageBase *lpMsg);
	WORD _SeqID;
	inline WORD getNextSeqID()
	{
		
		if(0xffff == _SeqID)
		{
			_SeqID = 0;
		}
		return _SeqID++;
	};

	inline void SendTo_s()
	{
	
		lpSendLock->on();
		try
		{
			while (!m_sendBuf.empty() && bCanWrite)
			{
				if(0 == SendTo()) //WSAWOULDBLOKING
					break;
				Sleep(1);
			}
		}
		catch (SocketException* e)
		{
			lpSendLock->off();
			throw e;
		}
		
		lpSendLock->off();
	};

	void EraseMsg(WORD wSeqID);
	int Receive(Queue *lpQueue); //==0 must close
	void SyncID(SEGMENTID dwSegID);	//����Local Bufferʱִ�С����������޵������Ƴ���
	void ClearSendBuf()
	{
		lpSendLock->on();
		while (!m_sendBuf.empty())
		{
			delete m_sendBuf.front();
			m_sendBuf.pop_front();
		}
		lpSendLock->off();
	}
};
