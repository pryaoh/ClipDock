// WinSocket.cpp: implementation of the CWinSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "WinSocket.h"

#include "WorkProcess.h"

#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
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


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

WSADATA CWinSocket::wsaData;

int	CWinSocket::iPort;
long CWinSocket::Result;
HWND CWinSocket::m_pHwnd;

CWinSocket::CWinSocket()
{
	m_bClose=false;

}

CWinSocket::~CWinSocket()
{
	if(m_bConnectState)
	{
		closesocket(m_ClientSocket);

	}

}

bool CWinSocket::ConnectServer(char * ServerIP,int Port)
{
	strcpy_s(SERVER_IP,(char *)ServerIP);
	iPort=Port;
	/*
	if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0)
	{
		return FALSE;  // 윈속 라이브러리 로드 실패.
	}
	*/
	DWORD dwThreadId=0;	

	m_pGoConnectThread = BEGINTHREADEX(NULL,0,THConnectServer,this,0,&dwThreadId);
	//m_pGoConnectThread = AfxBeginThread(THConnectServer,this);
	return TRUE;
}


UINT CWinSocket::THConnectServer(LPVOID lParam)
{
	CWinSocket * pSocket = (CWinSocket*)lParam;

	struct sockaddr_in server;
    struct hostent *host = NULL;
	
	pSocket->m_bClose=false;
	pSocket->m_bConnectState = FALSE; // 일단 FALSE 셋팅.
    
    pSocket->m_ClientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(pSocket->m_ClientSocket == INVALID_SOCKET)
    {
	        return 1;  // 실패.
    }    
    
	int _port = iPort;

	server.sin_family = AF_INET;
    server.sin_port = htons(_port);
	server.sin_addr.s_addr = inet_addr(pSocket->SERVER_IP);
	
	if (server.sin_addr.s_addr == INADDR_NONE)
	{
		host = gethostbyname(pSocket->SERVER_IP);
		if (host == NULL)
		{
			return 1;
		}
		CopyMemory(&server.sin_addr, host->h_addr_list[0],host->h_length);
	}
	
	if(connect( pSocket->m_ClientSocket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
    {
		return 0;  // 접속 실패.
    }  
	pSocket->m_bConnectState = TRUE;
	DWORD dwThreadId=0;	

	pSocket->m_pGoRecvThread = BEGINTHREADEX(NULL,0,RecvData,lParam,0,&dwThreadId);
	return 1;
}

UINT CWinSocket::RecvData(LPVOID lParam)
{
	unsigned int Size;
	CWinSocket * pSocket= (CWinSocket*)lParam;


	Packet receivedPacket;
	DWORD bytesTransferred;
	memset(pSocket->m_Buffer,0x00,PACKETBUFFERSIZE);
	pSocket->m_receivedPacketSize=0;
	
	WSANETWORKEVENTS event; // Event Storage Structure
	WSAEVENT hRecvEvent = WSACreateEvent(); // Init WinSock And Return Handle
	WSAEventSelect(pSocket->m_ClientSocket,hRecvEvent,FD_WRITE|FD_READ|FD_CLOSE); //

	//  Receiving packet
	while(pSocket->m_bConnectState)
	{		
		Size=PACKETBUFFERSIZE;

		WSAWaitForMultipleEvents(1,&hRecvEvent,true,WSA_INFINITE,false);

		WSAEnumNetworkEvents(pSocket->m_ClientSocket,hRecvEvent,&event);
		if((event.lNetworkEvents & FD_READ) == FD_READ)
		{
			bytesTransferred = recv(pSocket->m_ClientSocket ,pSocket->m_Buffer ,PACKETBUFFERSIZE-1,0);
			
			if (bytesTransferred == SOCKET_ERROR)
				break;

			if( bytesTransferred > 0 )
				pSocket->m_receivedPacketSize += bytesTransferred;

			
			receivedPacket.copyToBuffer( pSocket->m_Buffer, pSocket->m_receivedPacketSize );

			if( receivedPacket.isValidPacket() == true && pSocket->m_receivedPacketSize >= ( int )receivedPacket.getPacketSize() )
			{
				((WorkProcess *)pSocket->m_pWorkProcess)->PacketParser(&receivedPacket,true);
				
			} else 
				break;
			
		} else if((event.lNetworkEvents & FD_CLOSE) == FD_CLOSE) {

			pSocket->m_bConnectState = false;

			if(!pSocket->m_bClose)
			{
//				::SendMessage(m_pHwnd,WM_COMMAND,ID_DISCONNECTED,0);
			} else {
				
			}
			
		}
		Sleep(1);
	}
	pSocket->m_bConnectState = FALSE;
	pSocket->CloseServer();
	return 1;
}

void CWinSocket::PacketParsing( Packet& packet )
{
	
}

//--- 아래는 내부 함수들
BOOL CWinSocket::CloseServer()
{
//	::KillTimer(m_hWnd,ALIVE_TIMER);
	
	if(m_bClose) return true;
	m_bClose=true;

	m_bConnectState = FALSE;

	shutdown (m_ClientSocket, 0x00);
	closesocket(m_ClientSocket);
//	FireCloseSocket();
	//_sleep(100);

//	WSACleanup (); //kimm 07.03/

	return TRUE;
}


int CWinSocket::SendPacket(Packet *packet)
{
	int ret;
	ret = send(m_ClientSocket,packet->getPacketBuffer(),packet->getPacketSize(),0);
	if (ret == 0)
	{
		//m_bConnectState = FALSE;
		return 0;
	}
	else if (ret == SOCKET_ERROR)
	{
		//m_bConnectState = FALSE;
		return 0;
	}
	
	return 1; 
}

int CWinSocket::SendAlive()
{
	Packet packet;
	packet.id(0);
	return SendPacket(&packet);
}

void CWinSocket::SetParentWindow(HWND v)
{
	m_pHwnd=v;
}
