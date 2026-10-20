#pragma once


// CAlphaDlg 대화 상자입니다.

class CAlphaDlg : public CDialog
{
	DECLARE_DYNAMIC(CAlphaDlg)

public:
	int m_iAlpha;
	CAlphaDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CAlphaDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_ALPHADLG };


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

	virtual void SetTransparent(bool bTransparent)
	{
		if(::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & WS_EX_TRANSPARENT)
		{
			if(!bTransparent)
			{
				::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
				::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
			}
		} else {
			if(bTransparent)
			{
				::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
				::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_TRANSPARENT);
			}
		}

		//this->ShowWindow(SW_HIDE);
		//::SetParent(this->GetSafeHwnd(),NULL);
	}

	virtual void KillFocus()
	{
		this->GetParent()->SetFocus();
	}

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};
