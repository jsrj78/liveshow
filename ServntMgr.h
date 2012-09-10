/*
	
	һЩ�ؼ��㷨˵��

	1����Ϊ�ͻ��ˣ�ÿ��20�뷢��һ��SyncID��Ϣ�����յ�SyncID Resp�󣬼�飬�������˵�ID�뱾��ID�Ĳ��60%��
�򽫱��ص���Ϣͬ��Ϊ�������ϵ�ID
	2��TimeStep���̼�����������Ŀ��ÿռ䣬�ڿ��ÿռ�С��10%���ͷŵ�һ��Segment��Ϊ��֤�����ԣ�TimeStep���̿���ÿ��ִ��һ��
	3�������յ�15��Segment(15��)�����������������б��ز���
*/

#pragma once
#include "Buffer.h"
#include "Protocol.h"
#include "Servant.h"
#include "messages.h"
#include <vector>
#include <list>
#include "sys.h"
#include "Queue.h"
#include "FileDeliver.h"
#include "HttpDeliver.h"
#include "SourcePin.h"

#define MAX_SUPPLIER 4	//ÿ��Segment�����ṩ��
//#define PING_COUNT	4	//ͬʱ����ٸ��ͻ�������Ping��Ϣ
#define TTL		6		//TTL
#define HOPS	0		//hops
#define MAX_NODE_LIVE	1*60	//mCache�����Ĵ��ʱ�䣬��λ ��
#define	MAX_SEND_CONNECT 5	//����͵�Connect��Ϣ
#define SCHEDULER_TIMER	199	//������ִ��ʱ��������λ ����
#define MCACHE_TIMER	9 * 1000	//mCache_Procִ�м��ʱ��
#define EXBM_TIMER		6 * 1000	//Exchange BM Proc
#define SYNC_ID_TIMER	25 * 1000 //ͬ��ID����ִ��ʱ��
#define MAX_LOWID		0x1000000 //����LowID
#define IsHighID(id)	(id > MAX_LOWID) //�Ƿ���HighID
#define IsLowID(id)		(id <= MAX_LOWID)
#define PRX_HELLO_TIMMER 10 * 1000 //��ProxyServer����Hello��ʱ����

#define MAX_MCACHE_COUNT	120	//����mcache����
#define MAX_SERVANT_COUNT	32	//����servant����
/**********************************************************************
 *
 * �̶߳���
 *
 *********************************************************************/

class ServntMgr;

class  ThreadData
{
	friend class ServntMgr;
public:
	ThreadData(ServntMgr *lpmgr)
	{
		m_Lock = new WLock();
		lpQueue = new Queue();

		lpMgr = lpmgr;
	}
	~ThreadData()
	{
		delete m_Lock;
		delete lpQueue;
	}

public:

	WLock *m_Lock;
	ServntMgr *lpMgr;
	vector<SocketObj*> vtSocket; //�߳���SocketObjʵ���б�
	Queue	*lpQueue;				//������Ϣ����
	bool bRunAsTCP;


	bool Append(SOCKET skt,bool isListening,SocketObj **lpSktObj)
	{
		if(vtSocket.size() == FD_SETSIZE )
		{
			return false;
		}
		m_Lock->on();
		
		SocketObj *lpObj = new SocketObj(bRunAsTCP);
		lpObj->bIsListening = isListening;
		lpObj->lpServant= NULL;
		lpObj->skt = skt;
		lpObj->lpmCacheNode = NULL;

		vtSocket.push_back(lpObj);

		*lpSktObj = lpObj;

		m_Lock->off();
		return true;
	}

	bool Append(SOCKET skt,SocketObj **lpSktObj)
	{
		return Append(skt,false,lpSktObj);
	}
	
	bool Remove(SocketObj *lpSktObj)
	{
		//remove from vtSocket
		bool bFound = false;
		m_Lock->on();
		for(vector<SocketObj *>::iterator it2 = vtSocket.begin(); it2 !=vtSocket.end() ; it2++)
		{
			if((*it2) == lpSktObj)
			{
				//found
				vtSocket.erase(it2);
				bFound = true;
				break;
			}
		}
		m_Lock->off();
		return bFound;
	}
	

	SocketObj* Find(PEERID _sid)
	{
		SocketObj *lpRet = NULL;
		m_Lock->on();
		for(vector<SocketObj*>::iterator it = vtSocket.begin() ; it != vtSocket.end() ;it++)
		{
			
			if((*it)->sid == _sid && (*it)!= 0)
			{
				lpRet = *it;
				break;
			}
		}
		m_Lock->off();
		return lpRet;
	}
} ;


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ServntMgr
{
public:
	ServntMgr(void);
	~ServntMgr(void);

	//��������Ϊ�ͻ���
	void Start(char* szIP,WORD wPort,DWORD ChID);
	//��������Ϊ������
	void srv_Start(DWORD ChID);
	//ֹͣ
	void Stop(bool bIsRestart = false,int ErrorCode = 40);
	PEERID dwSID;						//���˵�ID
	PEERID SrvSID;						//�������˵�SID
	//Ƶ��ID
	DWORD dwChID;
	
	sockaddr_in	localAddr,serverAddr;	//���ص�ip��ַ
	/************************************************************************/
	/* ״̬�ӿ� Level:״̬����1:General information;2:Error;3:trigger Event*/
	/************************************************************************/
	 void (__stdcall * StatusPin)(int StatusCode,int Detail);
private:

	vector<ServantInfo*> mCache;			//���븲�����ľֲ���Ϣ
	WLock *lpCacheLock;						//mCache

	bool	bIsRunning	;				//�Ƿ�������
	Buffer *lpBuf;						//��Buffer����
	//vector<ThreadInfo> m_Threads;		//�̳߳�
	WLock *lpThreadsLock;				//�̳߳���
	bool	bIsSrv;						//�Ƿ���Ϊ���������
	bool	bRunAsTCP;					//�Ƿ���TCP����
	//SocketObj *lpSrvSkt;				//����Server��Socket����ֻ�������ڿͻ���ģʽʱ�����ж���
	vector<DWORD> vtLocalIps;			//����IP��ַ

	/************************************************************************/
	/*                                                             */
	/************************************************************************/
	SourcePin *lpSrcPin;
	
	/************************************************************************/
	/* ���ݽ��չ����̡߳���initMgr��������
	/* ֻ�������ݽ��չ���������ڱ�����Socket�б���û����ؼ�¼�����Զ������ͷ���ӵ�Socket�б�������SocketObj���󡣽��յ������ݴ���lpInfo->lpData->lpQueue���ɶ�Ӧ��workThreadHelperFunc������*/
	/************************************************************************/
	static  int workThreadFunc(ThreadInfo *lpInfo);

	/************************************************************************/
	/*���ݴ������̵߳ĸ����̡߳���initMgr��������                         */
	/*����Ӷ�Ӧ��lpInfo->lpData->lpQueue��ȡ���ݲ�����
	/************************************************************************/
	static	int workHelperThreadFunc(ThreadInfo *lpInfo);
	
	/************************************************************************/
	/*Udp�����߳�/
	/************************************************************************/
	static int UdpWrkThread(ThreadInfo *lpInfo);


	/************************************************************************/
	/* Client�˵����߳���Start����                                          */
	/************************************************************************/
	static int CltWrkThread(ThreadInfo *lpInfo);

	//Socket�������
	SocketObj* Append(SOCKET ,bool bIsListening);
	SocketObj* Append(SOCKET);
	SocketObj* Find(PEERID );
	
	//void Remove(SOCKET);
	void Remove(SocketObj*);

	SOCKET m_skt;					//��Socket
	SOCKET m_UdpSkt;				//������������UDP Socket
	
	FileDeliver *lpFileDeliver;
	HttpDeliver *lpHttpDeliver;
	

	ServantList *lpSvt;					//ServantList���Ѿ����ӵ�Node�б���lpSktObj->lpmCacheNode!=null
	vector<ThreadInfo *> vtThreads;		//Socket�̳߳�

	void initMgr();			//��ʼ��Mgr
	void SendToProxySrv(MessageBase *);

	//ͬ��ID��������
	static bool SyncIDHelper(Servant*,DWORD_PTR);
	
	/************************************************************************/
	/* ���ȳ�������
	/* ֻ����Ϊ�ͻ�������ʱ�������ǹؼ���ִ������                           */
	/************************************************************************/
	void static CALLBACK schedulerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//mCache��Ϣ��������
	void static CALLBACK mCacheProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//ExchangeBM��Ϣ��������
	void static CALLBACK ExBMProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//SyncID����
	void static CALLBACK SyncIDProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	void static  DoSyncIDProcess(SEGMENTID cStartID, ServntMgr *lpMe);
	
	//PrxyHello�̡߳�ֻ����LowIDʱ��������
	void static CALLBACK PrxHelloProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	//����߳�
	
	void static CALLBACK MonitorProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
		
	UINT uintScheduler,uintmCache,uintExBM,uintSyncID,uintPrxHello,uintMonitor;///,uintSendMsg;

	bool m_UdpIsOK;
	void startScheduler()	//�������ȳ���
	{
		uintmCache = timeSetEvent(MCACHE_TIMER,1,ServntMgr::mCacheProc,(DWORD_PTR)this,TIME_PERIODIC); //5������һ��

		uintExBM = timeSetEvent(EXBM_TIMER,1,ServntMgr::ExBMProc,(DWORD_PTR)this,TIME_PERIODIC);	//ExchangeBM����

		MMRESULT ret;
		if ( !bIsSrv ) //client
		{
			uintSyncID = timeSetEvent(SYNC_ID_TIMER,1,ServntMgr::SyncIDProc,(DWORD_PTR)this,TIME_PERIODIC); //SyncID
			lpBuf->setTimer();

			ret = timeSetEvent(SCHEDULER_TIMER,1,ServntMgr::schedulerProc,(DWORD_PTR)this,TIME_PERIODIC); //Scheduler
			uintScheduler = ret;
			if (IsLowID(dwSID))
			{
				uintPrxHello = timeSetEvent(PRX_HELLO_TIMMER,1,ServntMgr::PrxHelloProc,(DWORD_PTR)this,TIME_PERIODIC);
			}
			uintMonitor=timeSetEvent(10*1000,1,ServntMgr::MonitorProc,(DWORD_PTR)this,TIME_PERIODIC); //monitor
		}
		else //server
		{
			uintScheduler = NULL;
		}
	
	};
	void stopScheduler(bool isRestart = false)	//ֹͣ���ȳ���
	{
		if(NULL != uintScheduler)
			timeKillEvent(uintScheduler);
		timeKillEvent(uintmCache);
		timeKillEvent(uintExBM);
		
		if(!bIsSrv)
		{
			timeKillEvent(uintMonitor);
		}
		timeKillEvent(uintSyncID);
		
		
		if (uintPrxHello>0 )
		{
			timeKillEvent(uintPrxHello);
			uintPrxHello =0;
		}
	};

	//void startHttpd();		//��������http������
	//void stopHttpd();		//ֹͣ����http������
	
	
	void Restart()//������������Ϊ�ͻ���ʱ�ж���
	{
		lpBuf->Reset(0);
		lpCacheLock->on();
		for(vector<ServantInfo*>::iterator it = mCache.begin();it != mCache.end();it++)
		{
			if (NULL != (*it)->lpSktObj)
			{
				(*it)->lpSktObj->wSndSegID= 0;
				(*it)->lpSktObj->ClearSendBuf();
			}
		}
		lpCacheLock->off();

		lpHttpDeliver->Clear();

		//���·���Connect

		//Ͷ��һ����������

		MsgJoin *lpMsg = new MsgJoin(dwSID,dwChID);


		SOCKET skt ;
		if(bRunAsTCP)
			skt = sys->createSocket(SOCK_STREAM);
		else
			skt = m_skt;


		SocketObj *lpSrvSkt = Append(skt); // lpSrvSkt's lpmCacheNode is NULL,need to fix
		memcpy(&lpSrvSkt->addr,&serverAddr,sizeof(serverAddr));

		lpSrvSkt->addrlen = sizeof(serverAddr);
		lpMsg->vtIP = vtLocalIps;
		lpMsg->wPort =localAddr.sin_port;

		lpSrvSkt->SendTo(lpMsg);
		
	}
	void setStatus(int StatusCode,int Detail)
	{
		if (StatusPin != NULL)
		{
			StatusPin(StatusCode,Detail);
		}
	}
	void setStatus(int StatusCode)
	{
		setStatus(StatusCode,0);
	}
public:
	//void (* lpRestart)(void);
	//�������������������λ������[a,b]����[b,a]
	static int randInt(int a, int b)
	{
		if(a<b)
			return a + rand()%(b-a+1);
		else if(a>b)
			return b + rand()%(a-b+1);
		else // a==b
			return a;
	};

private:
typedef	struct SocketInfo_TAG
	{
		SOCKET skt;
		sockaddr_in addr;
		int addrlen;
	} SocketInfo;
};
