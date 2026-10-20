#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "Previewdlg.h"

// CRecordDlg 대화 상자입니다.

class CRecordDlg : public CDialog
{
	DECLARE_DYNAMIC(CRecordDlg)

public:
	int iBitRate;
	int iFrameRate;
	bool *pbStart;
	HWND hWndMaster;
	int m_Alpha;
	CPreviewDlg m_Preview;
	CRecordDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CRecordDlg();
	void Update();


// 대화 상자 데이터입니다.
	enum { IDD = IDD_RECORD };
	void	SetRecord(bool bRecord);

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	


	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	CButton m_RecordButton;
	CEdit M_BitRate;
	CEdit m_FrameRate;
	afx_msg void OnBnClickedRecord();
public:
	CComboBox m_ScreenSizeSel;
public:
	afx_msg void OnCbnSelchangeScreen();
public:
	virtual BOOL OnInitDialog();
public:
	afx_msg void OnDeltaposSpin3(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult);
public:
	afx_msg void OnEnChangeFramerate();
public:
	afx_msg void OnEnChangebitrate2();
public:
	CSpinButtonCtrl m_FrameSelSpin;
public:
	CSpinButtonCtrl m_BitrateSelSpin;
public:
	afx_msg void OnBnClickedCheck2();
public:
	CButton m_bShowPreview;
	CSliderCtrl m_AlphaSel;
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
public:
	afx_msg void OnBnClickedCheck3();
};
