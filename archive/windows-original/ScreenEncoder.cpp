// ScreenEncoder.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "ScreenEncoderDlg.h"
#include "CWebBrowser2.h"



/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderApp

BEGIN_MESSAGE_MAP(CScreenEncoderApp, CWinApp)
	//{{AFX_MSG_MAP(CScreenEncoderApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderApp construction


CScreenEncoderApp::CScreenEncoderApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CScreenEncoderApp object

CScreenEncoderApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderApp initialization

BOOL CScreenEncoderApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	
	InitSkinMagicLib( ::AfxGetResourceHandle(), "ScreenEncoder" ,  NULL, NULL );
	LoadSkinFile("LightBlue.smf"); 
	//SetDialogSkin( "Dialog" );

	

	CScreenEncoderDlg dlg;
	m_pMainWnd = &dlg;

	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	ExitSkinMagicLib();
/*
	gdiSO.NotificationUnhook(gdiHookToken);
	GdiplusShutdown(gdiToken);
*/


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
