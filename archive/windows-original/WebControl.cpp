// WebControl.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "WebControl.h"


// CWebControl 대화 상자입니다.

IMPLEMENT_DYNAMIC(CWebControl, CDialog)

CWebControl::CWebControl(CWnd* pParent /*=NULL*/)
	: CDialog(CWebControl::IDD, pParent)
{

}

CWebControl::~CWebControl()
{
}

void CWebControl::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ALPHA, m_AlphaCtrl);
	DDX_Control(pDX, IDC_SHOW, m_ShowWeb);
	DDX_Control(pDX, IDC_TRANSPARENT, m_TransParent);
}


BEGIN_MESSAGE_MAP(CWebControl, CDialog)
	ON_WM_NCMOUSEMOVE()
//	ON_WM_DRAWITEM()
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_SHOW, &CWebControl::OnBnClickedShow)
	ON_BN_CLICKED(IDC_TRANSPARENT, &CWebControl::OnBnClickedTransparent)
END_MESSAGE_MAP()


// CWebControl 메시지 처리기입니다.

BOOL CWebControl::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetSingleDialogSkin( this->m_hWnd ,"Dialog");

	this->m_AlphaCtrl.SetRange(0,100);
	this->m_AlphaCtrl.SetPos(60);

	m_Display.Create(m_Display.IDD);
	m_Display.ShowWindow(SW_HIDE);
	m_Alpha=  this->m_AlphaCtrl.GetPos();
	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CWebControl::OnNcMouseMove(UINT nHitTest, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CDialog::OnNcMouseMove(nHitTest, point);
}

//void CWebControl::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
//{
//	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
//
//	::MessageBox(NULL,"test","1",MB_OK);
//	CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
//}

void CWebControl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	// 그리기 메시지에 대해서는 CDialog::OnPaint()을(를) 호출하지 마십시오.
}

void CWebControl::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	CDialog::OnMouseMove(nFlags, point);
}

void CWebControl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if(m_Alpha != this->m_AlphaCtrl.GetPos())
	{
		m_Alpha = this->m_AlphaCtrl.GetPos();
		m_Display.ChangeAlpha(100-m_Alpha);
	}

	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CWebControl::OnBnClickedShow()
{
	if(this->m_ShowWeb.GetCheck())
	{
		this->m_Display.UnsetParent();
		this->m_Display.SetParentWithAlpha(this->GetParent()->GetParent()->GetParent()->GetSafeHwnd());
		m_Display.ChangeAlpha(100-m_Alpha);
		m_ShowWeb.SetWindowTextA("숨기기");
	} else {
		this->m_Display.UnsetParent();
		m_ShowWeb.SetWindowTextA("보이기");
	}
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CWebControl::OnBnClickedTransparent()
{
	if(this->m_TransParent.GetCheck())
	{
		::SendMessage(hWndMaster,WM_KEYDOWN,VK_CONTROL,(1<<30));
		::SendMessage(hWndMaster,WM_KEYDOWN,VK_F8,(1<<30));
		::SendMessage(hWndMaster,WM_KEYUP,VK_F8,(1<<31));
		::SendMessage(hWndMaster,WM_KEYUP,VK_CONTROL,(1<<31));
		this->m_TransParent.SetWindowTextA("창 클릭하기");
	} else {
		::SendMessage(hWndMaster,WM_KEYDOWN,VK_SHIFT,(1<<30));
		::SendMessage(hWndMaster,WM_KEYDOWN,VK_F8,(1<<30));
		::SendMessage(hWndMaster,WM_KEYUP,VK_F8,(1<<31));
		::SendMessage(hWndMaster,WM_KEYUP,VK_SHIFT,(1<<31));
		this->m_TransParent.SetWindowTextA("창 클릭금지");
	}
}
