#pragma once
#include <winsock2.h>
#include <Ws2tcpip.h>
#include "Packet.h"
#include <mmsystem.h>
#include <vector>
#include <list>

#define DEBUG_PRINT 1
									// multicasting addresses

typedef void CALLBACK UserCallBack(void * Data,int iLen);
typedef UserCallBack * pUserCallback;
typedef unsigned long HPACKET;
typedef unsigned long FRAME_ID;
using namespace std;

extern CRITICAL_SECTION  cs_SP;

typedef struct _tagRTPDataHead
{
	FRAME_ID		FrameID;
	HPACKET			PacketID;
	unsigned long   iTotalCount;
	unsigned long	iCurrentCount;
	unsigned long	iTotalSize;
	unsigned long	iMaxSize;
	unsigned long	iSize;
	unsigned long	iControlCount;
	unsigned long	lTimeStemp;

}RTPDataHead,*pRTPDataHead;



class RecvPacketBuffer: public Object
{
public:
	FRAME_ID		FrameID;
	SOCKADDR_IN		Source_sin;
	bool			bComplete;
	bool			bUpdated;
	bool			bOk[256];
	unsigned long	Count;
	unsigned long	TimeStamp;
	unsigned long	iSize;
	unsigned long	TotalCount;
	unsigned long	TotalSize;
	unsigned long	LastUpdated;
	unsigned long 	PacketID[256];
	unsigned long	RetryTimeStamp;

	unsigned char * pData;
public:
	RecvPacketBuffer()
	{
		FrameID = 0;
		Count = 0;
		TotalCount = 0;
		TotalSize = 0;
		LastUpdated = 0;
		pData = NULL;
		iSize = 0;
		RetryTimeStamp = 0;
		bComplete = false;
	};

	~RecvPacketBuffer()
	{

		if(TotalSize)
		{
			delete [] pData ;
			pData = NULL;
		}
	};

	void Init(RTPDataHead * pRTPDataHead,SOCKADDR_IN TempSin)
	{
		unsigned long FirstPacketID;
		Source_sin = TempSin;
		FrameID = pRTPDataHead->FrameID;
		TotalCount = pRTPDataHead->iTotalCount;
		TotalSize = pRTPDataHead->iTotalSize;
		pData = new unsigned char [TotalSize];
		memset(PacketID,0x00,sizeof(PacketID));
		memset(pData,0x00,sizeof(char)*TotalSize);
		bUpdated = true;
		bComplete = false;
		TimeStamp = pRTPDataHead->lTimeStemp;
		FirstPacketID = pRTPDataHead->PacketID - pRTPDataHead->iCurrentCount;
		this->LastUpdated = timeGetTime();
		for(unsigned int i=0;i< pRTPDataHead->iTotalCount;i++)
		{
			PacketID[i] = FirstPacketID+i;
			bOk[i] = false;
		}
	};

	FRAME_ID GetFrameID()
	{
		return FrameID;
	};

	bool AddFrameData(RTPDataHead * pRTPDataHead,unsigned char * pBuffer)
	{
		if(!bOk[pRTPDataHead->iCurrentCount])
		{
			Count++;
			memcpy(pData+pRTPDataHead->iCurrentCount*pRTPDataHead->iMaxSize,pBuffer,pRTPDataHead->iSize);
			this->TimeStamp = pRTPDataHead->lTimeStemp;
			LastUpdated = timeGetTime();
			this->RetryTimeStamp = LastUpdated;
			PacketID[pRTPDataHead->iCurrentCount] = pRTPDataHead->PacketID;
			bOk[pRTPDataHead->iCurrentCount] = true;
			this->bUpdated = true;
			iSize += pRTPDataHead->iSize;
			bOk[pRTPDataHead->iCurrentCount] = true;
			if(Count == TotalCount)
			{
				bComplete = true;
			}
		} else {
			return false;
		}
		return true;
	}
};


class CSendPacketBuffer: public Object
{
public:

	bool			bAck;
	bool			bSend;
	bool			bLock;
	int				iUseCount;
	unsigned long	LastSendTime;

	union
	{
		struct {
			FRAME_ID		FrameID;
			HPACKET			PacketID;
			unsigned long   iTotalCount;
			unsigned long	iCurrentCount;
			unsigned long	iTotalSize;
			unsigned long	iMaxSize;
			unsigned long	iSize;
			unsigned long	iControlCount;
			unsigned long	lTimeStemp;
		};
		RTPDataHead RTPHeader;
	};
	char pBuffer[2000];

public:
	CSendPacketBuffer::CSendPacketBuffer()
	{
		bAck = false;
		LastSendTime = 0;
		iTotalCount		= 0;
		iCurrentCount	= 0;
		iTotalSize		= 0;
		iSize			=0;
		iMaxSize		= 0;
		iUseCount = 0;
		bSend = false;
		bLock = false;
	};

	
	CSendPacketBuffer::~CSendPacketBuffer()
	{

	};
public:
	void AddSendPacketBuffer(FRAME_ID FrameID,HPACKET PacketID, char * pData,int iLen,bool bAck=false )
	{

		this->bAck = bAck;
		this->FrameID = FrameID;
		this->PacketID = PacketID;
		bSend = true;
		lTimeStemp = timeGetTime();
		iSize = iLen;
		memcpy(pBuffer,pData,iLen);
	};
};

class CSockList:public Object
{
protected:
	/*Req 처리 관련*/
	bool				bReqSended;		//Req 전송후 true 상대로 부터 Ack를 받을 경우 false로 전환된다. Req 처리중임을 설정하는 변수
	int					iReqRetryCount;
	unsigned long		SendedReqType;
	unsigned long		ReqTimeStamp;
	unsigned long		LastReqSendTime;//최종 리퀘스트를 전송한 시간을 기록, 일정기간 동안 Ack가 오지 않을경우 다시 Req를 보낸다

	/*Ack 처리 관련*/
	unsigned long		RetryTimeStamp;
	unsigned short		LastRecvReqID;
	unsigned long		LastRecvedReqTime;
	unsigned long		LastSendAckTime;
	unsigned long		LastRecvedReqTimeStamp;
	bool				bReqRecved;
	
	/*Data 전송 처리 관련*/
	bool				bSockBufferLock;
	unsigned int		PacketMax;
	unsigned int		iRecvCount;
	unsigned long		RecvedPacketID[256];
	unsigned int		iSendingCount;
	unsigned int		SleepCount;
	unsigned long		LastRecvDataTime;
	unsigned long		LastRecvDataAckTime;
	unsigned long		TempPacketCount;

	
	/*Recv 처리 관련 변수*/
	unsigned long		LastCompletedFrameID;


	bool				bSending;		// 데이터 보내는 상태 //Req를 순간 true Ack가 전달 확인되는 순간 false :즉 리퀘스트를 받아 완료하기 전까지 는 bProcess가 true가 된다
	unsigned int		iPacketCount;
	unsigned int		iNoReqPacketCount; //리퀘스트 없이 일정한 속도로 던지는 데이터
	unsigned long		WaitTime;		//대기시간 설정
	unsigned long		CurrentTime;
	unsigned long		iBufferCount;	//버퍼 갯수

	unsigned long		iTempSendCount;

	
//	unsigned long
public:
	SOCKET Sock;       // Datagram window socket
	SOCKADDR_IN Target_sin; //타겟의 주소
	CRITICAL_SECTION  cs,cs_pc,cs_rp,*cs_SP;
	CSendPacketBuffer TempSendBuffer[100];
public:

	std::vector<RecvPacketBuffer*> vRecvBuffer;
	std::vector<CSendPacketBuffer*> vSendPacket;
	std::list<CSendPacketBuffer*> vSendPacketNoReq;
	std::list<CSendPacketBuffer*> vSendPacketNoReq_Temp;
	
public:
	CSockList()
	{
		bSockBufferLock = false;
		TempPacketCount = 0;
		LastRecvDataAckTime = 0;
		LastCompletedFrameID = 0; 
		RetryTimeStamp = 0;
		iNoReqPacketCount = 0;
		iBufferCount = 0;
		iSendingCount = 0;
		LastSendAckTime = 0;
		ReqTimeStamp = 0;
		iReqRetryCount=0;
		iPacketCount = 0;
		bReqRecved = false;
		bReqSended	 = false;
		bSending = false;
		WaitTime = 50;			//Ack응답 대기시간을 100ms로 설정
		LastReqSendTime = 0;
		ReqTimeStamp = 0;
		LastRecvedReqTimeStamp = 0;
		PacketMax = 20;
		SleepCount =1;
		iTempSendCount = 0;
		InitializeCriticalSection ( &cs );
		InitializeCriticalSection ( &cs_pc );
		InitializeCriticalSection ( &cs_rp );
	};

	virtual ~CSockList()
	{
		DeleteCriticalSection ( &cs);
		DeleteCriticalSection ( &cs_pc);
		DeleteCriticalSection ( &cs_rp);
		vSendPacket.clear();
		vRecvBuffer.clear();
	};

	void AddSendPacket(RTPDataHead rtpHead ,char  * pData,bool bNoReq,bool bAck = false)
	{
		if(bNoReq)
		{
			EnterCriticalSection (&cs_pc );
			
			if(bSockBufferLock && (rtpHead.iCurrentCount == 0) &&(TempPacketCount < 30))
			{
				bSockBufferLock = false;							
			}

			if(bSockBufferLock)
			{
				LeaveCriticalSection (&cs_pc );
				return ;
			}


			if((TempPacketCount >300) && (rtpHead.iCurrentCount == 0)) 
			{
				bSockBufferLock = true;
				LeaveCriticalSection (&cs_pc );
				return ;
			}
			
			CSendPacketBuffer * pSendPacket = new CSendPacketBuffer;
			pSendPacket->AddSendPacketBuffer(rtpHead.FrameID,rtpHead.PacketID,pData,rtpHead.iSize,bAck);
			iNoReqPacketCount++;
			pSendPacket->RTPHeader = rtpHead;
			vSendPacketNoReq.push_back(pSendPacket);
			LeaveCriticalSection (&cs_pc );
		} else {
			EnterCriticalSection (&cs );
			CSendPacketBuffer * pSendPacket = new CSendPacketBuffer;
			pSendPacket->AddSendPacketBuffer(rtpHead.FrameID,rtpHead.PacketID,pData,rtpHead.iSize,bAck);
			iPacketCount++;
			pSendPacket->RTPHeader = rtpHead;
			vSendPacket.push_back(pSendPacket);
			LeaveCriticalSection (&cs );
		}
		
	};
	void NoReqDataUpdate(unsigned long FrameID,unsigned long PacketCount,unsigned long PacketID[256])
	{
		EnterCriticalSection (&cs_pc );
		CSendPacketBuffer * pSendPacketBuffer;
		unsigned long	CurrentTime = timeGetTime();
		unsigned long	TempTime = CurrentTime;
		unsigned long	iCount = 0;
		int	i;
		bool			bExist = false;

		std::list<CSendPacketBuffer *>::iterator pos;
		pos = vSendPacketNoReq_Temp.begin();
		while(pos != vSendPacketNoReq_Temp.end())
		{
			pSendPacketBuffer = *pos;
			if(TempTime > pSendPacketBuffer->lTimeStemp)
			{
				TempTime = pSendPacketBuffer->lTimeStemp;
			}

			if(pSendPacketBuffer->FrameID <= FrameID)
			{
				TempPacketCount--;
				delete pSendPacketBuffer;
				pos = vSendPacketNoReq_Temp.erase(pos);
			} else {
				bExist = false;
				i = PacketCount;
				while(i>0)
				{
					if((pSendPacketBuffer->PacketID == PacketID[i-1])/* &&((CurrentTime - pSendPacketBuffer->LastSendTime)  > 50 )*/)
					{
						vSendPacketNoReq.push_front(pSendPacketBuffer);
						pos = vSendPacketNoReq_Temp.erase(pos);
						TempPacketCount--;
						iNoReqPacketCount++;
						bExist = true;
						break;
					} 
					i--;
				}
				if(!bExist)
				{
					pos++;
				}
			}
			iCount++;
		}
		

		LeaveCriticalSection (&cs_pc );
	};

	void NoReqDataSending()
	{
		EnterCriticalSection (&cs_pc );

		if(((CurrentTime - LastRecvDataAckTime) >3000) && (TempPacketCount > 0))
		{
			CSendPacketBuffer * pSendPacketBuffer;
			std::list<CSendPacketBuffer *>::iterator pos;
			pos = vSendPacketNoReq_Temp.begin();
			while(pos != vSendPacketNoReq_Temp.end())
			{
				pSendPacketBuffer = *pos;
				delete pSendPacketBuffer;
				pos = vSendPacketNoReq_Temp.erase(pos);
			}

			pos = vSendPacketNoReq.begin();
			while(pos != vSendPacketNoReq.end())
			{
				pSendPacketBuffer = *pos;
				delete pSendPacketBuffer;
				pos = vSendPacketNoReq.erase(pos);
			}
			TempPacketCount = 0;
			iNoReqPacketCount = 0;
		}

		if(iNoReqPacketCount)
		{
			CSendPacketBuffer * pTempBuffer;
			Packet packet;
			unsigned int i;
			std::list<CSendPacketBuffer *>::iterator pos;
			unsigned int TempSleepCount = SleepCount;

			if(TempPacketCount == 0 )
			{
				LastRecvDataAckTime = CurrentTime;
			}

			for( i=0, pos = vSendPacketNoReq.begin();(i<iNoReqPacketCount) ;i++,pos++)
			{
				pTempBuffer= *pos;
				if(pTempBuffer->bAck)
				{
					SendTo(pTempBuffer->pBuffer,pTempBuffer->iSize);
					pos = vSendPacketNoReq.erase(pos);
					iNoReqPacketCount--;
					TempSleepCount--;
					iTempSendCount ++;
				} else {
					pTempBuffer->LastSendTime = CurrentTime;
					vSendPacketNoReq_Temp.push_back(pTempBuffer);
					TempPacketCount++;

					packet.clear();
					packet.id(RTP_CONTROL_SEND_PACKET_DATA_NOREQ);
					packet.writeData2(&pTempBuffer->RTPHeader,sizeof(RTPDataHead));
					packet.writeData2(pTempBuffer->pBuffer,(unsigned short)pTempBuffer->iSize);
					//if((iTempSendCount %8) != 1)
					{
						SendTo(packet.getPacketBuffer(),packet.getPacketSize());
					}
					pos = vSendPacketNoReq.erase(pos);
					iNoReqPacketCount--;
					TempSleepCount--;
					iTempSendCount ++;
				}
					
				if(TempSleepCount == 0 )
				{
					break;
				
				}
				if(pos == vSendPacketNoReq.end())
				{
					break;
				}
				
			
			}
		}
		LeaveCriticalSection (&cs_pc );	
	}

	void StatusOut(int iDest, char *strStat)
	{
	#if 1
		
		//HWND hWndMain = ::AfxGetMainWnd()->m_hWnd;
		if(this->hWndMain != NULL)
		{
			HDC hdc;
			hdc=GetDC(hWndMain);

			TextOutA(hdc,10,15*iDest,strStat,(int)strlen(strStat));
		//		TextOut(hdc,10,15,strStat,wcslen(strStat));

			ReleaseDC(hWndMain, hdc);
			UpdateWindow(hWndMain);
		}
	#endif
	}


	void AddRecvPacketBuffer(RTPDataHead Head,unsigned char * pBuffer)
	{
		bool bExist;
		unsigned long CurrentTime = timeGetTime();

		//StatusOut(6,"AddRecvPacketBuffer");
		EnterCriticalSection (&cs_rp );
		if(LastCompletedFrameID >= Head.FrameID)
		{
			LeaveCriticalSection (&cs_rp );
			return;
		}

		RecvPacketBuffer * pTempRecvPacketBuffer;
		bExist = false;
		std::vector<RecvPacketBuffer *>::iterator pos =  vRecvBuffer.begin();
		for(pos =  vRecvBuffer.begin();pos != vRecvBuffer.end();pos++)
		{
			pTempRecvPacketBuffer = *pos;

			if((pTempRecvPacketBuffer->FrameID == Head.FrameID))
			{
				if(pTempRecvPacketBuffer->AddFrameData(&Head,pBuffer))
				{
					//char strOut[100];
					//sprintf_s(strOut, ("RTP_STREAM_DATA:FrameID:%d PacketID:%d %dms %d/%d  %d         "),Head.FrameID, Head.PacketID,timeGetTime(),Head.iCurrentCount,Head.iTotalCount,Head.iSize);
					//StatusOut(Head.iCurrentCount+5,strOut);
				}
				bExist = true;
				break;
			}
		}

		if(!bExist)
		{
			pTempRecvPacketBuffer = new RecvPacketBuffer;
			pTempRecvPacketBuffer->Init(&Head,Target_sin);

			if(pTempRecvPacketBuffer->AddFrameData(&Head,pBuffer))
			{
				//char strOut[100];
				//sprintf_s(strOut, ("RTP_STREAM_DATA:FrameID:%d PacketID:%d %dms %d/%d  %d         "),Head.FrameID, Head.PacketID,timeGetTime(),Head.iCurrentCount,Head.iTotalCount,pTempRecvPacketBuffer->Count);
				//StatusOut(Head.iCurrentCount+5,strOut);
			}
			vRecvBuffer.push_back(pTempRecvPacketBuffer);
		}
		LeaveCriticalSection (&cs_rp );
	};

	bool SendTo(char * pData,int iLen)
	{
		fd_set	fdWrite  = { 0 };
		TIMEVAL	stTime;
		TIMEVAL	*pstTime = NULL;

		DWORD dwTimeout = 1;
		if ( INFINITE != dwTimeout ) {
			stTime.tv_sec = dwTimeout/1000;
			stTime.tv_usec = dwTimeout % 1000;
			pstTime = &stTime;
		}

		SOCKET s = (SOCKET) Sock;
		// Set Descriptor
		if ( !FD_ISSET( s, &fdWrite ) )
			FD_SET( s, &fdWrite );

		// Select function set write timeout
		DWORD dwBytesWritten = 0L;
	//	int res = select((int) s+1, NULL, &fdWrite, NULL, pstTime );
	//	if(res >0)
		{
			if (sendto (Sock, 
					  (char *)pData,
					  iLen,
					  0,
					  (struct sockaddr FAR *) &Target_sin,
					  sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
			{
				return FALSE;
			}
		}
		return true;
	}

	void EraseAllBuffer()
	{
		EnterCriticalSection (&cs );
		CSendPacketBuffer * pSendPacketBuffer;
		std::vector<CSendPacketBuffer *>::iterator pos;
		pos = vSendPacket.begin();
		while(pos!= vSendPacket.end())
		{
			pSendPacketBuffer = *pos;
			iPacketCount--;
			delete pSendPacketBuffer;
			pos = vSendPacket.erase(pos) ;
		}
		RecvPacketBuffer * pRecvPacketBuffer;
		std::vector<RecvPacketBuffer *>::iterator pos2;
		pos2 = vRecvBuffer.begin();
		while(pos2 != vRecvBuffer.end())
		{
			pRecvPacketBuffer = *pos2;
			delete pRecvPacketBuffer;
			pos2 = vRecvBuffer.erase(pos2) ;
		}
		LeaveCriticalSection (&cs );
	}

	void ErasePacketBuffer(unsigned long	PacketId[])
	{
		
		unsigned int i;
		unsigned int iCount;
		CSendPacketBuffer  TempSendBuffer1[100];

		iCount = 0;
		for( i=0;i<iSendingCount ;i++)
		{
			if(PacketId[i] == 0)
			{
				TempSendBuffer1[iCount] = TempSendBuffer[i];
				iCount++;			
			} else {
			}
			
		}

		for(i=0;i<iCount;i++)
		{
			TempSendBuffer[i] = TempSendBuffer1[i];
		}
		iBufferCount = iCount;
	}

	void PacketParser(char * pData,int iLen)
	{
		
		Packet packet;
		packet.copyToBuffer(pData,iLen);
		switch(packet.id())
		{
		case RTP_CONTROL_SEND_PACKET_START_REQ:
			{
				unsigned short			sSize;
				unsigned long	TimeStamp;
				
				//StatusOut(6,"RTP_CONTROL_SEND_PACKET_START_REQ");
				packet.readData(&sSize,2);
				packet.readData(&TimeStamp,sSize);
				if(LastRecvedReqTimeStamp < TimeStamp)
				{
					LastRecvReqID = RTP_CONTROL_SEND_PACKET_START_REQ;
					LastRecvedReqTimeStamp = TimeStamp;
					
					memset(RecvedPacketID,0x00,sizeof(RecvedPacketID));
					LastSendAckTime = timeGetTime();
					iRecvCount =0;
				}
				LastRecvedReqTime = timeGetTime();	
				AckSendingProcess();
			}
			break;
		case RTP_CONTROL_SEND_PACKET_START_ACK:
			{
				//StatusOut(6,"RTP_CONTROL_SEND_PACKET_START_ACK");
				unsigned short			sSize;
				unsigned long	TimeStamp;
				packet.readData(&sSize,2);
				packet.readData(&TimeStamp,sSize);
				if((!bSending)  && (ReqTimeStamp == TimeStamp) && (RTP_CONTROL_SEND_PACKET_START_REQ == SendedReqType))
				{
					LastReqSendTime = 0;
					ReqTimeStamp = 0;
					bSending = true;
					iReqRetryCount = 0;
				}
			}
			break;
		case RTP_CONTROL_SEND_PACKET_DATA:
			{
				//StatusOut(6,"RTP_CONTROL_SEND_PACKET_DATA");
				//unsigned char pBuffer[2048];
				RTPDataHead TempRTPHead;
				packet.readData2(&TempRTPHead);
				unsigned char * pBuffer = new unsigned char [TempRTPHead.iSize];
				packet.readData2(pBuffer);
				AddRecvPacketBuffer(TempRTPHead,pBuffer);
				delete [] pBuffer;

				RecvedPacketID[TempRTPHead.iControlCount] = TempRTPHead.PacketID;
				iRecvCount++;
			}
			break;
		case RTP_CONTROL_SEND_PACKET_DATA_NOREQ:
			{
				//StatusOut(6,"RTP_CONTROL_SEND_PACKET_DATA");
				//unsigned char pBuffer[2048];
				LastRecvDataTime = timeGetTime();
				RTPDataHead TempRTPHead;
				packet.readData2(&TempRTPHead);
				unsigned char * pBuffer = new unsigned char [TempRTPHead.iSize];
				packet.readData2(pBuffer);
				AddRecvPacketBuffer(TempRTPHead,pBuffer);
				delete [] pBuffer;
			}
			break;
		case RTP_CONTROL_SEND_PACKET_DATA_NOREQ_ACK:
			{
				unsigned long TempFrameID;
				unsigned long PacketCount;
				unsigned long PacketID[256];
				LastRecvDataAckTime = timeGetTime();
				packet.readData2(&TempFrameID);
				packet.readData2(&PacketCount);
				packet.readData2(PacketID);
				NoReqDataUpdate(TempFrameID,PacketCount,PacketID);
				//int a=0;

				//packet.writeData2(&LastCompletedPacketTimeStamp,4);
				//packet.writeData2(&PacketCount,4);
				//packet.writeData2(PacketID,sizeof(PacketID));
			}
			break;
		case RTP_CONTROL_SEND_PACKET_END_REQ:
			{
			//	StatusOut(6,"RTP_CONTROL_SEND_PACKET_END_REQ");
				unsigned short			sSize;
				unsigned long	TimeStamp;
				packet.readData(&sSize,2);
				packet.readData(&TimeStamp,sSize);
				if(LastRecvedReqTimeStamp < TimeStamp)
				{
					LastSendAckTime = 0;
					LastRecvReqID = RTP_CONTROL_SEND_PACKET_END_REQ;
					LastRecvedReqTimeStamp = TimeStamp;
				}
				LastRecvedReqTime = timeGetTime();	
				AckSendingProcess();
			}
			break;
		case RTP_CONTROL_SEND_PACKET_END_ACK:
			{
			//	StatusOut(6,"RTP_CONTROL_SEND_PACKET_END_ACK");
				unsigned short			sSize;
				unsigned long	lCount;
				unsigned long	PacketId[256];
				unsigned long	TimeStamp;

				packet.readData(&sSize,2);
				packet.readData(&TimeStamp,sSize);
				packet.readData2(&lCount);
				if( (ReqTimeStamp == TimeStamp) && (RTP_CONTROL_SEND_PACKET_END_REQ == SendedReqType))
				{
					
					std::vector<CSendPacketBuffer *>::iterator pos;
					packet.readData2(PacketId);
					ErasePacketBuffer(PacketId);
					LastReqSendTime = 0;
					ReqTimeStamp = 0;
					bSending = false;
					iReqRetryCount = 0;
					iSendingCount = 0;
				}
			}
			break;
		}
	}

	void ReqSendingProcess()
	{
		EnterCriticalSection (&cs );
		CurrentTime = timeGetTime();
		if((iPacketCount > 0) && (iSendingCount == 0))																											//전송할 패킷이 있을 때 
		{
			if(!bReqSended || (CurrentTime - LastReqSendTime > WaitTime))																	//현재 데이터 전송상태도 아니고, 리퀘스트를 전송한 상태도 아닐경우 보낼 패킷의 정보를 전송하는 리퀘스트를 보낸다.
			{
				if(iReqRetryCount == 0)	
				{
					unsigned int i;
					ReqTimeStamp = CurrentTime;						//첫번째 시도시에 생성시간을 저장해 중복 처리를 방지한다.
					bReqSended = true;
					iBufferCount  = 0;
					CSendPacketBuffer * pTempBuffer;
					std::vector<CSendPacketBuffer *>::iterator pos;
					if(iPacketCount)
					{
						if((iPacketCount < (PacketMax - iBufferCount)))
						{
							for( i=0, pos = vSendPacket.begin();(i<iPacketCount) ;i++,pos++)
							{
								
								TempSendBuffer[i] = **pos;
								pTempBuffer= *pos;
								pos = vSendPacket.erase(pos);
								iPacketCount--;
								iBufferCount++;
								if(pTempBuffer)
								{
									delete pTempBuffer;
								}
								if(pos == vSendPacket.end())
								{
									break;
								}
							
							}
						} else {
							for( i=0, pos = vSendPacket.begin();i<(PacketMax - iBufferCount) ;i++,pos++)
							{
								TempSendBuffer[i] = **pos;
								pTempBuffer= *pos;
								vSendPacket.erase(pos);
								iPacketCount--;
								iBufferCount++;
								if(pTempBuffer)
								{
									delete pTempBuffer;
								}
							}
						}
					}
				}
				if(iReqRetryCount < 400)
				{
					
					SendedReqType = RTP_CONTROL_SEND_PACKET_START_REQ;
					iReqRetryCount ++;
					Packet packet;
					packet.id(RTP_CONTROL_SEND_PACKET_START_REQ);
					packet.writeData2(&ReqTimeStamp,4);
					LastReqSendTime  = CurrentTime;
					
					SendTo(packet.getPacketBuffer(),packet.getPacketSize());
				} else {
					EraseAllBuffer();
					iReqRetryCount = 0;
					iPacketCount = 0;
					iSendingCount = 0;
				}

			}
		} else if(iSendingCount > 0){
			if(!bReqSended || (CurrentTime - LastReqSendTime > WaitTime))																	//현재 데이터 전송상태도 아니고, 리퀘스트를 전송한 상태도 아닐경우 보낼 패킷의 정보를 전송하는 리퀘스트를 보낸다.
			{
				SendedReqType = RTP_CONTROL_SEND_PACKET_END_REQ;				
				if(iReqRetryCount == 0)
				{
					ReqTimeStamp = CurrentTime;						//첫번째 시도시에 생성시간을 저장해 중복 처리를 방지한다.
					bReqSended = true;
				}
				iReqRetryCount ++;
				Packet packet;
				packet.id(RTP_CONTROL_SEND_PACKET_END_REQ);
				packet.writeData2(&ReqTimeStamp,4);
				LastReqSendTime  = CurrentTime;
				SendTo(packet.getPacketBuffer(),packet.getPacketSize());
				
			}
		}
		LeaveCriticalSection (&cs );
	}

	bool DataSending()
	{
		CurrentTime = timeGetTime();
		Packet packet;
		int i;
		int TempSleepCount;
		EnterCriticalSection (&cs );

		if((iSendingCount >= iBufferCount))
		{
			LeaveCriticalSection (&cs );
			return false;
		}
		
		TempSleepCount = SleepCount;

		if(((iSendingCount+SleepCount) >= iBufferCount) )
		{
			TempSleepCount = 1;
		}
		
		for(i=0;i<TempSleepCount;i++)
		{
			
			TempSendBuffer[iSendingCount].iControlCount = i;
			packet.clear();
			packet.id(RTP_CONTROL_SEND_PACKET_DATA);
			packet.writeData2(&TempSendBuffer[iSendingCount].RTPHeader,sizeof(RTPDataHead));
			packet.writeData2(TempSendBuffer[iSendingCount].pBuffer,(unsigned short)TempSendBuffer[iSendingCount].iSize);
			SendTo(packet.getPacketBuffer(),packet.getPacketSize());
			iSendingCount ++;
		}

		LeaveCriticalSection (&cs );
		return true;
	}

	void AckSendingProcess()
	{
		//EnterCriticalSection (&cs );		
		//CurrentTime = timeGetTime();
		//if(((CurrentTime-LastRecvedReqTime) < 100) && ((CurrentTime - LastSendAckTime) > 4 ))
		{

			switch(LastRecvReqID)
			{
			case RTP_CONTROL_SEND_PACKET_START_REQ:
				{
					Packet packet;
					packet.id(RTP_CONTROL_SEND_PACKET_START_ACK);
					packet.writeData2(&LastRecvedReqTimeStamp,4);
					LastSendAckTime  = CurrentTime;
					SendTo(packet.getPacketBuffer(),packet.getPacketSize());
				}
				break;
			case RTP_CONTROL_SEND_PACKET_END_REQ:
				{
					unsigned long lCount;
					Packet packet;
					packet.id(RTP_CONTROL_SEND_PACKET_END_ACK);
					packet.writeData2(&LastRecvedReqTimeStamp,4);
					LastSendAckTime  = CurrentTime;
					lCount = iRecvCount;
					packet.writeData2(&lCount,4);

					packet.writeData2(RecvedPacketID,sizeof(RecvedPacketID));
					SendTo(packet.getPacketBuffer(),packet.getPacketSize());
				}
				break;
			}
		}
		//LeaveCriticalSection (&cs );
	}

	void Process()
	{
		CurrentTime = timeGetTime();	
		NoReqDataSending();
		if(!bSending) 
		{
			ReqSendingProcess();
			
		} else {
			bSending = DataSending(); //데이터 전송 플래그가 세팅되면 해당하는 데이터를 보낸다.
		}
		//AckSendingProcess();
	}

	bool GetRecvCompletedPacket(Packet * pPacket)
	{
		bool bExist;
		unsigned long CurrentTime = timeGetTime();


/*
		EnterCriticalSection (&cs);
		bExist = false;
		vector<RecvPacketBuffer *>::iterator pos =  vRecvBuffer.begin();
		if(pos== vRecvBuffer.end()) 
		{
			LeaveCriticalSection (&cs );
			return false;
		}
		RecvPacketBuffer * pRecvPacketBuffer = *pos;
		if(pRecvPacketBuffer->bComplete)
		{
			pPacket->copyToBuffer((char *)pRecvPacketBuffer->pData,pRecvPacketBuffer->iSize);
			delete pRecvPacketBuffer;
			vRecvBuffer.erase(pos);
			LeaveCriticalSection (&cs );
			return true;			
		} else if((CurrentTime - pRecvPacketBuffer->LastUpdated)>5000) {
			delete pRecvPacketBuffer;
			vRecvBuffer.erase(pos);
			LeaveCriticalSection (&cs );
			return false;
		}
		LeaveCriticalSection (&cs);
*/
		unsigned int i;
		unsigned long PacketCount;
		unsigned long PacketID[50];
		bool	 bRet = false;
		EnterCriticalSection (&cs_rp);
		PacketCount = 0;
		bExist = false;
		std::vector<RecvPacketBuffer *>::iterator pos =  vRecvBuffer.begin();
		if(pos== vRecvBuffer.end()) 
		{
			LeaveCriticalSection (&cs_rp );
			return false;
		}
		RecvPacketBuffer * pRecvPacketBuffer = *pos;

		if(pRecvPacketBuffer->FrameID <= LastCompletedFrameID)
		{
			delete pRecvPacketBuffer;
			pos = vRecvBuffer.erase(pos);
			LeaveCriticalSection (&cs_rp);
			return false;
		}

		if(pRecvPacketBuffer->bComplete)
		{
			LastCompletedFrameID = pRecvPacketBuffer->FrameID;
			pPacket->copyToBuffer((char *)pRecvPacketBuffer->pData,pRecvPacketBuffer->iSize);
			delete pRecvPacketBuffer;
			vRecvBuffer.erase(pos);
			bRet = true;
		} else {
			//char strOut[100];
			//sprintf_s(strOut, ("First Recv Buffer State :%d/%d   Current Time Stamp:%d Last Time Stamp:%d     "),pRecvPacketBuffer->Count , pRecvPacketBuffer->TotalCount , pRecvPacketBuffer->FrameID , LastCompletedFrameID);
			//StatusOut(5,strOut);
		}
		
		memset(PacketID,0x00,sizeof(PacketID));
		if((CurrentTime - RetryTimeStamp)>300)
		{
			RetryTimeStamp = CurrentTime;
			while(pos != vRecvBuffer.end())
			{
				pRecvPacketBuffer = *pos;
				if(((CurrentTime - pRecvPacketBuffer->LastUpdated)>5) && (!pRecvPacketBuffer->bComplete))  //나중에 조정
				{
					for(i=0;i<pRecvPacketBuffer->TotalCount;i++)
					{
						if(!pRecvPacketBuffer->bOk[i])
						{
							if(PacketCount > 29)
							{
								break;
							} else {
								PacketID[PacketCount] = pRecvPacketBuffer->PacketID[i];
								PacketCount++;
							}
						}
					}
				}
				pos++;
			}


			if(((CurrentTime - LastRecvDataTime ) > 3000) && (PacketCount > 0) )
			{
				pos =  vRecvBuffer.begin();
				while(pos != vRecvBuffer.end())
				{
					pRecvPacketBuffer = *pos;
					delete pRecvPacketBuffer;
					pos = vRecvBuffer.erase(pos);

				}

			} else 	{
				char strOut[100];
				sprintf_s(strOut, ("RTP_STREAM_DATA Error Count:%d         "),PacketCount);
				StatusOut(7,strOut);

				Packet packet;
				packet.id(RTP_CONTROL_SEND_PACKET_DATA_NOREQ_ACK);
				packet.writeData2(&LastCompletedFrameID,4);
				packet.writeData2(&PacketCount,4);
				packet.writeData2(PacketID,sizeof(PacketID));
				RTPDataHead  TempHead;
				TempHead.iSize = packet.getPacketSize();
				EnterCriticalSection (&cs);
				//Sleep(1);
				SendTo(packet.getPacketBuffer(),packet.getPacketSize());
				LeaveCriticalSection (&cs);
				//AddSendPacket(TempHead,packet.getPacketBuffer(),true,true);

			}
		}
		
		LeaveCriticalSection (&cs_rp);

		return bRet;
	}
};

class RTP_Socket
{
public:
	unsigned long	TotalSize;
	HWND hWndMain ;
	unsigned long FrameID,PacketID;
	SOCKET Sock;       // Datagram window socket
	CRITICAL_SECTION  cs1,cs_pc,cs_rp,cs_SP;
public:
	RTP_Socket()
	{
		TotalSize = 0;
		PacketID = 0;
		FrameID = 0;
		
		InitializeCriticalSection ( &cs_SP );
		InitializeCriticalSection ( &cs1 );
		InitializeCriticalSection ( &cs_pc );
		InitializeCriticalSection ( &cs_rp );
	}
	~RTP_Socket()
	{
		DeleteCriticalSection ( &cs_SP );		
		DeleteCriticalSection ( &cs1 );		
		DeleteCriticalSection ( &cs_pc );
		DeleteCriticalSection ( &cs_rp );
		vSockList.clear();
	}
public:
	virtual void RTPDataPacketParser(SOCKADDR_IN temp_sin,Packet * pPacket)
	{
	};

	void InitSockList()
	{
		EnterCriticalSection ( &cs1 );
		
			CSockList * pSockList;
			std::vector<CSockList *>::iterator pos =  vSockList.begin();
			for(pos =  vSockList.begin();pos != vSockList.end();pos++)
			{
				pSockList = *pos;
				pSockList->EraseAllBuffer();
				delete pSockList;
			}
			vSockList.clear();
			
		LeaveCriticalSection ( &cs1 );
	}

	void ClearOldPacket() //  패킷의 변화 점검
	{
	
	}

	void Sending()
	{
		EnterCriticalSection ( &cs1 );
		CSockList * pSockList;
		std::vector<CSockList *>::iterator pos =  vSockList.begin();
		for(pos =  vSockList.begin();pos != vSockList.end();pos++)
		{
			pSockList = *pos;
			pSockList->Process();
		}
		LeaveCriticalSection ( &cs1 );
	};

	void RecvPakcetUpadate()
	{
		EnterCriticalSection ( &cs1 );
		CSockList * pSockList;
		std::vector<CSockList *>::iterator pos =  vSockList.begin();
		for(pos =  vSockList.begin();pos != vSockList.end();pos++)
		{
			pSockList = *pos;
			Packet packet;
			if(pSockList->GetRecvCompletedPacket(&packet))
			{
				RTPDataPacketParser(pSockList->Target_sin,&packet);
				TotalSize += packet.getPacketSize();
				
				char strOut[100];
				sprintf_s(strOut, ("Total %d  kb       "),TotalSize/1024);
				pSockList->StatusOut(2,strOut);
			}
		}	
		LeaveCriticalSection ( &cs1 );
	}

	void Process()
	{
		EnterCriticalSection ( &cs1 );
			Sending();
			RecvPakcetUpadate();	
			ClearOldPacket();
		LeaveCriticalSection ( &cs1 );
	}

public:
	CSockList* GetTarget(SOCKADDR_IN target_sin)
	{
		CSockList * pSockAddrTarget;
		EnterCriticalSection ( &cs1 );
		std::vector<CSockList *>::iterator pos =  vSockList.begin();
		for(pos =  vSockList.begin();pos != vSockList.end();pos++)
		{
			pSockAddrTarget = *pos;

			if((pSockAddrTarget->Target_sin.sin_addr.S_un.S_addr == target_sin.sin_addr.S_un.S_addr) &&(pSockAddrTarget->Target_sin.sin_port== target_sin.sin_port) )
			{
				LeaveCriticalSection ( &cs1 );
				return	pSockAddrTarget;
			}
		}
		
		pSockAddrTarget = new CSockList;
		pSockAddrTarget->hWndMain =this->hWndMain;
		pSockAddrTarget->cs_SP = &this->cs_SP;
		pSockAddrTarget->Sock = Sock;
		pSockAddrTarget->Target_sin = target_sin;
		vSockList.push_back(pSockAddrTarget);
		LeaveCriticalSection ( &cs1 );
		return pSockAddrTarget;
	}

	void AddSendPacket(char * pBuffer,unsigned long iLen,SOCKADDR_IN * pTarget_sin,int toCount = 1,bool bNoReq = false)
	{
		EnterCriticalSection ( &cs1 );
		unsigned int i;
		unsigned long iBufferSize;
		unsigned iTotalCount,iRemainder;
		char buffer[2000];
		unsigned long CurrentTime = timeGetTime();

		FrameID ++;
		if(FrameID  >= ULONG_MAX)	FrameID = 1;

		iBufferSize = 1400;
		iTotalCount = iLen / iBufferSize;
		iRemainder	= iLen % iBufferSize;

		for( i = 0; i < iTotalCount ; i++)
		{
			PacketID++;
			if(PacketID  >= ULONG_MAX)	PacketID = 1;
			RTPDataHead rtpHead;

			memcpy(buffer,pBuffer+i*iBufferSize,iBufferSize);
			
				
			rtpHead.lTimeStemp = CurrentTime;
			rtpHead.FrameID = FrameID;
			rtpHead.PacketID = PacketID;
			rtpHead.iTotalSize  = iLen;
			rtpHead.iSize = iBufferSize;
			rtpHead.iMaxSize = iBufferSize;
			if(iRemainder>0)
			{
				rtpHead.iTotalCount = iTotalCount+1;
			} else {
				rtpHead.iTotalCount = iTotalCount;
			}
			rtpHead.iCurrentCount = i;

			for(int i=0;i< toCount;i++)
			{
				GetTarget(pTarget_sin[i])->AddSendPacket(rtpHead,buffer,bNoReq);		
			}
		}

		if(iRemainder>0)
		{
			PacketID++;
			if(PacketID  >= ULONG_MAX)	PacketID = 1;
			RTPDataHead rtpHead;
			memcpy(buffer,pBuffer+iTotalCount*iBufferSize,iRemainder);

			rtpHead.lTimeStemp = CurrentTime;
			rtpHead.FrameID = FrameID;
			rtpHead.PacketID = PacketID;
			rtpHead.iTotalSize  = iLen;
			rtpHead.iSize = iRemainder;
			rtpHead.iMaxSize = iBufferSize;
			if(iRemainder>0)
			{
				rtpHead.iTotalCount = iTotalCount+1;
			} else {
				rtpHead.iTotalCount = iTotalCount;
			}
			rtpHead.iCurrentCount = iTotalCount;


			for(int i=0;i< toCount;i++)
			{
				GetTarget(pTarget_sin[i])->AddSendPacket(rtpHead,buffer,bNoReq);		
			}
		}
		LeaveCriticalSection ( &cs1 );	
	};

	void RecvPacketProcess(SOCKADDR_IN target_sin,char * pData,int iLen)
	{
		EnterCriticalSection (&cs1);
			GetTarget(target_sin)->PacketParser(pData,iLen);		
		LeaveCriticalSection (&cs1);	
	};

public:
	std::vector<CSockList*> vSockList;

};

struct IIocpProcessThread
{
	 virtual void ProcessingThread(LPVOID lParam)=0;
};

class CSock_Dgram:public IIocpProcessThread,public RTP_Socket
{
private:

public:
	CRITICAL_SECTION  cs;

	HANDLE g_hCompletionPort;
	OVERLAPPED Overlapped;
	HANDLE g_hReadEvent;

	bool bMultiCast;
	bool bSendLock;
	CSock_Dgram(void);
	~CSock_Dgram(void);

	int iTTL;


	char buffer[PACKETBUFFERSIZE];
//	SOCKET Sock;       // Datagram window socket
	SOCKADDR_IN bind_sin;					//  socket's address
	SOCKADDR_IN target_sin;					// Holds the source address upon 
	ip_mreq m_mrMReq;

	bool	m_bConnectState;
	DWORD	m_receivedPacketSize;

									    // recvfrom function returns
	WSADATA WSAData;                    // Contains details of the 
									    // Winsock implementation
	WSANETWORKEVENTS event; // Event Storage Structure
	WSAEVENT hRecvEvent;

	void * m_pWorkProcess;
	void  ProcessingThread(LPVOID lParam);
	void  SendingThread(LPVOID lParam);	

	//static UINT THConnectServer(LPVOID lParam);
	static DWORD __stdcall THSendStarter(LPVOID lParam);
	static DWORD __stdcall THConnectServer(PVOID pvParam);

	//void ProcessingThread(void);

public:
	void Init(void);

	int Connect();
	int SetSender(char * Address , int Portid);
	int SetReceiver(char * Address , int Port);
	int UpdatePacket();
	int RecvFrom(void);
	int Close(void);
	int SendTo(unsigned char * Data , int iLen);
	int SendTo(SOCKADDR_IN * target_in , char * Data , int iLen);
	void RTPControlPacketParser(SOCKADDR_IN temp_sin,Packet * pPacket);
	void RTPDataPacketParser(SOCKADDR_IN temp_sin,Packet * pPacket);
	void StatusOut(int iDest, char *strStat);
	
};
