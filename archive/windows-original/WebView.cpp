// WebView.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "WebView.h"


// CWebView 대화 상자입니다.

IMPLEMENT_DYNAMIC(CWebView, CAlphaDlg)

CWebView::CWebView(CWnd* pParent /*=NULL*/)
	: CAlphaDlg( pParent)
{

}

CWebView::~CWebView()
{
}

void CWebView::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ALPHA_CHANGE, m_Alpha);
}


BEGIN_MESSAGE_MAP(CWebView, CDialog)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ALPHA_CHANGE, &CWebView::OnNMCustomdrawAlphaChange)
	ON_WM_KILLFOCUS()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CWebView 메시지 처리기입니다.

void CWebView::OnNMCustomdrawAlphaChange(NMHDR *pNMHDR, LRESULT *pResult)
{
	//LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);

	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	int iPos = this->m_Alpha.GetPos();

	//::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
	//::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_CONTROLPARENT);

	this->ChangeAlpha(iPos);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

void CWebView::OnKillFocus(CWnd* pNewWnd)
{
	CDialog::OnKillFocus(pNewWnd);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

BOOL CWebView::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetSingleDialogSkin( this->m_hWnd ,"Dialog");

	//VERIFY( 1 == SetWindowSkin( this->m_hWnd , "MainFrame" ));
	this->m_Alpha.SetRange(20,100);
	this->m_Alpha.SetPos(70);
	//this->m_Frame1.ShowWindow(SW_HIDE);

	HINSTANCE hLibrary = (HINSTANCE)::LoadLibrary( "WTLDLG.dll" );
		typedef void (*GETPLUGIN)( IDLLFormView** );
		GETPLUGIN pfnLoad = (GETPLUGIN)::GetProcAddress( hLibrary, "GetPlugin" );
		if(pfnLoad == NULL)		
		{
			DWORD dwLastError = ::GetLastError();
			LPVOID lpMsgBuf;
			::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM|
				FORMAT_MESSAGE_IGNORE_INSERTS, 
				NULL, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
				(LPTSTR)&lpMsgBuf, 0, NULL );

			CString msg;
			msg.Format("GetProcAddress(\"GetPlugin\") Failed\r\nError Code: %ld\r\nReason:%s", dwLastError, lpMsgBuf); 
			LocalFree(lpMsgBuf);
			
			::MessageBox(NULL, msg, "Error", MB_OK );
//			return FALSE;
		}

		pOutput = 0;

		pfnLoad( &pOutput );

		//::DestroyWindow(m_hWndClient);
		//m_hWndClient = 0;
		

		 m_hWndClient = pOutput->Initialize(this->GetSafeHwnd());
		 ::BringWindowToTop(m_hWndClient);

		 RECT rect = { 0 };
		GetClientRect(&rect);

		// position bars and offset their dimensions
//		UpdateBarsPosition(rect, bResizeBars);

		// resize client window
		if(m_hWndClient != NULL)
			::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				SWP_NOZORDER | SWP_NOACTIVATE);


	this->SetWindowPos(NULL,0,0,800,600,SWP_NOMOVE|SWP_NOZORDER);


	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CWebView::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if( m_hWndClient )
	{
		 RECT rect = { 0 };
		GetClientRect(&rect);

		// position bars and offset their dimensions
	//		UpdateBarsPosition(rect, bResizeBars);

		// resize client window
		if(m_hWndClient != NULL)
			::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top-6,
				rect.right - rect.left, rect.bottom - rect.top+6,
				SWP_NOZORDER | SWP_NOACTIVATE);
	}

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}
