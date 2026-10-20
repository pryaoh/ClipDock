// EncodingThread.cpp : implementation file
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "EncodingThread.h"
#include ".\encodingthread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEncodingThread

IMPLEMENT_DYNCREATE(CEncodingThread, CWinThread)

void CEncodingThread::ThreadProc()
{
	
}
CEncodingThread::CEncodingThread()
{

	this->m_pfnThreadProc=(AFX_THREADPROC)ThreadProc;
}

CEncodingThread::~CEncodingThread()
{
	

}


BOOL CEncodingThread::InitInstance()
{
	bStart=false;
	aviFile = NULL;
	return TRUE;
}

int CEncodingThread::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	this->m_pfnThreadProc=NULL;
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CEncodingThread, CWinThread)
	//{{AFX_MSG_MAP(CEncodingThread)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEncodingThread message handlers

void CEncodingThread::InitClass(void)
{

}

void CEncodingThread::EncodeStart(void)
{

}

BOOL CEncodingThread::OnIdle(LONG lCount)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWinThread::OnIdle(lCount);
}

int CEncodingThread::Run()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWinThread::Run();
}

BOOL CEncodingThread::PumpMessage()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWinThread::PumpMessage();
}

BOOL CEncodingThread::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.

	return CWinThread::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
