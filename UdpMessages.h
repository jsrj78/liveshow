#pragma once
#include "MessageBase.h"
#include <vector>

using namespace std;

typedef struct tagUdpCmd
{
	//ָ��
	WORD wCMD;
	//����ָ���SID
	DWORD dwSID;
	//����ָ���ߵ�IP
	DWORD dwIP;
	//����ָ���ߵĶ˿ں�
	WORD wPort;
	//��Ӧ��Ƶ��ID
	DWORD	CHID;
}	UdpCmd;
#define SizeOfUdpCmd 16 
//////////////////////////////////////////////////////////////////////////

class UdpHello
	: public MessageBase
{
public:
	UdpHello():MessageBase()
	{
		wCMD = PXY_HELLO;
	};
	~UdpHello()
	{
		
	}
	DWORD SID;
	void fromBytes(const char*p,const int len);
	void toBytes();
};

class UdpHelloR
	: public MessageBase
{
public:
	UdpHelloR():MessageBase()
	{
		wCMD = PXY_HELLO_R;
	};
	//����ָ����
	DWORD SID;
	vector<UdpCmd*> vtCmds;	//һ���ŵ���callback����
	void fromBytes(const char*p,const int len);
	void toBytes();
};

//////////////////////////////////////////////////////////////////////////

class UdpCallBack
	:public MessageBase
{
public:
	UdpCallBack():MessageBase()
	{
		wCMD = PXY_CALLBACK;
	}
	~UdpCallBack()
	{
		
	}
	//����ָ����s
	DWORD SID;
	UdpCmd Body;

	void fromBytes(const char*p,const int len);
	void toBytes();
};

