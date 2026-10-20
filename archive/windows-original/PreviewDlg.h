#pragma once
#include "AlphaDlg.h"
#include "afxwin.h"

// CPreviewDlg 대화 상자입니다.

class CPreviewDlg : public CAlphaDlg
{
	DECLARE_DYNAMIC(CPreviewDlg)

public:
	CPreviewDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CPreviewDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_PREVIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedCancel();
public:
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
public:
	virtual BOOL OnInitDialog();

};
