#pragma once
#include "WebView.h"
#include "afxwin.h"

// CWebControl 대화 상자입니다.

class CWebControl : public CDialog
{
	DECLARE_DYNAMIC(CWebControl)

public:
	int m_Alpha;
	HWND hWndMaster;
	CWebView m_Display;
	CWebControl(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CWebControl();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_WEB_CONTROL };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	CSliderCtrl m_AlphaCtrl;
//	afx_msg void OnNMThemeChangedAlpha(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
//	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBnClickedShow();
	CButton m_ShowWeb;
	afx_msg void OnBnClickedTransparent();
	CButton m_TransParent;
};
