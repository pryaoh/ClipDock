// MyTabCtrl.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "MyTabCtrl.h"


// CMyTabCtrl

IMPLEMENT_DYNAMIC(CMyTabCtrl, CTabCtrl)

CMyTabCtrl::CMyTabCtrl()
{

}

CMyTabCtrl::~CMyTabCtrl()
{
}

BEGIN_MESSAGE_MAP(CMyTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CMyTabCtrl)
	ON_WM_LBUTTONDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// CMyTabCtrl 메시지 처리기입니다.

void CMyTabCtrl::Init()
{
	m_tabCurrent=0;
	m_RecordDlg.hWndMaster = hWndMaster;
	m_RecordDlg.Create(IDD_RECORD,this);
	m_RecordDlg.ShowWindow(SW_SHOW);
	m_tabPages[0] = &m_RecordDlg;

	m_WebCtrlDlg.hWndMaster = hWndMaster;
	m_WebCtrlDlg.Create(IDD_WEB_CONTROL,this);
	m_WebCtrlDlg.ShowWindow(SW_HIDE);
	m_tabPages[1]=&m_WebCtrlDlg;


	SetSingleDialogSkin( m_RecordDlg.m_hWnd ,"Dialog");
	SetSingleDialogSkin( m_WebCtrlDlg.m_hWnd ,"Dialog");
	

	TCITEMA TempItem;
	memset(&TempItem,0x00,sizeof(TempItem));
	TempItem.mask = TCIF_TEXT  ;

	TempItem.pszText = "레코딩";
	InsertItem(0,&TempItem);

	TempItem.pszText = "Web";
	InsertItem(1,&TempItem);
	
	SetCurSel(0);

	m_RecordDlg.SetWindowPos(0,0,20,0,0,SWP_NOSIZE|SWP_NOZORDER);
	m_WebCtrlDlg.SetWindowPos(0,0,20,0,0,SWP_NOSIZE|SWP_NOZORDER);

	m_nNumberOfPages=2;

	SetRectangle();
}

void CMyTabCtrl::SetRectangle()
{
	CRect tabRect, itemRect;
	int nX, nY, nXc, nYc;

	GetClientRect(&tabRect);
	GetItemRect(0, &itemRect);

	nX=itemRect.left;
	nY=itemRect.bottom+1;
	nXc=tabRect.right-itemRect.left-1;
	nYc=tabRect.bottom-nY-1;

	m_tabPages[0]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_SHOWWINDOW);
	for(int nCount=1; nCount < m_nNumberOfPages; nCount++){
		m_tabPages[nCount]->SetWindowPos(&wndTop, nX, nY, nXc, nYc, SWP_HIDEWINDOW);
	}
}

void CMyTabCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	CTabCtrl::OnLButtonDown(nFlags, point);

	if(m_tabCurrent != GetCurFocus()){
		m_tabPages[m_tabCurrent]->ShowWindow(SW_HIDE);
		m_tabCurrent=GetCurFocus();
		m_tabPages[m_tabCurrent]->ShowWindow(SW_SHOW);
		m_tabPages[m_tabCurrent]->SetFocus();
	}
}
