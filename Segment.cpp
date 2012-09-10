#include "StdAfx.h"
#include "Segment.h"

int Segment::m_PacketsOfSegment = 0;
int Segment::m_SegmentSize = 0;
int Segment::m_ChunkSize =0;


Segment::Segment(void)
		:m_segID(0)
		,m_DeadLine(0)
		,m_Data(NULL)
		,m_hasData(false)
		
{

	m_Lock = new WLock();
	t_ApplyTime = 0;
	
}

Segment::~Segment(void)
{
	if(m_Data)
	{
		delete[] m_Data;
	}
	delete m_Lock;
}

//���ö�
void Segment::Reset(SEGMENTID id,byte deadline)
{
	vtSuppliers.clear();
	bDelivered = false;

	m_segID = id;
	m_DeadLine = deadline;
	if(!m_Data)
	{
		m_Data = new char[Segment::m_SegmentSize];
	
	}
	memset(m_Data,0,Segment::m_SegmentSize); //��������0
	m_hasData = false;
	if( 0 == m_PM.capacity())
	{
		m_PM = vector<bool>(Segment::getPacketsOfSegment());
	}
	m_PM.assign(m_PM.size(),false);
	
}

//������ѹ��
PushDataResult Segment::PushData(byte seq, const char *p,int Length)
{
	//�����������Ƿ��Ѿ����ڣ��������ֱ�Ӷ���
	if(m_hasData || m_PM[seq])
	{
		return PushData_NotUse;
	}

	//�����ݸ��Ƶ�m_data�С�����seq������ʼλ�á�
	memcpy(m_Data + seq * Segment::getRealPacketSize() , p , Length);
	//����m_PM
	m_PM[seq]=true;
	bool bIsFill = true;
	for(int i =0;i < Segment::getPacketsOfSegment(); i++)
	{
		if(false == m_PM[i])
		{
			bIsFill = false;
			break;
		}
		
	}
	if(bIsFill)
	{
		m_hasData = true;
		return PushData_OK;
	}
	else
	{
		return PushData_Partail;
	}
	/*
	//����BM��Ӧ��λ
	if(NULL != setBMFunction)
	{
		setBMFunction(m_segID);
	}
	*/
}

//---------------------------------------------

SEGMENTID SegmentList::dwSegID_Start = 0;

SegmentList::SegmentList():
	m_lpRoot(NULL),
	m_lpTail(NULL)
{
	m_Lock = new WLock();
	
}
bool	DestorySegment(Segment *p)	{ delete p;return true; };
SegmentList::~SegmentList()
{
	ProcessSegment_s(DestorySegment);
	delete m_lpRoot;
	delete m_lpTail;
	delete m_Lock;
}
//�ڱ������л������ٽ����ṩ�̰߳�ȫ����
void SegmentList::ProcessSegment_s(bool (*Processor)(Segment *))
{
	//�����ٽ���
	m_Lock->on();
	ProcessSegment(Processor);
	//�뿪
	m_Lock->off();
}
//������������е�ÿ��Segment��Ҫ�󴫵�һ������ָ���������Ϊ��������������Ҫ����� bool fun(Segment *)ǩ����
//���̰߳�ȫ�汾
void SegmentList::ProcessSegment(bool (*Processor)(Segment *))
{
	
	SegmentNode *p = m_lpRoot;//,*p2;

	while(NULL != p)
	{
		//p2 = p->Next;
		p->Segment->m_Lock->on();
		if(false == Processor(p->Segment))
		{
			p->Segment->m_Lock->off();
			break;
		}
		p->Segment->m_Lock->off();
		p = p->Next ;
	}

}

//��������ǰ�����Ѿ��õ�ý��ͷ
void SegmentList::CreateList(SEGMENTID StartSegID)
{
	SegmentNode *p = NULL;

	m_Lock->on();
	for(byte i = 0 ; i< MAX_LIVE_TIME; i++)
	{
		SegmentNode *node = new SegmentNode();
		if( 0 == i)
		{
			m_lpRoot = node;
		}

		node->Segment = new Segment();
		node->Segment->Reset(StartSegID ++, i); //��ʼ��Segment;
		node ->Next = NULL;

		if(NULL != p)
		{
			//��������
			p->Next = node;
		}
		p = node;	 //pָ���´����Ľڵ�
	}
	m_lpTail = p;
	
	m_Lock->off();
}

bool	DecreaseDeadline(Segment *p)	{ p->DecreasetDeadLine();return true; };

//ʱ�䲽����������Segment��Deadline-1������������ṹ��ȡ��deadline<0�Ľڵ�
void SegmentList::TimeStep()
{
	if (NULL== m_lpRoot)
	{
		return;
	}
	//TODO: StartSegID ++;
	m_Lock->on();

	ProcessSegment(DecreaseDeadline);//Deadline --;
	
	//����������ͷ�������ã�Ȼ������β
	SegmentNode *p = m_lpRoot;
	
	p->Segment->m_Lock->on();

	m_lpRoot = m_lpRoot->Next;

#if _DEBUG
	SEGMENTID wOID = p->Segment->getSegID();
	char* _lpData = p->Segment->m_Data;
	if(p->Segment->getHasData())
	{
		//log::WriteToFile( _lpData,Segment::getSegmentSize());
	}
	else
	{
		int icnt =p->Segment->getPacketCount();
		 cout << "Segment:" << p->Segment->getSegID() << " only has " << icnt << " Packets";
	//	::_CrtDbgBreak();
	}
#endif

	p->Next = NULL;
	
	p->Segment->Reset(p->Segment->getSegID() + MAX_LIVE_TIME);
	p->Segment->m_Lock->off();

	m_lpTail->Next=p;
	
	m_lpTail = p;
//#if _DEBUG
//	char buf[50];
//	sprintf(buf,"TimeStep.Release %d , get New ID %d" ,wOID,p->Segment->getSegID());
//	log::WriteLine(buf);
//#endif
	m_Lock->off();
}

Segment* SegmentList::getSegment(SEGMENTID segID)
{
	m_Lock->on();
	SegmentNode *p = m_lpRoot;
	while(NULL != p)
	{
		assert(::_CrtIsValidPointer(p->Segment,sizeof(p->Segment),TRUE));
		p->Segment->m_Lock->on();
		if(p->Segment->getSegID() == segID)
		{
			p->Segment->m_Lock->off();
			break;
		}
		p->Segment->m_Lock->off();

		p = p->Next;
	}
	m_Lock->off();
	if(p == NULL)
		return NULL;
	else
		return p->Segment;
}

//��������
PushDataResult SegmentList::PushData(SEGMENTID segID,byte seq, const char *p, int Length )
{
	Segment* lpSeg = getSegment(segID);
	if(NULL !=lpSeg)
	{
		lpSeg->m_Lock->on();
		PushDataResult result = lpSeg->PushData(seq,p,Length);
		lpSeg->m_Lock->off();
		return result;
	}
	return PushData_OutOfDeadLine;
}