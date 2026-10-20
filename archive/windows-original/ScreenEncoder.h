// ScreenEncoder.h : main header file for the SCREENENCODER application
//

#if !defined(AFX_SCREENENCODER_H__6B67185F_FA65_4C9B_9799_A590FAAA8D11__INCLUDED_)
#define AFX_SCREENENCODER_H__6B67185F_FA65_4C9B_9799_A590FAAA8D11__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderApp:
// See ScreenEncoder.cpp for the implementation of this class
//

class CScreenEncoderApp : public CWinApp
{
public:
	CScreenEncoderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScreenEncoderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CScreenEncoderApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SCREENENCODER_H__6B67185F_FA65_4C9B_9799_A590FAAA8D11__INCLUDED_)
