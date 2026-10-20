#pragma once
#include "RecordDlg.h"
#include "WebControl.h"

// CMyTabCtrl

class CMyTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CMyTabCtrl)

public:
	HWND hWndMaster;
	CDialog *m_tabPages[20];
	int m_tabCurrent;
	int m_nNumberOfPages;
	CRecordDlg m_RecordDlg;
	CWebControl m_WebCtrlDlg;

	void Init();

	void SetRectangle();

	CMyTabCtrl();
	virtual ~CMyTabCtrl();

protected:
	//{{AFX_MSG(CMyTabCtrl)
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


