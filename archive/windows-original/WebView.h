#pragma once
#include "afxcmn.h"
#include "AlphaDlg.h"


struct  IDLLFormView
{
	virtual ~IDLLFormView() { }
	virtual HWND 	Initialize(HWND hParentWnd) = 0;
	virtual void 	Destroy() = 0;
	virtual TCHAR* 	Description() = 0;
};


// CWebView 대화 상자입니다.

class CWebView : public CAlphaDlg
{
	DECLARE_DYNAMIC(CWebView)

public:
	CWebView(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CWebView();

	HWND hWndMaster;
// 대화 상자 데이터입니다.
	enum { IDD = IDD_WEB_VIEW };

	IDLLFormView* pOutput;
	HWND m_hWndClient;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CSliderCtrl m_Alpha;
	afx_msg void OnNMCustomdrawAlphaChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
