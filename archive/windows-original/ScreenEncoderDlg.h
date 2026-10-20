// ScreenEncoderDlg.h : header file
//

#if !defined(AFX_SCREENENCODERDLG_H__3352B2F0_3032_4377_846D_9F5CF886C90F__INCLUDED_)
#define AFX_SCREENENCODERDLG_H__3352B2F0_3032_4377_846D_9F5CF886C90F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderDlg dialog

#include "H263Enc.h"
#include "ScreenCapture.h"
#include "Avi_Lib/avilib.h"
#include "afxwin.h"
//#include "DisplayWin.h"

#include "MainCtrlPanel.h"

#include <list>

#define NUM_SAMPLES				16000
#define BIT						16
#define CHANEL					2


#define WM_APP_UPDATE		(WM_APP + 0) // WM_USER ?
#define WM_APP_REHOOKCBT	(WM_APP + 1) // WM_USER ?
#define WM_APP_TRAY_NOTIFICATION (WM_APP+2)
#define WCM_UPDATE_CURPOS (WM_APP+0x05)

typedef struct tag_ScreenCaptureConfig
{
	unsigned int biBitCount;
	unsigned int iWidth;
	unsigned int iHeight;
	unsigned int iKeyFrame;
	unsigned int iFrameRate;
	char OutDirPath[255];
	char OutFilePath[255];
	bool bInit;
	tag_ScreenCaptureConfig()
	{
		iWidth=800;//GetSystemMetrics(SM_CXSCREEN);
		iHeight=600;//GetSystemMetrics(SM_CYSCREEN);
		iKeyFrame=5;
		iFrameRate=25;
		memset(OutFilePath,0x00,sizeof(OutFilePath));
		memset(OutDirPath,0x00,sizeof(OutDirPath));
		bInit = false;
		
	};
} _SC_Config;
enum tag_Timer1
{
	TID_RECONNECT =10002,
};
class CScreenEncoderDlg : public CDialog
{
private:
	int iCodec;
	static CScreenEncoderDlg * pMainClass;

	bool bSoundRecord;
	bool bShiftDown;
	bool bCtrlDown;

	bool bPause;
	bool bStart;
	bool bPreview;

	bool bWide;
	double AspectRatio;

	CScreenCapture m_SCM;
	unsigned int Frame;

	int iBitRate;
	int s_bbFormat;
	int  nWidth,nHeight;
	bool bFrameLocked;
	unsigned char * pFrameBuffer;
	unsigned char * pFrameBuffer1;

	char m_szTargetWinName[256];


	static UINT EncodingThreadProc(LPVOID lParam);
	static UINT EncodingThreadProc1(LPVOID lParam);
	static void CALLBACK CScreenEncoderDlg::waveInProc(  HWAVEIN hwi, UINT uMsg,DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
	avi_t* aviFile ;

public:
	
	//CTaksiDX9 m_D3D9;

	CRITICAL_SECTION cs;

/**/

	unsigned long iWaveCount;
	CFile m_MP3_File;
	HINSTANCE m_hMp3Enc,m_hStreamDll;

	std::list<WAVEHDR*> m_headers;
	std::list<char*> m_buffers;

	//lame_global_flags * m_lgf;
	HWAVEIN m_hWaveIn;
	WAVEFORMATEX m_wfe;


	int (*InitEnc)(int nSamplesPerSec,int Bitrate,int Quality);
	short (*Encode_Buffers)(DWORD dwParam1,unsigned char * buffer,int * iBufferSize);
	void (*Close)(void);

	void (init)(HWND hWnd);
	void (Connect)(char * AddrTo,int PortNum);
	int (StartStreaming)();
	int (SendStreamData)(BITMAPINFOHEADER bi,char * buffer,int len);

	void SoundInit();
/**/
	//void LoadAndCompress(CFile * pFile);
	CH263Enc m_h263Encoder;
//	CH263Dec m_h263Decoder;


public:
	_SC_Config m_SCConfig;
	CScreenEncoderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CScreenEncoderDlg)
	enum { IDD = IDD_SCREENENCODER_DIALOG };
	CMainCtrlPanel m_MainCtrlPanel;
	
	CButton	m_PauseButton;
	CEdit	m_FrameRate;
	CEdit	m_KeyFrame;
	CStatic	m_Cross;
	CButton	m_RecordButton;
	CEdit	m_Width;
	CEdit	m_Height;
	CComboBox	m_ScreenSize;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScreenEncoderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CScreenEncoderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRecord();
//	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnEditchangeScreensize();
	afx_msg void OnSelchangeScreensize();
	afx_msg void OnChangeWidth();
	afx_msg void OnChangeHeight();
	afx_msg void OnChangeKeyframe();
	afx_msg void OnChangeFramerate();
	afx_msg void OnPause();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedButton1();
	CButton m_Close;
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnClose();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);



static void WINAPI DrawImageData1(void * Data,unsigned long DataLen);
public:
	CComboBox m_RecordMethod;
public:
	afx_msg void OnCbnSelchangeRecordMethod();
public:
	CEdit m_CurDir;
public:
	afx_msg void OnEnChangeCurdir();
public:
	CEdit m_CurWindowName;
public:
	afx_msg void OnEnChangeWindowname();
public:
	afx_msg void OnBnClickedSelwin();
public:
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedButton3();
public:
	CButton m_SelWinButton;
protected:
	virtual void OnCancel();
	virtual void OnOK();
public:
	CStatic m_View1;
public:
	afx_msg void OnBnClickedCheck1();
public:
	CButton m_Preview;
public:
	CButton m_Wide;
public:
	afx_msg void OnBnClickedWide();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
public:
	CButton m_SoundRecord;
public:
	afx_msg void OnBnClickedSoundRecord();
	CEdit M_BitRate;
	afx_msg void OnEnChangebitrate();
	afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
protected:
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedStart();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCREENENCODERDLG_H__3352B2F0_3032_4377_846D_9F5CF886C90F__INCLUDED_)
