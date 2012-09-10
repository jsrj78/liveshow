#pragma once
#include "Segment.h"
#include "sys.h"
#include <map>
#include "BufferMap.h"
#include "IDeliver.h"

using namespace std;

#define TIME_STEP_TIMER	1*1000	//TimeStep���̵�ִ�м��
#define TIMER_PRELUDE  (UINT)(MAX_SEGMENT_COUNT *0.8 * 1000) // ��һ������Timer��ʱ��������λ���롣
#define DELIVER_PROC	void (*Proc)(Segment*,bool) //�����·�����ǩ��
#define DELIVER_PROC_TIMER	667	//Deliver���̵�ִ�м��ÿ����ִ������
class Buffer
{
public:
	Buffer(void);
	~Buffer(void);
	
private:
	
	SegmentList*	m_lpSegList;
	
	//���ö�ʱ��
	
	static void CALLBACK TimerProc(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);

	
	vector<IDeliver*> vtDeliverProcs;
public:
	

	struct _MediaHeader_TAG
	{
		char* lpData ;
		int len;
		SEGMENTID dwStartID;
	} m_Header;

	BufferMap*		m_lpBM;	

	UINT			m_lpTimer;
	int				m_TimerElapse;//ʱ����


	SEGMENTID			wServerCurrSegID;//���������ϵ�ǰ��SegmentID


	//�趨��ʼ��ʱ�����˶�ʱ����ִ��һ�Σ�֮��������ʱ��Normalȡ��
	void			setTimer();
	bool			bIsSrv;//�Ƿ��Ƿ�������

	void			clearTimer()
	{
		if(NULL != m_lpTimer)
			timeKillEvent( m_lpTimer);
		m_lpTimer = NULL;
	};

	//��������ִ�еĶ�ʱ�����˶�ʱ��ÿ��ִ��һ��
	void		startNormalTimer()
	{
		return;
		if(m_TimerElapse == TIME_STEP_TIMER)
			return;
		clearTimer();

		m_lpTimer = timeSetEvent(TIME_STEP_TIMER,50,TimerProc,(DWORD_PTR)this,TIME_PERIODIC);
		m_TimerElapse = TIME_STEP_TIMER;

	}
	
	///��������ר�á���������ʱ��
	PushDataResult srv_PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize())	
	{
		if( NULL == m_lpTimer)
		{
			m_lpTimer = 1;
			m_TimerElapse = NULL;
		}
		PushDataResult ret = PushData(segID,seq,p,Length) ;
		if(PushData_OutOfDeadLine == ret)
		{
			TimeStep();
			return PushData(segID,seq,p,Length);
		}
		else
		{
			return ret;
		}
	}
	
	PushDataResult srv_PushData(SEGMENTID segID,const char *p,int Length)	
	{
		for (char cSeq = 0;cSeq < Segment::getPacketsOfSegment();cSeq++)
		{
			if (cSeq==Segment::getPacketsOfSegment() -1) //last packet
			{
				PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq,Segment::getLastPacketSize());

			}
			else
			{
				PushDataResult ret  = PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq);
				if(PushData_OutOfDeadLine == ret)
				{
					TimeStep();
					PushData(segID,cSeq,p+Segment::getRealPacketSize()*cSeq);
				}
			}
		}
		return PushData_OK;
		
	}
	//�ͻ���ר�á���Ҫʱ������ʱ��
	PushDataResult PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize())
	{
		m_lpBM->m_Lock->on();

		PushDataResult rst = m_lpSegList->PushData(segID,seq,p,Length);


		if(PushData_OK == rst) //�߱�ȫ�����ݺ�����bm
		{
#if _DEBUG
		//char buf[512];
			//sprintf(buf,"Get Data .segID:%d",segID,seq);
			//log::WriteLine(buf );
			
#endif
			m_lpBM->SetBM(segID);

		if((NULL == m_lpTimer || m_TimerElapse == TIMER_PRELUDE) && (this->m_lpBM->getFullRate() >= 80 || segID > m_lpBM->getStartID() * MAX_SEGMENT_COUNT* 0.8)  ) //TODO:�������˲�������ʱ��
		{
			//TODO:���������StartID�������StartID����25%��������
			if(abs(m_lpBM->getStartID() - wServerCurrSegID) > MAX_SEGMENT_COUNT * STARTID_MARGIN)
			{
				startNormalTimer();
			}
		}

		}
		else if(PushData_Partail == rst)
		{
		
		}
		else
		{
#if _DEBUG
			//sprintf(buf,"Segment%d is Out of deadline.",segID);
			//log::WriteLine(buf);
#endif
		}
		
		m_lpBM->m_Lock->off();
		if (rst==PushData_OK)
		{
			Deliver(getSegment(segID),true);
		}
		return rst;
	}

	//����Buffer��ÿ�ε��õ���һ���ӵ�����
	void TimeStep()
	{
		if(m_lpTimer == 1) //��Ϊ����������
		{
			//Buffer::DeliverProc(0, 0, (DWORD_PTR)this, (DWORD_PTR)getStartID(),NULL);
		}
		m_lpSegList->TimeStep();
		m_lpBM->TimeStep();
	}

	//���õ�ǰ�Ρ���ʱ�������ͬ��ʱ����ô˷���
	void SetPosition(SEGMENTID segID)
	{

		
		while(segID > getStartID()) 
		{
			Deliver(getSegment(getStartID()),false);
			TimeStep();//����

		}
	
	}

	void Reset(SEGMENTID segID)
	{
		m_lpSegList->ResetList(segID);
		m_lpBM->Reset(segID);
	}

	void Init(SEGMENTID segID)
	{
	
		m_lpSegList->CreateList(segID);
	
		m_lpBM->setStartID( segID);
	}
	Segment* getSegment(SEGMENTID segID)
	{
		return m_lpSegList->getSegment(segID);
	}

	inline SEGMENTID getStartID()
	{
		return m_lpBM->getStartID();
		
	}


	/************************************************************************/
	/* ע��Deliver������                                                     */
	/************************************************************************/
	void RegisterDeliver(IDeliver *lpProc)
	{
		lpProc->bDeliveredHeader= false;
		vtDeliverProcs.push_back(lpProc);
	}
	void UnRegisterDeliver(IDeliver *lpProc)
	{
		//TODO:not implement
		return;

	}
	void Deliver(Segment* p,bool isImmediate)
	{
		for (vector<IDeliver*>::iterator it = vtDeliverProcs.begin(); it != vtDeliverProcs.end() ; it++)
		{
			if(!(*it)->bDeliveredHeader)
			{
				(*it)->DeliverHeader(m_Header.lpData,m_Header.len);
				(*it)->bDeliveredHeader = true;
			}
			if (isImmediate)
			{
				(*it)->ImmediateDeliver(p,true);
			}
			else
			{
				(*it)->Deliver(p,false);
			}
		}
	}
};

