#include "StdAfx.h"
#include "ThreadWork.h"


WorkThread::WorkThread(void)
{

}

WorkThread::~WorkThread(void)
{
}

void WorkThread::ProcessingThread(LPVOID lParam)
{
	
	this->m_WorkProc.Run(true);

}

DWORD WorkThread::THConnectServer(LPVOID lParam)
{
	WorkThread * pThis = (WorkThread*)lParam;
	pThis->ProcessingThread(lParam);
	return 2;
}
