/*

�ڵ����

*/
#pragma once
#include "BufferMap.h"
#include "sys.h"
#include <vector>
#include "SocketObj.h"
#include <queue>
#include "messages.h"

class Servant
{
public:
	Servant(DWORD SID,SocketObj *lp_SktObj)
	{
		//��ʼ��socket����
		m_SID = SID;
		lpSktObj = lp_SktObj;
		lpSktObj->lpServant = this;

		m_Lock = new WLock();
		m_BM = new BufferMap();

	};


	~Servant(void);
private:
	DWORD	m_SID;				//ServantID

public:
	WLock	*m_Lock;

	SocketObj *lpSktObj;		//��Servant��Ӧ��SocketObj
	BufferMap*	m_BM;			//Buffer Map

	ThreadInfo	*m_Thread;		//�������߳�
	



private:
	vector<SEGMENTID> vtAppliedData;
	//WLock lpAppliedLock;

	//����
public:
	DWORD	getSID()	{return m_SID;}

	BufferMap* getBufferMap(void)	{ return m_BM; }
	void	setBufferMap(BufferMap* p)	{delete m_BM;m_BM = p;}

	//��������
public:
	//�������ݡ������������������ݼ��뵽���Ͷ�����
	
	//�õ�ApplyData�����ݺ����ñ��������Ѿ�ȷ�����ݱ��档������������һϵ��MsgPushData�󣬵���PushMsg
	void	ApplyDatas(vector<SEGMENTID> &vt)
	{
		if(_isNull())
			return;
		//lpAppliedLock.on();
		vector<SEGMENTID>::iterator it;
		for(it=vt.begin();it!=vt.end();it++)
		{
			if(vtAppliedData.size() >= MAX_SEGMENT_COUNT)
				vtAppliedData.erase(vtAppliedData.begin());
			vtAppliedData.push_back((*it));
		}
		//lpAppliedLock.off();
	};

	/************************************************************************/
	/* check if already applied data                                        */
	/************************************************************************/
	bool HasApplied(DWORD segID)
	{
		bool bret;
		//lpAppliedLock.on();
		bret = find(vtAppliedData.begin(),vtAppliedData.end(),segID) != vtAppliedData.end();
		//lpAppliedLock.off()();
		return bret;
	}

	void	ApplyData(SEGMENTID dwSegID)
	{
		if(_isNull())
			return;
		vector<SEGMENTID> vt;
		vt.push_back(dwSegID);
		ApplyDatas(vt);
	};
	inline bool _isNull()
	{
		Servant* _lp = this;
		if((DWORD_PTR)0xfeeefeee == (DWORD_PTR)_lp)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	/************************************************************************/
	/* ͬ��ID��ֻ����Ϊ�ͻ���ʱ���ж���                                     */
	/************************************************************************/
	void SyncID(DWORD);
};

//-----------------------------------------------
class ServantList
{
public:
	struct ServantNode
	{
		Servant* Servant;
		ServantNode *Next;
	};
public:
	ServantList(void);
	~ServantList(void);
	int Count ;
	WLock *m_Lock;
//private:
	ServantNode* m_Root;
	ServantNode* m_Tail;// ���е�ͷ��βָ��

public:
	//���Servant�����С�ȷ���������һ��Servant�������С�
	void	Append(Servant *lpSv)
	{
		ServantNode* p = new ServantNode();
		p->Servant = lpSv;
		p->Next = NULL;

		if( NULL == m_Root )
		{
			m_Root = p;
			m_Tail = p;
		}
		else
		{
			m_Tail->Next = p;
			m_Tail = p;
		}
		Count ++;
	}

	void	Remove(DWORD svtID)
	{
		Remove(getServant(svtID));
	}
	void	Remove(Servant *lpSr)
	{
		if(NULL == lpSr || NULL == m_Root)
		{
			return;
		}
		m_Lock->on();
		ServantNode *p ,*p2 = m_Root;

		if(m_Root->Servant == lpSr)
		{
			m_Root = m_Root->Next;
			if(lpSr == m_Tail->Servant)
				m_Tail=NULL;
			delete p2;
			Count --;
		}
		else
		{
			p = m_Root->Next;
			while(NULL != p)
			{
				if(p->Servant == lpSr)
				{
					p2->Next = p->Next;
					if (lpSr == m_Tail->Servant )
					{
						m_Tail = p2;
					}
					delete p;
					Count--;
					break;
				}
				p2 = p;
				p = p->Next;
			}
		}
		m_Lock->off();
	}
	//��ȡָ��ID��Servant
	Servant* getServant(const DWORD SID)
	{
		if( NULL == m_Root)
			return NULL;
		m_Lock->on();
		ServantNode *p = m_Root;
		while(NULL != p)
		{
			if(p->Servant->getSID() == SID)
				break;
				//return p->Servant;
			p = p->Next;
		}
		m_Lock->off();
		if(NULL != p)
			return p->Servant;
		else
			return NULL;
	};



	//��ȡָλ�õ�Servant
	Servant* getServantAt(int idx)
	{
		if(idx < 0 || idx > this->Count )
		{
			return NULL;
		}
		if( NULL == m_Root)
			return NULL;
		m_Lock->on();
		ServantNode *p = m_Root;
		int iLoc = 0;
		while(iLoc != idx )
		{
			p = p->Next;
			iLoc ++;
		}
		m_Lock->off();
		if(NULL != p)
			return p->Servant;
		else
			return NULL;
	}

	void processSevants_s(bool (*processor)(Servant *,DWORD_PTR),DWORD_PTR  dwUser)
	{
		if(NULL == m_Root)
			return;
		m_Lock->on();
		
		ServantNode *p = m_Root;
		while(NULL !=p)
		{
			processor(p->Servant,dwUser);
			p = p->Next;
		};
		m_Lock->off();
	}
};