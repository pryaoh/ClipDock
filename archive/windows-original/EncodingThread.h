#if !defined(AFX_ENCODINGTHREAD_H__D683EBEF_7DBA_4A75_8A3E_513CCB6B558C__INCLUDED_)
#define AFX_ENCODINGTHREAD_H__D683EBEF_7DBA_4A75_8A3E_513CCB6B558C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EncodingThread.h : header file
//



/////////////////////////////////////////////////////////////////////////////
// CEncodingThread thread
#include "H263Enc.h"
#include "H263Dec.h"
#include "ScreenCapture.h"
#include "Avi_Lib/avilib.h"

typedef struct tag_ScreenCaptureConfig
{
	unsigned int iWidth;
	unsigned int iHeight;
	unsigned int iKeyFrame;
	unsigned int iFrameRate;
	tag_ScreenCaptureConfig()
	{
		iWidth=320;
		iHeight=240;
		iKeyFrame=20;
		iFrameRate=10;
	};


} _SC_Config;

class CEncodingThread : public CWinThread
{
	DECLARE_DYNCREATE(CEncodingThread)

private:

	bool bStart;
	CScreenCapture m_SCM;
	unsigned int Frame;

	static void ThreadProc();

	avi_t* aviFile ;
public:
	_SC_Config m_SCConfig;


	//AFX_THREADPROC ThreadProc();
public:
	//void LoadAndCompress(CFile * pFile);
	CH263Enc m_h263Encoder;
	CH263Dec m_h263Decoder;

protected:
	CEncodingThread();           // protected constructor used by dynamic creation

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEncodingThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CEncodingThread();

	// Generated message map functions
	//{{AFX_MSG(CEncodingThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
public:
	void InitClass(void);
	void EncodeStart(void);
	virtual BOOL OnIdle(LONG lCount);
	virtual int Run();
	virtual BOOL PumpMessage();
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENCODINGTHREAD_H__D683EBEF_7DBA_4A75_8A3E_513CCB6B558C__INCLUDED_)
