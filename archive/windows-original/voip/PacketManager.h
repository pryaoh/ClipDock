#pragma once
#include "Sock_Dgram.h"
#include "PacketParser.h"


class CPacketManager
{
public:
	CFile file;
	bool	AckCheck[128];
	CWnd * pMainwnd;
	CPacket	RecvPacket;

	CSock_Dgram		*pSock;
	
//	_SendData	Request;
//	_SendData	SendData;
//	_ParserData ParserData;
	CEdit * m_pEdit;
	CPacketManager(void);
	~CPacketManager(void);
	static void WINAPI PacketParser(void * pData, int iLen);
public:



public:
	void RequestSend(int RequestType, void * pData);
};
