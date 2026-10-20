// DisplayWin.cpp : implementation file
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "DisplayWin.h"
#include <atlbase.h>    
#include <atlcom.h>     
#include <atlhost.h>    

CComModule _Module;
BEGIN_OBJECT_MAP(ObjectMap)
END_OBJECT_MAP()
#pragma comment(lib,"Release/MouseHook.lib")

HWND g_hChild, g_hwndIE;
const int IE_WIDTH = 450, IE_HEIGHT=300;
const int IE_X = 10, IE_Y = 100;

   


// CDisplayWin dialog

IMPLEMENT_DYNAMIC(CDisplayWin, CDialog)

CDisplayWin::CDisplayWin(CWnd* pParent /*=NULL*/)
	: CDialog(CDisplayWin::IDD, pParent)
{
	m_hWndClient = NULL;

}

CDisplayWin::~CDisplayWin()
{
//	UnInstallHook();
}

void CDisplayWin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
//	DDX_Control(pDX, IDC_COMBO1, m_Address);
//	DDX_Control(pDX, IDC_EXPLORER1, m_WebBrowser1);
//	DDX_Control(pDX, IDC_GO, m_Go);
	DDX_Control(pDX, IDC_VIEW1, m_View);
	DDX_Control(pDX, IDC_ALPHA_CHANGE, m_Alpha);
	DDX_Control(pDX, IDC_HIDE, m_Hide);
}


BEGIN_MESSAGE_MAP(CDisplayWin, CDialog)
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_NCHITTEST()
	ON_WM_PAINT()
	ON_WM_MOVE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDOK, &CDisplayWin::OnBnClickedOk)
	ON_BN_CLICKED(IDNO, &CDisplayWin::OnBnClickedNo)
	ON_BN_CLICKED(IDCANCEL, &CDisplayWin::OnBnClickedCancel)
	ON_WM_SETCURSOR()
	ON_WM_MOUSEACTIVATE()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_SHOWWINDOW()
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_HIDE, &CDisplayWin::OnBnClickedHide)
	ON_WM_CHAR()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ALPHA, &CDisplayWin::OnNMCustomdrawAlpha)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ALPHA_CHANGE, &CDisplayWin::OnNMCustomdrawAlphaChange)
END_MESSAGE_MAP()


// CDisplayWin message handlers

void CDisplayWin::OnSetFocus(CWnd* pOldWnd)
{
	//CDialog::OnSetFocus(pOldWnd);

	// TODO: Add your message handler code here
}

void CDisplayWin::OnKillFocus(CWnd* pNewWnd)
{
	//CDialog::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
}

LRESULT CDisplayWin::OnNcHitTest(CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	return CDialog::OnNcHitTest(point);
}

void CDisplayWin::OnPaint()
{
	RECT rt;
	CWnd * pParent = this->GetParent();
	pParent->GetClientRect(&rt);
	pParent->ClientToScreen(&rt);

	

	CPaintDC dc(this); // device context for painting
	// TODO: Add your message handler code here
	// Do not call CDialog::OnPaint() for painting messages
}

void CDisplayWin::OnMove(int x, int y)
{
	CDialog::OnMove(x, y);

	// TODO: Add your message handler code here
}

void CDisplayWin::MakeHtmlView()
{
	/*
    POINT pt;
    pt.x = pt.y = 0;
    ClientToScreen( &pt);

	RECT rt;
	this->GetWindowRect(&rt);

	if(m_pWebBrowser) return;

    // 비주얼 스튜디오 7.1이라 AtlAxWin71
	g_hwndIE = CreateWindow(ATLAXWIN_CLASS, "about:blank", WS_CHILD|WS_VISIBLE, 0,50,rt.right - rt.left,rt.bottom - rt.top -50, 
               this->GetSafeHwnd(), (HMENU)0, GetModuleHandle(NULL), NULL);

	//::ShowScrollBar(g_hwndIE,SB_BOTH,true);
    if(!g_hwndIE) return; 

    CComPtr<IUnknown>    punkIE; 

    if (AtlAxGetControl(g_hwndIE, &punkIE) == S_OK) 
    { 
        m_pWebBrowser = punkIE; 
        if( m_pWebBrowser ) 
        { 
            // 메뉴같은 것들을 안나오게 한다.
//            m_pWebBrowser->put_FullScreen(VARIANT_TRUE); 
            
            // 초기에 뜰 웹사이트를 지정할 수 있다. 
			char pszBuf[ 256] = "about:blank";
            CComVariant vempty, vUrl(pszBuf);
			
            m_pWebBrowser->Navigate ( CComBSTR(pszBuf), &vempty, &vempty, &vempty, &vempty );      
			//m_pWebBrowser->Navigate2 ( &vUrl, &vempty, &vempty, &vempty, &vempty );      
			//m_pWebBrowser->Navigate(


			
			


        } 
        else 
        { 
            return; 
        } 
    } 
    else 
    { 
        return; 
    } 
	*/
}



BOOL CDisplayWin::OnInitDialog()
{
	CDialog::OnInitDialog();


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


		//	SetDialogSkin( "Dialog" );
	this->SetWindowPos(NULL,0,0,800,600,SWP_NOMOVE|SWP_NOZORDER);

	

	/*
	_Module.Init(ObjectMap, NULL,&myGuid);
	AtlAxWinInit(); 
//	InstallHook(); 

//	SetMainHwnd(this->GetSafeHwnd());
	//gm_Shared.hWnd = (DWORD)this->GetSafeHwnd();

//	m_WebBrowser.Create(
	
	//MakeHtmlView();


	HWND hWndEdit = this->m_Address.GetWindow(GW_CHILD)->GetSafeHwnd();
	::SetWindowLong(hWndEdit, GWL_STYLE,
	::GetWindowLong(hWndEdit, GWL_STYLE) | ES_MULTILINE|ES_WANTRETURN|ES_AUTOVSCROLL  );
*/
	//ES_MULTILINE
	

	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CDisplayWin::OnSize(UINT nType, int cx, int cy)
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
			::SetWindowPos(m_hWndClient, NULL, rect.left, rect.top+20,
				rect.right - rect.left, rect.bottom - rect.top-20,
				SWP_NOZORDER | SWP_NOACTIVATE);
	}

	/*
	if(m_WebBrowser1)
	{
		m_WebBrowser1.SetWindowPos(NULL,0,50,cx,cy-50,SWP_NOZORDER);
		m_Hide.SetWindowPos(NULL,cx-10,0,0,0,SWP_NOSIZE|SWP_NOZORDER);
	}
	*/
	/*
	::SetWindowPos(g_hwndIE,NULL,0,50,cx,cy-50,SWP_NOZORDER);
	if(m_pWebBrowser)
	{
		m_pWebBrowser->put_Width(cx);
		m_pWebBrowser->put_Height(cy-50);
	}
	*/


	//::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
	//::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
	//Sleep(100);
	//::SetLayeredWindowAttributes(this->GetSafeHwnd(), 0, (255 * 60) / 100, LWA_ALPHA);


	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CDisplayWin::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	this->ShowWindow(SW_HIDE);

	::SetFocus(this->GetParent()->GetSafeHwnd());
	//CDialog::OnClose();
}

void CDisplayWin::OnBnClickedOk()
{

	return ;
	

	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//OnOK();
}

void CDisplayWin::OnBnClickedNo()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CDisplayWin::OnBnClickedCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	//OnCancel();
}

void CDisplayWin::OnCbnSelchangeCombo1()
{
	
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CDisplayWin::OnCbnEditchangeCombo1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int a=0;
	
	
}

//void CDisplayWin::OnNMCustomdrawAlpha(NMHDR *pNMHDR, LRESULT *pResult)
//{
//	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
//	int iPos = this->m_Alpha.GetPos();
//
//	//::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
//	//::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_CONTROLPARENT);
//
//	if(::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE)&WS_EX_LAYERED)
//	{
//
//		::SetLayeredWindowAttributes(this->GetSafeHwnd(), 0, (255 * iPos) / 100, LWA_ALPHA);
//
//
//	}
//	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
//	*pResult = 0;
//}

void CDisplayWin::OnCbnSetfocusCombo1()
{
	

	//this->m_Address.SetFocus();
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

BOOL CDisplayWin::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	return CDialog::OnSetCursor(pWnd, nHitTest, message);
}

int CDisplayWin::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	//::SetFocus(pDesktopWnd->GetSafeHwnd());
//	pDesktopWnd->SetFocus();
	return CDialog::OnMouseActivate(pDesktopWnd, nHitTest, message);
}

void CDisplayWin::OnMouseMove(UINT nFlags, CPoint point)
{
	CDialog::OnMouseMove(nFlags, point);
}

void CDisplayWin::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: Add your message handler code here and/or call default

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
	CDialog::OnLButtonDown(nFlags, point);
}

void CDisplayWin::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);
	if(bShow)
	{
		HWND TemphWnd = this->GetParent()->GetSafeHwnd();
		{
			RECT rt;
			::GetWindowRect(TemphWnd,&rt);

			if((rt.left == 0) && (rt.top == 0))
			{

				if(::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE)&WS_EX_LAYERED)
				{

					
				} else {
					::SendMessage(this->hWndMaster,WM_KEYDOWN,17,0);
					::SendMessage(this->hWndMaster,WM_KEYDOWN,VK_F9,0);

					
					::SendMessage(this->hWndMaster,WM_KEYUP,VK_F9,0);
					::SendMessage(this->hWndMaster,WM_KEYUP,17,0);
				}


			//	::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
			//	::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_LAYERED);

				//this->ShowWindow(SW_HIDE);
				//::SetParent(this->m_Display.GetSafeHwnd(),NULL);

				//::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
				//::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
				//::SetLayeredWindowAttributes(this->GetSafeHwnd(), 0, (255 * 70) / 100, LWA_ALPHA);

				//::SetWindowPos(this->m_Display.GetSafeHwnd(),hTargetWnd,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			} else {
				//::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
				//::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_LAYERED);
			}

		}
		
		//MakeHtmlView();
	}

	// TODO: Add your message handler code here
}

void CDisplayWin::OnCbnEditupdateCombo1()
{
	
	// TODO: Add your control notification handler code here
}

void CDisplayWin::OnCbnSelendokCombo1()
{
	
	// TODO: Add your control notification handler code here
}

void CDisplayWin::OnCbnKillfocusCombo1()
{
	int a=0;
	
	// TODO: Add your control notification handler code here
}

void CDisplayWin::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CDisplayWin::WebUpdate()
{
	CString szTemp;

	this->m_Address.GetWindowTextA(szTemp);

	CComVariant vempty, vUrl(szTemp);
//	m_WebBrowser1.Navigate2( &vUrl, &vempty, &vempty, &vempty, &vempty );
			

	/*
	if( m_pWebBrowser ) 
	{ 
		// 메뉴같은 것들을 안나오게 한다.
//		m_pWebBrowser->put_FullScreen(VARIANT_TRUE); 
        
		// 초기에 뜰 웹사이트를 지정할 수 있다. 
		char pszBuf[ 256] = "about:blank";
		

		CComVariant vempty, vUrl(szTemp);
		//m_pWebBrowser->Navigate2 ( &vUrl, &vempty, &vempty, &vempty, &vempty );             
		m_pWebBrowser->Navigate ( CComBSTR(szTemp), &vempty, &vempty, &vempty, &vempty );             
		

		VARIANT_BOOL bTRUE;
		bool bTrue;

		bTrue = 1;

		
		
		
//		m_pWebBrowser->put_AddressBar(1);
//		m_pWebBrowser->put_MenuBar(1);
//		m_pWebBrowser->put_RegisterAsBrowser(1);
//		m_pWebBrowser->put_Resizable(1);

		
		{
			CComPtr<IDispatch>pdispDoc;
			m_pWebBrowser->get_Document(&pdispDoc);

			CComQIPtr<IHTMLDocument2>piDoc(pdispDoc);
			CComPtr<IHTMLElement>piElem;
			CComPtr<IHTMLBodyElement>piBody;

			piDoc->get_body(&piElem); 

			//piElem->putwin
			piElem->QueryInterface(&piBody); 
			if(piBody)
			{
				(piBody->put_scroll(CComBSTR(_T("yes")))); //// no scroll bar!!!!!
				BSTR p;
				piBody->get_scroll(&p);


				// which allows us to do scrolling
				IHTMLElement2 *pElement = NULL;
				long s;
				HRESULT hr = piBody->QueryInterface(IID_IHTMLElement2,(void**)&pElement);
				ASSERT(SUCCEEDED(hr));
				ASSERT( pElement );

				// now we are ready to scroll
				// scroll down to 100th pixel from top
				pElement->put_scrollTop( 100 ); 
				pElement->get_clientHeight(&s);
			    
				// try to get the whole page size - but the returned number
				// is not allways correct. especially with pages that use dynamic html
				// tricks...
				long scroll_height; 
				pElement->get_scrollHeight( &s );

				// we can use this workaround!
				RECT rt;
				this->GetWindowRect(&rt);

				long real_scroll_height;
				pElement->put_scrollTop( 20000000 ); // ask to scroll really far down...
				pElement->get_scrollTop( &real_scroll_height );
				real_scroll_height += rt.bottom - rt.top -50; // will return the scroll height

				pElement->put_scrollTop( 100 ); 
				// for the first visible pixel, to get whole html page size must
				// add the window's height... (to obtain window_height is
				// left as an exercise for the reader)

			}

			

		}


		
	} 
	*/
}

BOOL CDisplayWin::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class

	switch(LOWORD(wParam))
	{
	case ID_UPDATED:
		{
			if((HWND)lParam == this->m_Address.GetSafeHwnd())
			{
				WebUpdate();
			}
		}
		break;
	case ID_KILLFOCUS:
		{
			KillFocus();
		
		}
		break;
	default:
		break;	
	}
	return CDialog::OnCommand(wParam, lParam);
}

void CDisplayWin::KillFocus()
{
	this->GetParent()->SetFocus();
}
void CDisplayWin::OnBnClickedGo()
{
	KillFocus();

	


	/*
	{
			CComPtr<IDispatch>pdispDoc;
			m_pWebBrowser->get_Document(&pdispDoc);

			CComQIPtr<IHTMLDocument2>piDoc(pdispDoc);
			CComPtr<IHTMLElement>piElem;
			CComPtr<IHTMLBodyElement>piBody;

			piDoc->get_body(&piElem); 

			//piElem->putwin
			piElem->QueryInterface(&piBody); 
			if(piBody)
			{
				(piBody->put_scroll(CComBSTR(_T("yes")))); //// no scroll bar!!!!!
				BSTR p;
				piBody->get_scroll(&p);


				// which allows us to do scrolling
				IHTMLElement2 *pElement = NULL;
				long s;
				HRESULT hr = piBody->QueryInterface(IID_IHTMLElement2,(void**)&pElement);
				ASSERT(SUCCEEDED(hr));
				ASSERT( pElement );

				// now we are ready to scroll
				// scroll down to 100th pixel from top
				pElement->put_scrollTop( 100 ); 
				pElement->get_clientHeight(&s);
			    
				// try to get the whole page size - but the returned number
				// is not allways correct. especially with pages that use dynamic html
				// tricks...
				long scroll_height; 
				pElement->get_scrollHeight( &s );

				// we can use this workaround!
				RECT rt;
				this->GetWindowRect(&rt);

				long real_scroll_height;
				pElement->put_scrollTop( 20000000 ); // ask to scroll really far down...
				pElement->get_scrollTop( &real_scroll_height );
				real_scroll_height += rt.bottom - rt.top -50; // will return the scroll height

				pElement->put_scrollTop( 100 ); 
				// for the first visible pixel, to get whole html page size must
				// add the window's height... (to obtain window_height is
				// left as an exercise for the reader)

			}

			

		}
		*/
}

void CDisplayWin::OnBnClickedHide()
{
	this->ShowWindow(SW_HIDE);
	KillFocus();

}

void CDisplayWin::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	CDialog::OnChar(nChar, nRepCnt, nFlags);
}

void CDisplayWin::OnNMCustomdrawAlpha(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

void CDisplayWin::OnNMCustomdrawAlphaChange(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}
