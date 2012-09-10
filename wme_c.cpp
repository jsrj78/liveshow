#pragma once

#include "StdAfx.h"
#include "wme_c.h"
#include "sys.h"
#include "stream.h"
#include "common.h"
#include "asf.h"

const char* HTTP_REQ_CMD = "GET / HTTP/1.0\r\n";
const char* HTTP_REQ_AGENT = "User-Agent: NSPlayer/4.1.0.3925\r\n";
const char* HTTP_REQ_PRAGMA = "no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=1,max-duration=0\r\nxClientGUID={2200AD50-2C39-46c0-AE0A-2CA76D8C766D}\r\n";
const char* HTTP_REQ_PRAGMA2 ="Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=4294967295:4294967295,request-context=2,max-duration=2147609515\r\nPragma: xPlayStrm=1\r\nPragma: xClientGUID={2200AD50-2C39-46c0-AE0A-2CA76D8C766D}\r\nPragma: stream-switch-count=2\r\nPragma: stream-switch-entry=ffff:1:0 ffff:2:0\r\n";
wme_c::~wme_c(void)
{
}

void wme_c::Start(char *ip, WORD port)
{
	//�ŵ�һ���߳���
	
	lpInfo.lpIP = ip;
	lpInfo.wPort = port;
	lpInfo.lpInstance = this;

	lpThrd = new ThreadInfo();
	lpThrd->data = &lpInfo;
	
	lpThrd->func = (THREAD_FUNC)DataProcessFunc;
	sys->startThread(lpThrd);
	
}

int wme_c::DataProcessFunc(ThreadInfo *lpInfo)
{
	IpInfo* lpIP = ((IpInfo*)lpInfo->data);
	char *ip = lpIP->lpIP;
	WORD port = lpIP->wPort;
	
	while(1)
	{

		lpIP->lpInstance->getHeader(ip,port);

		lpIP->lpInstance->processData(ip,port);
		//lpIP->lpInstance->receive(ip,port);
	}
	return 0;
}
char* wme_c::getHeader(char* ip,WORD port)
{
	Buffer* lpBuf = m_buf;
	SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e ;
	if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR && GetLastError() == WSAEWOULDBLOCK)
	{		
		e=GetLastError();
		throw StreamException("wme_c::getHeader.connect ",e);
		//return NULL;
	}
	MemoryStream *stream  =new MemoryStream(1024);
	memset(stream->buf,0,1024);
	stream->write(HTTP_REQ_CMD, static_cast<int>( strlen(HTTP_REQ_CMD)  ));
	stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
	stream->write(HTTP_REQ_PRAGMA,static_cast<int>(strlen(HTTP_REQ_PRAGMA))  );
	stream->writeChar('\r');
	stream->writeChar('\n');

	if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
	{
		e = GetLastError();
		throw StreamException("wme_c::getHeader.send ",e);
		//return NULL;
	}
	delete stream;
	 //�������
	char buf[8192];
	char *lpRet = NULL;//= new char;
	int iGet= 0,iPos = 0;
	WORD len = 0;
	while(true) //��ȡ���ݵ�β
	{
		int l =0;
		if( (l = recv(skt,buf+iGet,sizeof(buf) - iGet,0)) == SOCKET_ERROR)
		{
			e = GetLastError();
			throw StreamException("wme_c::getHeader.recv ",e);
		}
		if( 0 == l)
		{
			//throw StreamException("wme_c::getHeader.socetclosed ");
			break;
		}
		iGet +=l ;
	}
		
	if(strstr(buf,"OK") !=NULL) // get ok
	{
		char* pLoc = strstr(buf,"\r\n\r\n");
		pLoc +=4;//�ƶ���\r\n\r\n��ߣ�����ʵ�ʵİ���

		if( 0x24 == pLoc[0]  && 0x48 == pLoc[1]) //header
		{
			
			pLoc += 2;
			memcpy(&len,pLoc,2);
			len -= 8;
			//&len  pLoc[2] < 8 & pLoc[3]; //get length
			lpRet = new char[len];
			pLoc += 10 ;
			memcpy(lpRet,pLoc,len);

			

			//����Segment::setSegmentSize;
			//memcpy(&dwPacketSize,pLoc + HDR_ASF_PACKET_SIZE,4);
			//memcpy(&dwBitRate,pLoc + HDR_ASF_BITRATE,4);
			asfHeader *objHeader = new asfHeader(pLoc,len);
			dwPacketSize = objHeader->getChunkSize();
			dwBitRate = objHeader->getBitRate();
			delete objHeader;
			int size = ( dwBitRate /8 /dwPacketSize)*dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
			Segment::setSegmentSize( size);//������ 1.06M
			
			lpBuf->m_Header.lpData = lpRet;
			lpBuf->m_Header.len = len;
			lpBuf->Init(0);
		}
		
		
	}
	else
	{
		//error
		return NULL;
	}
		
	
	closesocket(skt);

	return lpRet;
}


void wme_c::receive(char* ip,WORD port)
{
	Buffer* lpBuf = m_buf;
	SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e;

	int iReqNum = 1;	//�������
	bool bCnnected = false;

	const int BUF_SIZE = 64*1024 +12 ;// 8192; //64KB
	char buf[BUF_SIZE];
	int iGet = 0;
	int iPos = 0;
	bool bInited = false;
	while (true)
	{
		
		if (!bCnnected) //not connected
		{
			if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR)
			{		
				e=GetLastError();
				throw StreamException("wme_c::getHeader.connect ",e);
				//return NULL;
			}	
			MemoryStream *stream  =new MemoryStream(2048);
			if (iReqNum % 2 != 0) //��һ������
			{
				memset(stream->buf,0,stream->len);
				stream->write(HTTP_REQ_CMD, static_cast<int>( strlen(HTTP_REQ_CMD)  ));
				stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
				stream->write(HTTP_REQ_PRAGMA,static_cast<int>(strlen(HTTP_REQ_PRAGMA))  );
				stream->writeChar('\r');
				stream->writeChar('\n');
			} 
			else //�ڶ�������
			{
				
				memset(stream->buf,0,stream->len);
				stream->write(HTTP_REQ_CMD, static_cast<int>(strlen(HTTP_REQ_CMD)  ));
				stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
				stream->write(HTTP_REQ_PRAGMA2,static_cast<int>(strlen(HTTP_REQ_PRAGMA2))  ); //�ڶ�������
				stream->writeChar('\r');
				stream->writeChar('\n');
			}
			if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
			{
				e = GetLastError();

				return ;
			}
			delete stream;
		} 
		
		Sleep(10);
lblRecv:
		if (iPos > 0)
		{
			memcpy(buf,buf+iPos,iGet - iPos);
			iGet -= iPos;
			iPos = 0;
		}
		int iRecv = recv(skt,buf+iGet,BUF_SIZE - iGet,0);
		if (0 == iRecv)
		{
			//net workerror
		}
		iGet += iRecv;

		char* HTTP = "HTTP";
		
		char* RNRN = "\r\n\r\n";
		if (memfind(buf,iGet,HTTP,4) != NULL)
		{
			iPos = ( memfind(buf,iGet,RNRN,4)-buf) + 4;
		}
		AsfChunkHeader chkHeader;
		memcpy(&chkHeader,buf+iPos,sizeof(chkHeader));
		if (chkHeader.wConfirmLen != chkHeader.wLen)
		{
			//TODO:error
		}
		if (chkHeader.wLen + 4> iGet - iPos )
		{
			goto lblRecv;
		}

		switch(chkHeader.wCMD)
		{
		case 0x4824://"$H"
			{
				if (!bInited)
				{
					asfHeader *objHeader = new asfHeader(buf+iPos + 4 + 8,chkHeader.wLen);
					dwPacketSize = objHeader->getChunkSize();
					dwBitRate = objHeader->getBitRate();
					delete objHeader;
					int size = ( dwBitRate /8 /dwPacketSize)*dwPacketSize;//(349056 /8 /0x5a4) * 0x5a4;
					Segment::setSegmentSize( size);//������ 1.06M

					lpBuf->m_Header.lpData = new char[chkHeader.wLen - 8];
					memcpy(lpBuf->m_Header.lpData,buf + iPos + 4 + 8 ,chkHeader.wLen -  8);
					lpBuf->m_Header.len = chkHeader.wLen - 8;
					lpBuf->Init(0);
					bInited = true;
				}
				bCnnected =true;
				iPos +=  chkHeader.wLen + 4;
			}
			break;
		case 0xa444://?D
		case 0x4424://$D
			{
				iPos = processData(lpBuf,buf,iPos,iGet);
				if(iPos<0)
				{
					bCnnected = false;
					closesocket(skt);
				}
			}
		    break;
		
		}
		
	}

}

//must process one asf chunk only!!!
int wme_c::processData(Buffer *lpBuf, char* buf,int iPos,int iGet)
{
	static int cSeqID = 0 ;//�ӵ����뿪ʼ �����Ի��塣 //TODO:���Դ�BM�еõ���ǰIDȻ���һ������ʱ��
	static byte cSeq = 0;	//live show packet �е����
	 int iAsf = 0; //��ǰasf chunk��С
	 char buf_t [PACKET_DATA_SIZE];
	 int iOffset_t = 0;
	 int len;
	 int iNeedData = 0 ; //��ǰ���ݰ���Ҫ������
	if (cSeq != 0) //��������segment
	{
		cSeq = 0;
	}
	static int iPadLen = 0;// Padding data length
	
		while(iPos < iGet ) //���ݴ���ѭ��.iPos��ǰ���ݴ���ָ�룬iGet����ǰ�������ݳ���
		{
			int iAvailable = iGet - iPos; //���ջ����п�������

			if(iAvailable <= 12)
				break; //��������wme�ж�ȡ�������ݡ�
			if((char)0x24 == buf[iPos]  && (char)0x44 == buf[iPos + 1]) //a new asf chunk
			{
				if (memcmp(buf+iPos+2,buf+iPos+10,2)!=0)//len != confirm len
				{
					std::cout << endl << "ERROR!!Read Http Streaming chunk Error!len != confirm len.ERROR!!" << endl;
					
					return -1;
				}

				memcpy(&len,buf + iPos +2 ,2);//�õ����ݵı���С
				len -= 8;//http stream�еĳ���= ���ݳ��ȼ�������ͷ����(8�ֽ�)
				assert(len > 0);

				iAsf = len ; 

				//ȥ�� http streaming ͷ ������ͷ����4+8
				iPos += 12;
				iAvailable -= 12;
				//fix asf parse info 's padding length
				
				//���¼������ݲ���ȷ��û�д���Error correction data(82 00 00)

				//����ÿ������0x 82 00 00 ��ʼ���������һ��������asf parse info�ĵ�һ���ֽ�
				int iSkip =0;

				//assert((char)0x82==buf[iPos] && (char)0 == buf[iPos+1] && (char)0 == buf[iPos+2]);

				char c= (buf[iPos +3] & 0x60) ; //�ж� Packet Length Type�Ƿ����
				c = c>> 5 ;	

				switch(c)
				{
				case 0:
					iSkip =0;
					break;
				case 1:
					iSkip = 1;
					//TODO:���256�ֽ�һ���������񲻻�����������
#if _DEBUG
					log::WriteLine("Packet Length is 0x01(Byte)");
#endif
					memcpy(buf+iPos +5,&dwPacketSize,1);
					break;
				case 2: //340Kbps���Ҷ�0x5a4�Ĵ�С
					iSkip = 2;
					memcpy(buf+iPos +5,&dwPacketSize,2);
					break;
				case 3:
					iSkip = 4;
					memcpy(buf+iPos +5,&dwPacketSize,4);
					break;
				}
				c= (buf[iPos +3] & 0x06) ; //�ж� Sequence Length Type�Ƿ����
				c = c>> 5 ;	
				switch(c)
				{
				case 0:
					iSkip += 0 ;
					break;
				case 1:
					iSkip += 1;
					break;
				case 2:
					iSkip += 2;
					break;
				case 3:
					iSkip += 4;
					break;
				}
				//��γ������Padding Type = 01 ����Byte��������ַ�Byte�����д��� 
				if(iAsf != dwPacketSize)	//����padding��
				{
					buf[iPos + 5 + iSkip ] = (byte)(dwPacketSize - iAsf); //�޸�asf����padding�ߴ�
					//������Ӧ�ü����iPadlen�Ĵ�С???
					//iPadLen = dwPacketSize - iAsf;
					
				iPadLen = 0;
				} //end of iAsf != dwPacketSize
				else
				{
					assert(buf[iPos + 5 + iSkip] == 0);
				}
				

			}

			if( 0 == iNeedData) //��ǰ���ݰ���Ҫ������Ϊ0����Ҫ���¼������ݰ���������
			{
				if(cSeq < Segment::getPacketsOfSegment() - 1)
				{
					//�������������Ƿ��㹻push�����������������һ����
					iNeedData = PACKET_DATA_SIZE;
				}
				else
				{
					iNeedData = Segment::getLastPacketSize();
				}
				//��ʱiOffset == 0
				assert(0 == iOffset_t);
				//�������һ����δ���ɵ�padding���ݣ���padding���ݲ�����ǰ��cSeqment��
				while(iPadLen >0)
				{
					buf_t[iOffset_t] = 0;
					iOffset_t ++;
					iPadLen --;
				}
			}
			assert(iNeedData >= iOffset_t);
			if(iAvailable > (iNeedData - iOffset_t)) //�������е����ݶ�����������
			{
				if(iNeedData - iOffset_t >= iAsf) //��������ݱ�һ��asf chunk���������Ҫchunk
				{
					//����asf���Ƶ�buf_t
					memcpy(buf_t + iOffset_t,buf + iPos,iAsf);
					iOffset_t += iAsf;
					iPos += iAsf;
					iAsf = len; //len ��ǰasf chunk���ȡ�this is  necessary for next if sentence
					iPadLen = dwPacketSize - len; //padding data length

				}
				else //�ò���һ��asf chunk�Ϳɽ���ǰbut_t����������
				{
					memcpy(buf_t + iOffset_t,buf + iPos,iNeedData - iOffset_t);//����һ��������
					iPos += iNeedData - iOffset_t;
					iAsf -= iNeedData - iOffset_t;//iAsf chunk��ʣ���ֽ�

					iOffset_t += iNeedData - iOffset_t;
				}

				if(len == iAsf && 0 != iPadLen && iOffset_t != iNeedData  ) //��Ҫ��䣬�ҵ�ǰSegment�пռ�
				{
					while( iOffset_t != iNeedData && iPadLen >0 )
					{
						buf_t[iOffset_t] = 0;
						iOffset_t++;
						iPadLen --;
					}
					//ִ��������ѭ���󣬻�����ʣ��һ����padding���ݣ����ʱ��iPadLen>0��iOffset_t == iNeedData
					
				}

				//�������Ƿ�������
				if(iOffset_t == iNeedData)
				{

#if _DEBUG
					if (0 == cSeq)
					{
						assert((char)0x82==buf_t[0] && (char)0 == buf_t[1] && (char)0 == buf_t[2]);
					}
#endif

					lpBuf->srv_PushData(cSeqID,cSeq,buf_t,iNeedData);

					if(cSeq == Segment::getPacketsOfSegment() -1) //���һ����
					{
						//����ID
						cSeq = 0;
						if(cSeqID == 0xFFFF)
							cSeqID = 0;
						else
							cSeqID++;
					}
					else
					{
						cSeq++;
					}
					iOffset_t = 0;
					iNeedData = 0; 
				}
				if(iAsf == len) //�Ѿ�������һ��chunk
				{
					return iPos;
				}		

			} 
			else //���ݲ���
			{
				if (iAsf <= iAvailable)
				{
					memcpy(buf_t+iOffset_t,buf+iPos,iAsf);
					iOffset_t+=iAsf;
					iPos += iAsf;
					iAsf -= iAsf;

				}
				else
				{
					memcpy(buf_t+iOffset_t,buf+iPos,iAvailable);;
					iOffset_t+=iAvailable;
					iPos += iAvailable;
					iAsf -= iAvailable;

				}
				if (iAsf ==0 )
				{
					iPadLen = dwPacketSize - len;
				}
				break;
			}//end of ������

		} //end of ���ݴ���ѭ��

		//����������

		assert(iGet >= iPos);

	
	return iPos;
}
void wme_c::processData(char* ip,WORD port)
 {
	 
	 Buffer* lpBuf = m_buf;
	 SOCKET skt = sys->createSocket(SOCK_STREAM);
	sockaddr_in clientService; 
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = inet_addr( ip);
	clientService.sin_port = htons( port);
	DWORD e;
	if(connect(skt,(SOCKADDR*) &clientService,sizeof(clientService))==SOCKET_ERROR)
	{		
		e=GetLastError();
		return ;
	}
	MemoryStream *stream  =new MemoryStream(2048);
	memset(stream->buf,0,1024);
	stream->write(HTTP_REQ_CMD, static_cast<int>(strlen(HTTP_REQ_CMD)  ));
	stream->write(HTTP_REQ_AGENT,static_cast<int>(strlen(HTTP_REQ_AGENT) ) );
	stream->write(HTTP_REQ_PRAGMA2,static_cast<int>(strlen(HTTP_REQ_PRAGMA2))  ); //�ڶ�������
	stream->writeChar('\r');
	stream->writeChar('\n');

	if(send(skt,stream->buf,static_cast<int>(strlen(stream->buf)),0)==SOCKET_ERROR)
	{
		e = GetLastError();
		
		return ;
	}
	delete stream;
	
	//��������
 	const int BUF_SIZE = 64*1024 +12 ;// 8192; //64KB
	char buf[BUF_SIZE];
	int iGet = 0;
	Sleep(10);
	iGet = recv(skt,buf,17,0); //��ȡ״̬��
	if(0 == iGet)
	{
		e = GetLastError();
		return;
	}
	buf[18]='\0';

	if(strstr(buf,"OK")==NULL) // not got HTTP/1.x 200 OK
	{
		return;
	}
	iGet = recv(skt,buf,BUF_SIZE,0); //����ͷ
	if(iGet == 0)
	{
		iGet = recv(skt,buf,BUF_SIZE,0); //����ͷ
	}
	char* lpLoc = strstr(buf,"\r\n\r\n");
	if(lpLoc != NULL)
		lpLoc += 4;
	memcpy(buf,lpLoc,iGet - (lpLoc - buf )); //�ƶ����ݣ�����httpͷ��
	iGet -= (lpLoc - buf  ); 
	WORD len;
	memcpy(&len,buf+2,2); //get asf header len

	//�����ڶ��δ�����ͷ����
	while(len + 4 > iGet) //���ݲ���ʱ����ȡ����
	{
		int i = recv(skt,buf + iGet,BUF_SIZE - iGet,0);
		if(i == SOCKET_ERROR)
		{
			e = GetLastError();
			return;
		}
		if(0 == i)
		{
			e = GetLastError();
			return;//�����ѶϿ�
		}
		iGet +=i;
	}
	
	memcpy(buf,buf + len,iGet - (len +4)); //����asf ͷ +4 = http streaming 's header
	iGet -= (len +4);

	len = 0;
	int iPos = 0;
	
	static int cSeqID = 0 ;//�ӵ����뿪ʼ �����Ի��塣 //TODO:���Դ�BM�еõ���ǰIDȻ���һ������ʱ��
	byte cSeq = 0;	//live show packet �е����
	int iAsf = len; //��ǰasf chunk��С
	char buf_t [PACKET_DATA_SIZE];
	int iOffset_t = 0;
	int iNeedData = 0 ; //��ǰ���ݰ���Ҫ������
		
	cSeq=0;
	assert(cSeq == 0);
	int iPadLen = 0;// Padding data length
	while(true)  //����ѭ��
	{
		while(iPos < iGet) //���ݴ���ѭ��.iPos��ǰ���ݴ���ָ�룬iGet����ǰ�������ݳ���
		{
			int iAvailable = iGet - iPos; //���ջ����п�������

			if(iAvailable <= 12)
				break; //��������wme�ж�ȡ�������ݡ�
			if((char)0x24 == buf[iPos]  && (char)0x44 == buf[iPos + 1]) //a new asf chunk
			{
				if (memcmp(buf+iPos+2,buf+iPos+10,2)!=0)//len != confirm len
				{
					std::cout << endl << "ERROR!!Read Http Streaming chunk Error!len != confirm len.ERROR!!" << endl;
					closesocket(skt);
					return;
				}
				memcpy(&len,buf + iPos +2 ,2);//�õ����ݵı���С
				len -= 8;//http stream�еĳ���= ���ݳ��ȼ�������ͷ����(8�ֽ�)
				assert(len > 0);
				
				iAsf = len ; 

				//ȥ�� http streaming ͷ ������ͷ����4+8
				iPos += 12;
				iAvailable -= 12;
				//fix asf parse info 's padding length

				
					//����ÿ������0x 82 00 00 ��ʼ���������һ��������asf parse info�ĵ�һ���ֽ�
					int iSkip =0;

					//assert((char)0x82==buf[iPos] && (char)0 == buf[iPos+1] && (char)0 == buf[iPos+2]);

					char c= (buf[iPos +3] & 0x60) ; //�ж� Packet Length Type�Ƿ����
					c = c>> 5 ;	
					
					switch(c)
					{
						case 0:
							iSkip =0;
							break;
						case 1:
							iSkip = 1;
							//TODO:���256�ֽ�һ���������񲻻�����������
#if _DEBUG
							log::WriteLine("Packet Length is 0x01(Byte)");
#endif
							memcpy(buf+iPos +5,&dwPacketSize,1);
							break;
						case 2: //340Kbps���Ҷ�0x5a4�Ĵ�С
							iSkip = 2;
							memcpy(buf+iPos +5,&dwPacketSize,2);
							break;
						case 3:
							iSkip = 4;
							memcpy(buf+iPos +5,&dwPacketSize,4);
							break;
					}
					c= (buf[iPos +3] & 0x06) ; //�ж� Sequence Length Type�Ƿ����
					c = c>> 5 ;	
					switch(c)
					{
						case 0:
							iSkip += 0 ;
							break;
						case 1:
							iSkip += 1;
							break;
						case 2:
							iSkip += 2;
							break;
						case 3:
							iSkip += 4;
							break;
					}
				//��γ������Padding Type = 01 ����Byte��������ַ�Byte�����д��� 
				if(iAsf != dwPacketSize)	//����padding��
				{
					buf[iPos + 5 + iSkip ] = (byte)(dwPacketSize - iAsf); //�޸�asf����padding�ߴ�
					//������Ӧ�ü����iPadlen�Ĵ�С???
					//iPadLen = dwPacketSize - iAsf;

				} //end of iAsf != dwPacketSize
				else
				{
					//assert(buf[iPos + 5 + iSkip] == 0);
				}


			}

			if( 0 == iNeedData) //��ǰ���ݰ���Ҫ������Ϊ0����Ҫ���¼������ݰ���������
			{
				if(cSeq < Segment::getPacketsOfSegment() - 1)
				{
					//�������������Ƿ��㹻push�����������������һ����
					iNeedData = PACKET_DATA_SIZE;
				}
				else
				{
					iNeedData = Segment::getLastPacketSize();
				}
				//��ʱiOffset == 0
				assert(0 == iOffset_t);
				//�������һ����δ���ɵ�padding���ݣ���padding���ݲ�����ǰ��cSeqment��
				while(iPadLen >0)
				{
					buf_t[iOffset_t] = 0;
					iOffset_t ++;
					iPadLen --;
				}
			}
			assert(iNeedData >= iOffset_t);
			if(iAvailable > (iNeedData - iOffset_t)) //�������е����ݶ�����������
			{
				if(iNeedData - iOffset_t >= iAsf) //��������ݱ�һ��asf chunk���������Ҫchunk
				{
					//����asf���Ƶ�buf_t
					memcpy(buf_t + iOffset_t,buf + iPos,iAsf);
					iOffset_t += iAsf;
					iPos += iAsf;
					iAsf = len; //len ��ǰasf chunk���ȡ�this is  necessary for next if sentence
					iPadLen = dwPacketSize - len; //padding data length

				}
				else //�ò���һ��asf chunk�Ϳɽ���ǰbut_t����������
				{
					memcpy(buf_t + iOffset_t,buf + iPos,iNeedData - iOffset_t);//����һ��������
					iPos += iNeedData - iOffset_t;
					iAsf -= iNeedData - iOffset_t;//iAsf chunk��ʣ���ֽ�
					
					iOffset_t += iNeedData - iOffset_t;
				}
				
				if(len == iAsf && 0 != iPadLen && iOffset_t != iNeedData  ) //��Ҫ��䣬�ҵ�ǰSegment�пռ�
				{
					while( iOffset_t != iNeedData && iPadLen >0 )
					{
						buf_t[iOffset_t] = 0;
						iOffset_t++;
						iPadLen --;
					}
					//ִ��������ѭ���󣬻�����ʣ��һ����padding���ݣ����ʱ��iPadLen>0��iOffset_t == iNeedData
				}

				//�������Ƿ�������
				if(iOffset_t == iNeedData)
				{
					
#if _DEBUG
					if (0 == cSeq)
					{
						assert((char)0x82==buf_t[0] && (char)0 == buf_t[1] && (char)0 == buf_t[2]);
					}
#endif
					
					lpBuf->srv_PushData(cSeqID,cSeq,buf_t,iNeedData);

					if(cSeq == Segment::getPacketsOfSegment() -1) //���һ����
					{
						//����ID
						cSeq = 0;
						if(cSeqID == 0xFFFF)
							cSeqID = 0;
						else
							cSeqID++;
					}
					else
					{
						cSeq++;
					}
					iOffset_t = 0;
					iNeedData = 0; 
				}
				
			} 
			else //���ݲ���
			{
				break;
			}//end of ������
			
		} //end of ���ݴ���ѭ��
		
		//����������

		assert(iGet >= iPos);

		if(iGet - iPos >0)
			memcpy(buf,buf + iPos,iGet - iPos);
		iPos = iGet - iPos;
		
		assert(iPos <= BUF_SIZE);

		iGet = recv(skt,buf + iPos,BUF_SIZE - iPos,0); //��������		
		
		assert(iGet <= BUF_SIZE - iPos);

		if (0 == iGet)
			break; //�޿������ݣ������ѹر�
		iGet += iPos; //iGet��ʾ����������
		iPos = 0;
	}//end of ����ѭ��
 }