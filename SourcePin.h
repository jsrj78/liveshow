#pragma once
/************************************************************************/
/*����Դ�ӿڣ�ֻ����Server״̬����                                      */
/************************************************************************/
#include "Buffer.h"
#include "sys.h"

class SourcePin
{
public:
	SourcePin(void);
	~SourcePin(void);
	void Start(Buffer* lpBuf);
	void Stop();

private:
	bool bIsRunning ;
	static int DataProcessFunc(ThreadInfo *lpInfo);
	ThreadInfo *lpThrd;
	Buffer *lpBuf;
};
