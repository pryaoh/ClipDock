#pragma once
#include "AlphaDlg.h"
#include "afxcmn.h"

#include "MyTabCtrl.h"

// CMainCtrlPanel 대화 상자입니다.

class CMainCtrlPanel : public CDialog
{
	DECLARE_DYNAMIC(CMainCtrlPanel)

public:
	int m_iAlpha;
	HWND hWndMaster;
	CWnd * pSubWnd[256];

	CMainCtrlPanel(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CMainCtrlPanel();



	
	virtual void SetParentWithAlpha(HWND hTargetWnd)
	{
		RECT rt;

		::GetWindowRect(hTargetWnd,&rt);

		if(IsWindow(hTargetWnd))
		{
			
			HWND TemphWnd = this->GetParent()->GetSafeHwnd();
			if(TemphWnd  !=  hTargetWnd)
			{
				::SetWindowLong(hTargetWnd,GWL_STYLE,::GetWindowLong(hTargetWnd,GWL_STYLE)|WS_CLIPCHILDREN);
				
				if((rt.left == 0) && (rt.top == 0))
				{
					::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
					::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_LAYERED);
					Sleep(100);
					::SetLayeredWindowAttributes(this->GetSafeHwnd(), 0, (255 * m_iAlpha) / 100, LWA_ALPHA);
				} else {
					::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
					::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_LAYERED);
				}
				
				::SetParent(this->GetSafeHwnd(),hTargetWnd);
				this->ShowWindow(SW_SHOW);
			}
		}
	}

	virtual void ChangeAlpha(int iPos)
	{
		m_iAlpha = 100 - iPos;
		if(m_iAlpha < 30)
		{
			m_iAlpha  = 30;
		}
		if(::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE)&WS_EX_LAYERED)
		{
			::SetLayeredWindowAttributes(this->GetSafeHwnd(), 0, (255 * m_iAlpha) / 100, LWA_ALPHA);
		}
	}

	virtual void  UnsetParent()
	{
		::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
		::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_LAYERED);

		this->ShowWindow(SW_HIDE);
		::SetParent(this->GetSafeHwnd(),NULL);
	}

	virtual void KillFocus()
	{
		this->GetParent()->SetFocus();
	}



// 대화 상자 데이터입니다.
	enum { IDD = IDD_MAIN_MENU };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CMyTabCtrl m_MainTab;
	virtual BOOL OnInitDialog();
	CSliderCtrl m_Alpha;
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
};
