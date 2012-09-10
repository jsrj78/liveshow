/*

����1��������

*/

#pragma once
#include "common.h"
#include "sys.h"

using namespace std;

enum PushDataResult
{
	//ѹ��ɹ����Ҿ߱�ȫ������
	PushData_OK,
	//ѹ��ɹ��������ݲ�������
	PushData_Partail,
	//�ظ�����
	PushData_NotUse,
	//���ѹ���
	PushData_OutOfDeadLine
};

/************************************************************************/
/* ����1��������                                                        */
/************************************************************************/
class Segment
{
public:
	Segment(void);
	~Segment(void);
private:
	SEGMENTID			 m_segID;					//��segment��ID
	char				 m_DeadLine; 
	
	bool				 m_hasData;					//�Ƿ��Ѿ�������
	
	
	
	static int			m_SegmentSize;				//Segment��С����λ�ֽ�
	static int			m_PacketsOfSegment ;		//ÿ��segment������Packet����
	static int			m_ChunkSize;				//asf Chunk�Ĵ�С
public:
	vector<bool>		 m_PM;						//Segment�и���packet�ı��λ
	char*				 m_Data;					//������
//	void (*setBMFunction)(WORD segID);				//����ָ�롣Buffer��Ҫʵ�ִ˺�������������ָ�븳ֵ��������
	WLock*				m_Lock	;
	vector<DWORD>		vtSuppliers;				//�����ṩ�ߵ�SID�б�
	time_t				t_ApplyTime;				//��������ʱ��
	bool				bDelivered;					//�����Ƿ��Ѿ����·���ȥ
public:
	void Reset(void)								//����Segment
	{
		Reset(0);
	}
	void Reset(SEGMENTID id)
	{
		Reset(id,MAX_LIVE_TIME);
	}

	void Reset(SEGMENTID id,byte deadline);

	inline int getPacketCount()
	{
		//return static_cast<int>(std::count_if(m_PM.begin(),m_PM.end(),true));
		int cnt = 0;
		for(vector<bool>::iterator it = m_PM.begin();it != m_PM.end();it++)
		{
			if((*it))
				cnt++;
		}
		return cnt;
	};

	inline SEGMENTID getSegID()	{ return m_segID; };

	//�����ջ����е����ݴ���segment��ָ��packet��
	
	PushDataResult PushData(byte seq,const char *p,int Length=Segment::getRealPacketSize());		

	
	inline bool getData(char** p)
	{
		if( !m_hasData) 
			return false;
		if ( NULL == *p)
			*p = new char[Segment::getSegmentSize()];
		else if(sizeof(*p)< Segment::getSegmentSize())
		{
			delete[] *p;
			*p = new char[Segment::getSegmentSize()];
		}
		memcpy(*p,m_Data,Segment::getSegmentSize());

		return true;
	};

	//û����ȫ����ʱֻҲ���صõ�����ȷ����
	int getData2(char **p,bool bIsImmediate)
	{
		/*if (getData(p))
		{
			return Segment::getSegmentSize();
		}
		else
		{
			return 0;
		}*/
		m_Lock->on();
		if (m_hasData && bIsImmediate)
		{
			getData(p);
			m_Lock->off();
			return Segment::getSegmentSize();
		}
		else if (m_hasData)
		{
			getData(p);
			m_Lock->off();
			return Segment::getSegmentSize();
		}
		else
		{

			int iCnt =0;
			int iSize = 0;
			for (vector<bool>::iterator it = m_PM.begin();it!= m_PM.end();it++)
			{
				if(*it)
					iCnt++;
			}
			if (iCnt != 0)
			{
				if (NULL==*p)
				{
					*p = new char[Segment::getRealPacketSize()*iCnt];
				}
				iCnt =0;
				int iFilledCnt =0;
				
				for (vector<bool>::iterator it = m_PM.begin();it!= m_PM.end();it++)
				{
					if ((*it))
					{
						if (iCnt!= getChunksOfPackage() -1) //not last
						{
							memcpy(*p+Segment::getRealPacketSize()*iFilledCnt,m_Data+iCnt*getRealPacketSize(),getRealPacketSize());
							iSize+=getRealPacketSize();
						}
						else
						{
							memcpy(*p+Segment::getRealPacketSize()*iFilledCnt,m_Data+iCnt*getRealPacketSize(),getLastPacketSize());
							iSize+=getLastPacketSize();
						}
						
						iFilledCnt++;
					}
					iCnt++;
				}
		
			}
			m_Lock->off();
			return iSize;
		}
	}

	inline bool getHasData(void){return m_hasData;}

	inline vector<bool> GetPacketMap(void){return m_PM;} 

	inline void DecreasetDeadLine()	{ m_DeadLine--;}

	inline static void setSegmentSize(int size)
	{
		Segment::m_SegmentSize = size;
		//Segment::m_PacketsOfSegment = (size % PACKET_DATA_SIZE) == 0
		//								? size / PACKET_DATA_SIZE
		//								: size / PACKET_DATA_SIZE +1;

	}
	inline static void setChunkSize(int size)
	{
		m_ChunkSize = size;
	}

	inline static int getChunksOfPackage()
	{
		return PACKET_DATA_SIZE / m_ChunkSize;
	}
	
	inline static int getPacketsOfSegment(void)
	{
		Segment::m_PacketsOfSegment = (	m_SegmentSize % getRealPacketSize()) == 0
										? m_SegmentSize /  getRealPacketSize()
										: m_SegmentSize / getRealPacketSize() +1;
		return Segment::m_PacketsOfSegment;
	}

	inline static int getSegmentSize(void)
	{
		return Segment::m_SegmentSize;
	}
	inline static int getRealPacketSize()
	{
		return getChunksOfPackage() * m_ChunkSize ;
	}
	//����һ������С
	inline static int getLastPacketSize(void)
	{
		return Segment::m_SegmentSize % getRealPacketSize()  == 0
										?  getRealPacketSize()
										: Segment::m_SegmentSize  - ((Segment::getPacketsOfSegment() -1) *getRealPacketSize() ) ;
	}

	inline int getDeadline(){return m_DeadLine; };


};


//Segment ��
class SegmentList
{
public:
	SegmentList();
	~SegmentList();
private:
	//����ṹ����
	struct SegmentNode
	{
		Segment*		Segment;
		SegmentNode		*Next;
	};
	
	SegmentNode			*m_lpRoot, //������ڵ�
						*m_lpTail; //���һ���ڵ�
	WLock				*m_Lock;
	//deadline -= deadline
	static SEGMENTID dwSegID_Start;//������Ϊ���±ߵĺ�����������;

	//������������ѭ���б�����
	static bool ResetListProc(Segment *p)
	{
		p->Reset(dwSegID_Start);
		dwSegID_Start ++;
		return true;
	};
public:
	//������������е�ÿ��Segment��Ҫ�󴫵�һ������ָ���������Ϊ��������������Ҫ����� bool fun(Segment *)ǩ����
	//�ڱ������л������ٽ����ṩ�̰߳�ȫ����
	void	ProcessSegment_s(bool (*Processor)(Segment*));
	//���̰߳�ȫ�汾
	void	ProcessSegment(bool (*Processor)(Segment*));
	//��������
	void	CreateList(SEGMENTID startSegID);
	//ʱ�䲽����������Segment��Deadline-1������������ṹ��ȡ��deadline<0�Ľڵ�
	void	TimeStep();
	void	ResetList(SEGMENTID startSegID)
	{
		dwSegID_Start = startSegID;
		ProcessSegment(SegmentList::ResetListProc);
	};
	PushDataResult PushData(SEGMENTID segID,byte seq,const char *p,int Length=Segment::getRealPacketSize());
	Segment* getSegment(SEGMENTID segID);
};