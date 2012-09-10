#pragma once
#include "messagebase.h"
#include <time.h>
class MsgPushHeader :
	public ReliableMessageBase
{
public:
	MsgPushHeader(void);
	~MsgPushHeader(void);

public:
	SEGMENTID wInitSegID;	//��ʼSegID
	time_t tInitTime;	//��ʼʱ��
	char *lpData;			//header����
	int dataLen;			//���ݳ���
private:
	bool bIsOwnerBuf;
public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};

//-------------------------------------------------------

class MsgPushHeaderR :
	public ACKMessageBase
{
public:
	MsgPushHeaderR(WORD wSeq):ACKMessageBase()
	{
		wCMD = CMD_PUSH_HEADER_R;
		wSeqID = wSeq;
	}
	MsgPushHeaderR(void):ACKMessageBase(){wCMD = CMD_PUSH_HEADER_R;};
public:
	void fromBytes(const char*p,const int len);
	void toBytes();
};