#pragma once
#include "afxwin.h"
#include "MyComboBox.h"
//#include "CWebBrowser2.h"
//#include "explorer1.h"
#include "afxcmn.h"

//#include "Mousehook/mousehook.h"

// CDisplayWin dialog


struct  IDLLFormView
{
	virtual ~IDLLFormView() { }
	virtual HWND 	Initialize(HWND hParentWnd) = 0;
	virtual void 	Destroy() = 0;
	virtual TCHAR* 	Description() = 0;
};


class CDisplayWin : public CDialog
{
	DECLARE_DYNAMIC(CDisplayWin)


public:
	//CWebBrowser2 m_WebBrowser;

	HWND hWndMaster;
	 CComQIPtr<IWebBrowser>  m_pWebBrowser;
	GUID  myGuid;
	void MakeHtmlView();
	CDisplayWin(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDisplayWin();
	CPoint m_CursorPosOld;


	void WebUpdate();
	void KillFocus();

// Dialog Data
	enum { IDD = IDD_DISPLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSetFocus(CWnd* pOldWnd);
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
public:
	afx_msg LRESULT OnNcHitTest(CPoint point);
public:
	afx_msg void OnPaint();
public:
	afx_msg void OnMove(int x, int y);
public:
	virtual BOOL OnInitDialog();
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	CStatic m_Frame1;
	CMyComboBox m_Address;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedNo();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnEditchangeCombo1();
	afx_msg void OnNMCustomdrawAlpha(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSetfocusCombo1();
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
public:
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
public:
	afx_msg void OnCbnEditupdateCombo1();
public:
	afx_msg void OnCbnSelendokCombo1();
public:
	afx_msg void OnCbnKillfocusCombo1();
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedGo();
	HWND m_hWndClient;
	IDLLFormView* pOutput;
//	CExplorer1 m_WebBrowser1;
	afx_msg void OnBnClickedHide();
	CButton m_Go;
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	CStatic m_View;
	CSliderCtrl m_Alpha;
	CButton m_Hide;
	afx_msg void OnNMCustomdrawAlphaChange(NMHDR *pNMHDR, LRESULT *pResult);
};
