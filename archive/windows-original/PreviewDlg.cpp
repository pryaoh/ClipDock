// PreviewDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "PreviewDlg.h"


// CPreviewDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CPreviewDlg, CAlphaDlg)

CPreviewDlg::CPreviewDlg(CWnd* pParent /*=NULL*/)
	: CAlphaDlg( pParent)
{

}

CPreviewDlg::~CPreviewDlg()
{
}

void CPreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CAlphaDlg::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPreviewDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CPreviewDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CPreviewDlg::OnBnClickedCancel)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CPreviewDlg 메시지 처리기입니다.

void CPreviewDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	this->ShowWindow(SW_HIDE);
	//OnOK();
}

void CPreviewDlg::OnBnClickedCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//OnCancel();
}

void CPreviewDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
	CDialog::OnLButtonDown(nFlags, point);
}

BOOL CPreviewDlg::OnInitDialog()
{
	CAlphaDlg::OnInitDialog();

	SetSingleDialogSkin( this->m_hWnd ,"Dialog");
	SetWindowPos(NULL,0,0,320,240,SWP_NOMOVE|SWP_NOZORDER);
	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}
