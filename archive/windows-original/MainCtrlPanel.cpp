// MainCtrlPanel.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "MainCtrlPanel.h"


// CMainCtrlPanel 대화 상자입니다.

IMPLEMENT_DYNAMIC(CMainCtrlPanel, CDialog)

CMainCtrlPanel::CMainCtrlPanel(CWnd* pParent /*=NULL*/)
	: CDialog(CMainCtrlPanel::IDD, pParent)
{

}

CMainCtrlPanel::~CMainCtrlPanel()
{
}

void CMainCtrlPanel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAIN_TAB, m_MainTab);
	DDX_Control(pDX, IDC_ALPHA, m_Alpha);
}


BEGIN_MESSAGE_MAP(CMainCtrlPanel, CDialog)
	ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDOK, &CMainCtrlPanel::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CMainCtrlPanel::OnBnClickedCancel)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()


// CMainCtrlPanel 메시지 처리기입니다.

void CMainCtrlPanel::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
	CDialog::OnLButtonDown(nFlags, point);
}

void CMainCtrlPanel::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CMainCtrlPanel::OnBnClickedCancel()
{
	this->ShowWindow(SW_HIDE);
}

BOOL CMainCtrlPanel::OnInitDialog()
{
	CDialog::OnInitDialog();

	this->m_MainTab.hWndMaster = hWndMaster;
	this->m_MainTab.Init();

	this->m_Alpha.SetPos(0);
	int iPos = this->m_Alpha.GetPos();
	this->ChangeAlpha(iPos);

//	LoadSkinFile("LightBlue.smf");
	SetSingleDialogSkin( this->m_hWnd ,"Dialog");


	//this->m_MainTab.InsertItem(1,"Test2");
	//this->m_MainTab.InsertItem(2,"Test3");

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CMainCtrlPanel::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	int iPos = this->m_Alpha.GetPos();
	this->ChangeAlpha(iPos);

	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}
