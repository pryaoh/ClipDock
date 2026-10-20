// WinSocket.h: interface for the CWinSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WINSOCKET_H__4CE52A56_74E4_44EF_8481_EF9EC4277577__INCLUDED_)
#define AFX_WINSOCKET_H__4CE52A56_74E4_44EF_8481_EF9EC4277577__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

#include "Packet.h"

//#include "PROTOCOL.h"
//#include "DataStruct.h"

class CWinSocket  
{
private:
	static WSADATA wsaData;
	static HWND m_pHwnd;
	static long Result;
	static int	iPort;

public:
	bool	m_bClose;
	void *  m_pWorkProcess;

	int		m_bConnectState;	// 연결 상태.	
	SOCKET	m_ClientSocket;		// Client Socket.
	char SERVER_IP[256];
	char m_Buffer[PACKETBUFFERSIZE];
	int m_receivedPacketSize;
	
	HANDLE m_pGoRecvThread;
	HANDLE m_pGoConnectThread;
public:
	void SetParentWindow(HWND v);
	
	BOOL CloseServer();
	int SendAlive();
	int SendPacket(Packet *packet);
	
	virtual void PacketParsing( Packet& packet );
	static UINT RecvData(LPVOID lParam);
	static UINT THConnectServer(LPVOID lParam);
	bool ConnectServer(char * ServerIP,int Port);
	CWinSocket();
	virtual ~CWinSocket();

};

#endif // !defined(AFX_WINSOCKET_H__4CE52A56_74E4_44EF_8481_EF9EC4277577__INCLUDED_)
