#include "StdAfx.h"
#include "Sock_Dgram.h"
#include <winsock.h>
#include <Mmsystem.h>

#include "StdAfx.h"
#include <assert.h>
#include "WorkProcess.h"

#pragma comment(lib,"ws2_32.lib")

typedef unsigned (__stdcall *PTHREAD_START) (void *);

#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, \
   pvParam, fdwCreate, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                      \
         (void *)        (psa),                     \
         (unsigned)      (cbStack),                 \
         (PTHREAD_START) (pfnStartAddr),            \
         (void *)        (pvParam),                 \
         (unsigned)      (fdwCreate),               \
         (unsigned *)    (pdwThreadId)))

  //char buffer[PACKETBUFFERSIZE];               // ASCII string 
  TCHAR szError[100];   

//  SOCKADDR_IN CSock_Dgram::target_sin;               // Holds the source address upon 

static int EraseCount ;
static CRITICAL_SECTION  cs_SP;
CSock_Dgram::CSock_Dgram(void)
{
	bMultiCast = false;
	iTTL = 128;
	Sock = INVALID_SOCKET;       // Datagram window socket
	bSendLock= false;
	InitializeCriticalSection ( &cs );
}

CSock_Dgram::~CSock_Dgram(void)
{
	DeleteCriticalSection ( &cs );
}

void setnonblock(int sock)
{
#ifdef WIN32
	unsigned long fl = 1;
	if (ioctlsocket(sock, FIONBIO, &fl) < 0)
	{
	//	perrdie("socketioctl");
	}
#else
	int fl = fcntl(sock, F_GETFL);
	if (fl < 0)
		perrdie("fcntl");
	if (fcntl(sock, F_SETFL, fl | O_NONBLOCK) < 0)
		perrdie("fcntl");
#endif
}


void CSock_Dgram::Init(void)
{
	bSendLock= false;
	m_bConnectState=false;
	Connect();
}

int CSock_Dgram::Connect()
{

	BOOL fFlag = TRUE;

	if (WSAStartup (MAKEWORD(2,3), &WSAData) != 0) 
	{
		wsprintf (szError, TEXT("WSAStartup failed! Error: %d"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		return FALSE;
	}

	// Create a datagram socket, Sock.
	if ((Sock = socket (AF_INET, SOCK_DGRAM,0)) == INVALID_SOCKET) 
	{
		wsprintf (szError, TEXT("Allocating socket failed! Error: %d"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		return FALSE;
	}

	setnonblock((int)Sock);

	int sndbuf =  0x80000;
	int sndlen = sizeof(sndbuf); 

	if (setsockopt(Sock,SOL_SOCKET,SO_SNDBUF, (char*)&sndbuf , sndlen)  == SOCKET_ERROR) 
	{
		wsprintf (szError, TEXT("setsockopt() SO_SNDBUF failed, Err: %d\n"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		
	}


	if (setsockopt(Sock,SOL_SOCKET,SO_RCVBUF, (char*)&sndbuf , sndlen)  == SOCKET_ERROR) 
	{
		wsprintf (szError, TEXT("setsockopt() SO_RCVBUF failed, Err: %d\n"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		
	}
//	int ERR=setsockopt(Sock,SOL_SOCKET,SO_SNDBUF, (char*)&sndbuf , sndlen);
//	 ERR=setsockopt(Sock,SOL_SOCKET,SO_RCVBUF, (char*)&sndbuf , sndlen);
//	 ERR=getsockopt(Sock,SOL_SOCKET,SO_TYPE , (char*)&sndbuf , &sndlen);
	

	if (setsockopt(Sock,SOL_SOCKET,SO_REUSEADDR, (char *)&fFlag, sizeof(fFlag)) == SOCKET_ERROR) 
	{
		wsprintf (szError, TEXT("setsockopt() SO_REUSEADDR failed, Err: %d\n"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		
	}
	setnonblock((int)Sock);

	
//	hRecvEvent = WSACreateEvent(); // Init WinSock And Return Handle
//	WSAEventSelect(Sock,hRecvEvent,FD_WRITE|FD_READ|FD_CLOSE); //

//	iPacketCount = 0;

	

	return TRUE;
}

int CSock_Dgram::SetSender(char * Address , int Port)
{
    struct hostent *host = NULL;

	if(bMultiCast )
	{
		setsockopt(Sock,IPPROTO_IP,IP_MULTICAST_TTL, (char FAR *)&iTTL, sizeof(int)) ;
	}

	target_sin.sin_family = AF_INET;
	target_sin.sin_port = htons (Port);  
	target_sin.sin_addr.s_addr = inet_addr (Address);

	if (target_sin.sin_addr.s_addr == INADDR_NONE)
	{
		host = gethostbyname(Address);
		if (host == NULL)
		{
			return 1;
		}
		CopyMemory(&target_sin.sin_addr, host->h_addr_list[0],host->h_length);
	}

	return TRUE;
}

int CSock_Dgram::SetReceiver(char * Address , int Port)
{
	// Fill out the local socket's address information.


	if(m_bConnectState) return 0;

	bind_sin.sin_family = AF_INET;
	bind_sin.sin_port = htons (Port);  
	bind_sin.sin_addr.s_addr = htonl (INADDR_ANY);

	/*
	if (bind (Sock, 
					(struct sockaddr FAR *) &bind_sin, 
					sizeof (bind_sin)) == SOCKET_ERROR) 
	{
		wsprintf (szError, TEXT("Binding socket failed! Error: %d"), 
				  WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
		closesocket (Sock);
		return FALSE;
	}
	*/

	if(bMultiCast )
	{		

		/*
		if (bind (Sock, 
					(struct sockaddr FAR *) &bind_sin, 
					sizeof (bind_sin)) == SOCKET_ERROR) 
		{
			wsprintf (szError, TEXT("Binding socket failed! Error: %d"), 
					  WSAGetLastError ());
			MessageBox (NULL, szError, TEXT("Error"), MB_OK);
			closesocket (Sock);
			return FALSE;
		}
		*/

		// Associate the local address with Sock.
		m_mrMReq.imr_multiaddr.s_addr = inet_addr(Address);	// group addr 
		m_mrMReq.imr_interface.s_addr = htons(INADDR_ANY);		// use default 
		if(setsockopt(Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char FAR *)&m_mrMReq, sizeof(m_mrMReq)) < 0)
		return FALSE;
	}

	m_bConnectState = true;

	SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_TIME_CRITICAL); 

	HANDLE hThread;
	DWORD dwThreadId=0;		
	hThread = BEGINTHREADEX(NULL,0,THConnectServer,this,0,&dwThreadId);
	SetThreadPriority (hThread , THREAD_PRIORITY_TIME_CRITICAL);
	CloseHandle(hThread);

	for(int i=0;i<1;i++)
	{
		hThread = BEGINTHREADEX(NULL,0,THSendStarter,this,0,&dwThreadId);
		SetThreadPriority (hThread , THREAD_PRIORITY_TIME_CRITICAL);
		CloseHandle(hThread );
	}
	
	

	return 3;
}

int CSock_Dgram::UpdatePacket()
{
	/*
	Packet packet;

	int iCount;
	char * buffer = NULL;
	short sSize = 0;
	bool bPacket ;

	bPacket = false;
	

	EnterCriticalSection ( &cs );
	iCount = iPacketCount;
	LeaveCriticalSection ( &cs );

	while(iCount>0)
	{

		EnterCriticalSection ( &cs );
			vector<char *>::iterator pos =  Packetbuffers.begin();
			buffer = *pos;
			Packetbuffers.erase(pos);
			iPacketCount--;
		LeaveCriticalSection ( &cs );


		memcpy(&sSize,buffer,2); 
		packet.copyToBuffer(buffer+2,sSize);
		delete [] buffer;
		((WorkProcess *)m_pWorkProcess)->PacketParser(&packet);

		iCount--;
		SwitchToThread();
	}

	*/

	return 0;
}


void CSock_Dgram::StatusOut(int iDest, char *strStat)
{
#if DEBUG_PRINT
	//HWND hWndMain = ::AfxGetMainWnd()->m_hWnd;
	if(hWndMain != NULL)
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

void CSock_Dgram::RTPControlPacketParser(SOCKADDR_IN temp_sin,Packet * pPacket)
{
	switch(pPacket->id())
	{
	
	case RTP_CTRL_NEW_FRAME:
		{
			RTPDataHead tempRTPDataHead;
			pPacket->readData2(&tempRTPDataHead);
//			AddRecvPacketBuffer(tempRTPDataHead,temp_sin,NULL);
		}
		break;
	case RTP_CTRL_TEST:
		{
			char strOut[100];
			//sprintf_s(strOut, ("RTP_CONTROL_TEST_REQ:%d         "),timeGetTime());
			StatusOut(3,strOut);
		}
		break;
	case RTP_CTRL_RECV_PACKET_UPDATE:
		{
			unsigned short Count;
			unsigned long PacketID;
			pPacket->readData2(&Count);
			for(int i=0;i<Count;i++)
			{
				pPacket->readData2(&PacketID);
				//RemovePacket(PacketID,temp_sin);
			}
		}
		break;
	}
}

void CSock_Dgram::RTPDataPacketParser(SOCKADDR_IN temp_sin,Packet * pPacket)
{
	((WorkProcess *)m_pWorkProcess)->PacketParser(pPacket,false);
}

int CSock_Dgram::RecvFrom(void)
{
	 int index = 0,                      // Integer index
      iRecvLen,iSize; 

	 sockaddr_in temp_sin;

	iSize = sizeof (temp_sin);

	 iRecvLen = recvfrom(Sock, 
				buffer,
				sizeof(buffer),         
				0,(sockaddr *)&temp_sin,&iSize);

	if (iRecvLen == SOCKET_ERROR)
	{
		TRACE(_T("SOCKET_ERROR\r\n"));

	}
	else
	{
		
		Packet packet;
		packet.copyToBuffer(buffer,iRecvLen);
		unsigned short ID = packet.id();

		switch(ID)
		{
		case RTP_CONTROL_SEND_PACKET_DATA_NOREQ:
		case RTP_CONTROL_SEND_PACKET_DATA_NOREQ_ACK:
		case RTP_CONTROL_SEND_PACKET_DATA:
		case RTP_CONTROL_SEND_PACKET_START_ACK:
		case RTP_CONTROL_SEND_PACKET_START_REQ:
		case RTP_CONTROL_SEND_PACKET_END_ACK:
		case RTP_CONTROL_SEND_PACKET_END_REQ:
		case RTP_CONTROL_NEW_PACKET_ACK:
		case RTP_CONTROL_NEW_PACKET_REQ:
		{
			RecvPacketProcess(temp_sin,buffer,iRecvLen);
		}
		break;

		default:
			{
				((WorkProcess *)m_pWorkProcess)->PacketParser(&packet);
			}
			break;
		}
		
		
	}
	
	return iRecvLen;
}


int CSock_Dgram::Close(void)
{
	 // Disable receiving on Sock before closing it.

	if(!this->m_bConnectState) return true;

	this->m_bConnectState = false;

	shutdown (Sock, 0x00);

	// Close Sock.
	closesocket (Sock);

	//WSACleanup (); //kimm 07.03/

	Sock = INVALID_SOCKET;       // Datagram window socket

	return TRUE;
}

int CSock_Dgram::SendTo(unsigned char * Data , int iLen)
{
	// Send a message to the multicasting address.
	if(!this) return 0;


	//RTPDataHead

	bSendLock = true;
//	this->AddSendPacket((char *)Data,iLen,&target_sin);

	fd_set	fdWrite  = { 0 };
	TIMEVAL	stTime;
	TIMEVAL	*pstTime = NULL;

	DWORD dwTimeout = 100;
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
	int res = select((int) s+1, NULL, &fdWrite, NULL, pstTime );



	if(res >0)
	{
		if (sendto (Sock, 
				  (char *)Data,
				  iLen,
				  0,
				  (struct sockaddr FAR *) &target_sin,
				  sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
		{
		//	closesocket (Sock);
			return FALSE;
		}
	}

	bSendLock = false;

	return TRUE;
}

int CSock_Dgram::SendTo(SOCKADDR_IN * target ,char * Data , int iLen)
{

	if(!this) return 0;

	bSendLock = true;

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
	int res = select((int) s+1, NULL, &fdWrite, NULL, pstTime );

	if(res >0)
	{
		if (sendto (Sock, 
				  (char *)Data,
				  iLen,
				  0,
				  (struct sockaddr FAR *) target,
				  sizeof (SOCKADDR_IN)) == SOCKET_ERROR)
		{
		//	closesocket (Sock);
			return FALSE;
		}
	}

	bSendLock = false;

	return TRUE;
}


void CSock_Dgram::SendingThread(LPVOID lParam)
{
	CSock_Dgram * pSocket = (CSock_Dgram*)lParam;
	unsigned long Time_Old,Time_Elapsed,Time_Wait;
	double sSize = 0.0;
	double sSpeed_Max;
	sSpeed_Max = 1024;

	Time_Wait = 2;
	Time_Old =timeGetTime();
	while(pSocket->m_bConnectState)
	{
		Time_Old =  timeGetTime();
		pSocket->Process();
		Time_Elapsed = timeGetTime() - Time_Old ;
		Time_Old += Time_Elapsed ;
		//Time_Wait = 5 - Time_Elapsed;
		
		Sleep(1);		
	}
	//Sleep(1);
}

void CSock_Dgram::ProcessingThread(LPVOID lParam)
{
	unsigned int Size;
	CSock_Dgram * pSocket = (CSock_Dgram*)lParam;
	//memset(pSocket->m_Buffer,0x00,PACKETBUFFERSIZE);
	pSocket->m_receivedPacketSize=0;

	Size=PACKETBUFFERSIZE;

	fd_set	fdRead  = { 0 };
	TIMEVAL	stTime;
	TIMEVAL	*pstTime = NULL;

	DWORD dwTimeout = 1;
	if ( INFINITE != dwTimeout ) {
		stTime.tv_sec = dwTimeout/1000;
		stTime.tv_usec = dwTimeout % 1000;
		pstTime = &stTime;
	}

	while(pSocket->m_bConnectState)
	{
		SOCKET s = (SOCKET) pSocket->Sock;
		if ( !FD_ISSET( s, &fdRead ) )
			FD_SET( s, &fdRead );

		DWORD dwBytesRead = 0L;
		int res = select( (int)s+1, &fdRead, NULL, NULL, pstTime );

		if(res>0)
		{
			pSocket->RecvFrom();
			pSocket->RecvPakcetUpadate();
			
		} else {
			Sleep(1);		
		}
		//Sleep(1);		
		
	}
	
}

DWORD CSock_Dgram::THSendStarter(LPVOID lParam)
{
	CSock_Dgram * pSocket = (CSock_Dgram*)lParam;
	pSocket->SendingThread(lParam);
	return 4;
}


DWORD CSock_Dgram::THConnectServer(LPVOID lParam)
{
	CSock_Dgram * pSocket = (CSock_Dgram*)lParam;
	pSocket->ProcessingThread(lParam);
	return 4;
}