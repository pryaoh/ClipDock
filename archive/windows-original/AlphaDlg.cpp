// AlphaDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "AlphaDlg.h"


// CAlphaDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CAlphaDlg, CDialog)

CAlphaDlg::CAlphaDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAlphaDlg::IDD, pParent)
{

	m_iAlpha = 100;
}

CAlphaDlg::~CAlphaDlg()
{
}

void CAlphaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CAlphaDlg, CDialog)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()


// CAlphaDlg 메시지 처리기입니다.

void CAlphaDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));

	CDialog::OnLButtonDown(nFlags, point);
}
