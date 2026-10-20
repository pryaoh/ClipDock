// MainFormView.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "MainFormView.h"


// CMainFormView

IMPLEMENT_DYNCREATE(CMainFormView, CFormView)

CMainFormView::CMainFormView()
	: CFormView(CMainFormView::IDD)
{

}

CMainFormView::~CMainFormView()
{
}

void CMainFormView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMainFormView, CFormView)
END_MESSAGE_MAP()


// CMainFormView 진단입니다.

#ifdef _DEBUG
void CMainFormView::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void CMainFormView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// CMainFormView 메시지 처리기입니다.
