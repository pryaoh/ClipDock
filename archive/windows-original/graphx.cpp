//
// graphx.cpp
//
#include "stdafx.h"
#include <tchar.h>
#include "graphx.h"


HINSTANCE g_hInst = NULL;		// Handle for the dll for the current process.
CSize m_SizeWnd;
CShared sg_Shared;




TCHAR* GetFileTitlePtr( TCHAR* pszPath )
{
	// Given a full file path, get a pointer to its title.
	TCHAR* p = pszPath + lstrlen(pszPath); 
	while ((p > pszPath) && ! FILE_IsDirSep(*p)) 
		p--;
	if ( FILE_IsDirSep(*p))
		p++; 
	return p;
}

HRESULT CreateDirectoryX( const TCHAR* pszDir )
{
	// This is like CreateDirectory() except.
	// This will create intermediate directories if needed.
	// NOTE: like SHCreateDirectoryExA() but we cant use since thats only for Win2K+

	ASSERT(pszDir);
	TCHAR szTmp[ _MAX_PATH ];
	int iLen = 0;
	for (;;)
	{	
		int iStart = iLen;
		TCHAR ch;
		for (;;)
		{
			if ( iLen >= COUNTOF(szTmp)-1 )
				return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
			ch = pszDir[iLen];
			szTmp[iLen] = ch;
			if ( ch == '\0' )
				break;
			iLen++;
			if ( FILE_IsDirSep( ch ))
				break;
		}
		if ( iStart >= iLen )
		{
			if ( ch == '\0' )
				break;
			return HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);	// bad name!
		}
		szTmp[iLen] = '\0';
		if ( ! CreateDirectory( szTmp, NULL ))
		{
			DWORD dwLastError = ::GetLastError();
			if ( dwLastError == ERROR_ALREADY_EXISTS )
				continue;
			if ( dwLastError == ERROR_ACCESS_DENIED && iStart==0 ) // create a drive? 'c:\' type thing.
				continue;
			return HRESULT_FROM_WIN32(dwLastError);
		}
	}

	return S_OK;
}

bool Str_IsSpace( char ch )
{
	// isspace() chokes on bad values!
	if ( ch < 0 || ch >= 0x80 )	// isspace() chokes on this!
		return false;
	if ( isspace(ch))
		return true;
	return false;
}

char* Str_SkipSpace( const char* pszNon )
{
	// NOTE: \n is a space.
	ASSERT(pszNon);
	for(;;)
	{
		char ch = *pszNon;
		if ( ch <= 0 )
			break;
		if ( ! Str_IsSpace(ch))
			break;
		pszNon++;
	}
	return (char*) pszNon;
}


bool CHookJump::InstallHook( LPVOID pFunc, LPVOID pFuncNew )
{
	ASSERT(pFuncNew);
	if ( pFunc == NULL )
	{
//		DEBUG_ERR(("InstallHook: NULL."));
		return false;
	}
	if ( IsHookInstalled() && ! memcmp(pFunc, m_Jump, sizeof(m_Jump)))
	{
//		DEBUG_MSG(("InstallHook: already has JMP-implant." LOG_CR));
		return true;
	}

	// DEBUG_TRACE(("InstallHook: pFunc = %08x, pFuncNew = %08x" LOG_CR, (UINT_PTR)pFunc, (UINT_PTR)pFuncNew ));
	m_Jump[0] = 0;

	DWORD dwNewProtection = PAGE_EXECUTE_READWRITE;
	if ( ! ::VirtualProtect( pFunc, 8, dwNewProtection, &m_dwOldProtection))
	{
		ASSERT(0);
		return false;
	}

	// unconditional JMP to relative address is 5 bytes.
	m_Jump[0] = 0xe9;
	DWORD dwAddr = (DWORD)((UINT_PTR)pFuncNew - (UINT_PTR)pFunc) - sizeof(m_Jump);
//	DEBUG_TRACE(("JMP %08x" LOG_CR, dwAddr ));
	memcpy(m_Jump+1, &dwAddr, sizeof(dwAddr));

	memcpy(m_OldCode, pFunc, sizeof(m_OldCode));
	memcpy(pFunc, m_Jump, sizeof(m_Jump));

//	DEBUG_MSG(("InstallHook: JMP-hook planted." LOG_CR));
	return true;
}

void CHookJump::RemoveHook( LPVOID pFunc )
{
	if (pFunc == NULL)
		return;
	if ( ! IsHookInstalled())	// was never set!
		return;
	try 
	{
		memcpy(pFunc, m_OldCode, sizeof(m_OldCode));	// SwapOld(pFunc)
		DWORD dwOldProtection = 0;
		::VirtualProtect(pFunc, 8, m_dwOldProtection, &dwOldProtection ); // restore protection.
		m_Jump[0] = 0;	// destroy my jump. (must reconstruct it)
	}
	catch (...)
	{
//		DEBUG_ERR(("CHookJump::RemoveHook FAIL" LOG_CR));
	}
}

bool CShared::InitMasterWnd(HWND hWnd)
{
	// The Master TAKSI.EXE App just started.
	ASSERT(hWnd);
	m_hMasterWnd = hWnd;
	m_iMasterUpdateCount = 0;

	// Install the hook
	return HookCBT_Install();
}

void CShared::DestroyShared()
{
	// Master App Taksi.EXE is exiting. so close up shop.
	// Set flag, so that TaksiDll knows to unhook device methods
//	DEBUG_TRACE(( "CTaksiShared::DestroyShared" LOG_CR ));
	// Signal to unhook from IDirect3DDeviceN methods
	m_bMasterExiting = true;

	// Uninstall the hook
	HookCBT_Uninstall();
}

bool CShared::InitShared()
{
	// Call this only once the first time.
	// determine my file name 
	ASSERT(g_hInst);
//	DEBUG_MSG(( "CTaksiShared::InitShared" LOG_CR ));

	m_hMasterWnd = NULL;
	m_iMasterUpdateCount = 0;
	m_bMasterExiting = false;
	m_hHookCBT = NULL;
	m_iProcessCount = 0;	// how many processes attached?
	m_dwHotKeyMask = 0;
	m_bRecordPause = false;


	m_nDX8_Present = 0; // offset from start of DLL to the interface element i want.
	m_nDX8_Reset = 0;
	m_nDX9_Present = 0;
	m_nDX9_Reset = 0;


	// determine my directory 
	m_szIniDir[0] = '\0';
	DWORD dwLen = GetCurrentDirectory( sizeof(m_szIniDir)-1, m_szIniDir );
	if ( dwLen <= 0 )
	{
		dwLen = GetModuleFileName( g_hInst, m_szIniDir, sizeof(m_szIniDir)-1 );
		if ( dwLen > 0 )
		{
			TCHAR* pszTitle = GetFileTitlePtr(m_szIniDir);
			ASSERT(pszTitle);
			*pszTitle = '\0';
		}
	}
	if ( ! FILE_IsDirSep( m_szIniDir[dwLen-1] ))
	{
		m_szIniDir[dwLen++] = '\\';
		m_szIniDir[dwLen] = '\0';
	}

	// read optional configuration file. global init.
	m_dwConfigChangeCount = 0;	// changed when the Custom stuff in m_Config changes.

	// ASSUME InitMasterWnd will be called shortly.
	return true;
}


LRESULT CALLBACK CShared::HookCBTProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	// WH_CBT = computer-based training - hook procedure
	// NOTE: This is how we inject this DLL into other processes.
	// NOTE: There is a race condition here where sg_Shared.m_hHookCBT can be NULL. 
	// NOTE: This can probably get called on any thread?!
	// ASSERT(sg_Shared.IsHookCBT());
	switch (nCode)
	{
	// case HCBT_CREATEWND:
	case HCBT_ACTIVATE:
	case HCBT_SETFOCUS:
		// Set the DLL implants on whoever gets the focus/activation!
		// ASSUME DLL_PROCESS_ATTACH was called.
		// TAKSI_INDICATE_Hooked
		// wParam = NULL. can be null if losing focus ? (NULL not used for desktop in this case)
		/*
		if ( g_Proc.m_bIsProcessSpecial )
			break;
		LOG_MSG(( "HookCBTProc: nCode=0x%x, wParam=0x%x" LOG_CR, (DWORD)nCode, wParam ));
		if ( wParam == NULL )	// ignore this!
			break;
		if ( g_Proc.AttachGraphXAPIs( (HWND) wParam ) == S_OK )
		{
			g_Proc.CheckProcessCustom();	// determine frame capturing algorithm 
		}
		*/
		break;
		
	}

	// We must pass the all messages on to CallNextHookEx.
	return ::CallNextHookEx(sg_Shared.m_hHookCBT, nCode, wParam, lParam);
}

bool CShared::HookCBT_Install(void)
{
	// Installer for calling of HookCBTProc 
	// This causes my dll to be loaded into another process space. (And hook it!)
	if ( IsHookCBT())
	{
//		LOG_MSG(( "HookCBT_Install: already installed. (0x%x)" LOG_CR, m_hHookCBT ));
		return false;
	}

	m_hHookCBT = ::SetWindowsHookEx( WH_CBT, HookCBTProc, g_hInst, 0);

//	LOG_MSG(( "HookCBT_Install: hHookCBT=0x%x,ProcID=%d" LOG_CR,
//		m_hHookCBT, ::GetCurrentProcessId()));

	UpdateMaster();
	return( IsHookCBT());
}

bool CShared::HookCBT_Uninstall(void)
{
	// Uninstaller for WH_CBT
	// NOTE: This may not be the process that started the hook
	// We may have successfully hooked an API. so stop looking.
	if (!IsHookCBT())
	{
//		LOG_MSG(( "HookCBT_Uninstall: already uninstalled." LOG_CR));
		return false;
	}

	if ( ::UnhookWindowsHookEx( m_hHookCBT ))
	{
//		LOG_MSG(( "CTaksiShared::HookCBT_Uninstall: hHookCBT=0%x,ProcID=%d" LOG_CR,
//			m_hHookCBT, ::GetCurrentProcessId()));
	}
	else
	{
//		DEBUG_ERR(( "CTaksiShared::HookCBT_Uninstall FAIL" LOG_CR ));
	}
	m_hHookCBT = NULL;
	UpdateMaster();
	return true;
}


void CShared::UpdateMaster()
{
	// tell the Master EXE to redisplay state info. 
	// sg_ProcStats or sg_Shared has changed
	// PostMessage is Multi thread safe.
	if ( m_hMasterWnd == NULL )
		return;
	if ( m_bMasterExiting )
		return;
	/*
	if ( ::PostMessage( m_hMasterWnd, WM_APP_UPDATE, 0, 0 ))
	{
		m_iMasterUpdateCount ++;	// count unprocessed WM_APP_UPDATE messages
	}
	*/
}

void CVideoFrameForm::InitPadded( int cx, int cy, int iBPP, int iPad )
{
	// iPad = sizeof(DWORD)
	m_Size.cx = cx;
	m_Size.cy = cy;
	m_iBPP = iBPP;
	int iPitchUn = cx * iBPP;
	int iRem = iPitchUn % iPad;
	m_iPitch = (iRem == 0) ? iPitchUn : (iPitchUn + iPad - iRem);
}

HRESULT Check_GetLastError( HRESULT hResDefault )
{
	// Something failed so find out why
	// hResDefault = E_FAIL or CONVERT10_E_OLESTREAM_BITMAP_TO_DIB
	DWORD dwLastError = ::GetLastError();
	if ( dwLastError == 0 )
		dwLastError = hResDefault; // no idea why this failed.
	return HRESULT_FROM_WIN32(dwLastError);
}

//**************************************************************** 

void CVideoFrame::FreeFrame()
{
	if ( !IsValidFrame()) 
		return;
	::HeapFree( ::GetProcessHeap(), 0, m_pPixels);
	m_pPixels = NULL;
}

bool CVideoFrame::AllocForm( const CVideoFrameForm& FrameForm )
{
	// allocate space for a frame of a certain format.
	if ( IsValidFrame())
	{
		if ( ! memcmp( &FrameForm, this, sizeof(FrameForm)))
			return true;
		FreeFrame();
	}
	((CVideoFrameForm&)*this) = FrameForm;
	m_pPixels = (BYTE*) ::HeapAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, get_SizeBytes());
	if ( ! IsValidFrame())
		return false;
	return true;
}

bool CVideoFrame::AllocPadXY( int cx, int cy, int iBPP, int iPad )
{
	// iPad = sizeof(DWORD)
	CVideoFrameForm FrameForm;
	FrameForm.InitPadded(cx,cy,iBPP,iPad);
	return AllocForm(FrameForm);
}

void CVideoFrame::SetupBITMAPINFOHEADER( BITMAPINFOHEADER& bmih ) const
{
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = m_Size.cx;
	bmih.biHeight = m_Size.cy;
	bmih.biPlanes = 1;
	bmih.biBitCount = m_iBPP*8;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
}

HRESULT CVideoFrame::SaveAsBMP( const TCHAR* pszFileName ) const
{
	// Writes a BMP file
	ASSERT( pszFileName );
/*
	// save to file
	CNTHandle File( ::CreateFile( pszFileName,            // file to create 
		GENERIC_WRITE,                // open for writing 
		0,                            // do not share 
		NULL,                         // default security 
		OPEN_ALWAYS,                  // overwrite existing 
		FILE_ATTRIBUTE_NORMAL,        // normal file 
		NULL ));                        // no attr. template 
	if ( ! File.IsValidHandle()) 
	{
		HRESULT hRes = Check_GetLastError( HRESULT_FROM_WIN32(ERROR_CANNOT_MAKE));
		DEBUG_ERR(("CVideoFrame::SaveAsBMP: FAILED save to file. 0x%x" LOG_CR, hRes ));
		return hRes;	// 
	}

	DWORD dwSizeBytes = get_SizeBytes();

	// fill in the headers
	BITMAPFILEHEADER bmfh;
	bmfh.bfType = 0x4D42; // 'BM'
	bmfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwSizeBytes;
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	DWORD dwBytesWritten;
	::WriteFile(File, &bmfh, sizeof(bmfh), &dwBytesWritten, NULL);
	if ( dwBytesWritten != sizeof(bmfh) )
	{
	}

	BITMAPINFOHEADER bmih;
	SetupBITMAPINFOHEADER(bmih);
	::WriteFile(File, &bmih, sizeof(bmih), &dwBytesWritten, NULL);
	if ( dwBytesWritten != sizeof(bmih) )
	{
	}

	::WriteFile(File, m_pPixels, dwSizeBytes, &dwBytesWritten, NULL);
	if ( dwBytesWritten != dwSizeBytes )
	{
	}
	*/
	return S_OK;
}

//******************************************************************************


// D3DCOLOR format is high to low, Alpha, Blue, Green, Red
const DWORD CTaksiGraphX::sm_IndColors[TAKSI_INDICATE_QTY] = 
{
	0xff88fe00,	// TAKSI_INDICATE_Ready = green
	0xff4488fe,	// TAKSI_INDICATE_Hooked = Blue,
	0xfffe4400,	// TAKSI_INDICATE_Recording = red.
	0xff444444,	// TAKSI_INDICATE_RecordPaused = Gray
};

//**************************************************************** 

HRESULT CTaksiGraphX::MakeScreenShot( bool bHalfSize )
{
	// make custom screen shot
	/*
	CLOCK_START(a);
	int iDiv = bHalfSize ? 2 : 1;

	// allocate buffer for pixel data
	CVideoFrame frame;
	frame.AllocPadXY( g_Proc.m_Stats.m_SizeWnd.cx / iDiv, g_Proc.m_Stats.m_SizeWnd.cy / iDiv );

	// get pixels from the backbuffer into the new buffer
	HRESULT hRes = GetFrame( frame, bHalfSize );	// virtual
	if (FAILED(hRes))
	{
		m_bGotFrameInfo = false;	// must get it again.
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError),
			_T("MakeScreenShot: unable to get RGB-data. 0%x"), hRes );
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		return hRes;
	}

	// save as bitmap
	TCHAR szFileName[_MAX_PATH];

#ifdef USE_GDIP	// use Gdiplus::Bitmap to save images as PNG (or JPEG)
	hRes = MakeScreenShotGDIP( szFileName, frame );
	if ( hRes != S_OK )
#endif
	{
		g_Proc.MakeFileName( szFileName, _T("bmp"));
		hRes = frame.SaveAsBMP(szFileName);
	}
	if (FAILED(hRes))
	{
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError),
			_T("MakeScreenShot(%d) SaveAsBMP '%s' failed. 0%x"),
			bHalfSize, szFileName, hRes	);
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		return hRes;
	}

	CLOCK_STOP(a, "MakeScreenShot: clock=%d");

	g_Proc.m_Stats.m_dwDataRecorded = frame.get_SizeBytes();
	g_Proc.UpdateStat(TAKSI_PROCSTAT_DataRecorded);

	_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError),
		_T("MakeScreenShot(%d) '%s' OK"),
		bHalfSize, szFileName	);
	g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
	*/
	return S_OK;
}

//*****************************************

void CTaksiGraphX::RecordAVI_Reset()
{
	// There was a reset of the video device API
	// Could be the window is changing size etc.
	// Stop video record and start again later.

	// if in recording mode, close the AVI file,
	// and set the flag so that the next Present() will restart it.
	/*
	if ( g_AVIFile.IsOpen())
	{
		LOG_MSG(( "CTaksiGraphX::RecordAVI_Reset" LOG_CR));
		g_AVIThread.WaitForAllFrames();
		g_AVIFile.CloseAVI();
		if ( ! sg_Shared.m_bRecordPause )
		{
			g_HotKeys.ScheduleHotKey(TAKSI_HOTKEY_RecordBegin);	// re-open it later.
		}
	}
*/
	// is HWND still valid ? 

	/*
	if ( ! ::IsWindow( g_Proc.m_Stats.m_hWndCap ))
	{
		g_Proc.m_Stats.m_hWndCap = NULL;	// must re-query for this
		g_Proc.UpdateStat(TAKSI_PROCSTAT_Wnd);
	}
	*/
	m_bGotFrameInfo = false;	// must get it again.
}

HRESULT CTaksiGraphX::RecordAVI_Start()
{
	/*
	sg_Shared.m_bRecordPause = false;

	if ( g_AVIFile.IsOpen())
	{
		return S_OK;
	}

	TCHAR szFileName[_MAX_PATH];
	g_Proc.MakeFileName( szFileName, _T("avi"));

	DEBUG_MSG(( "CTaksiGraphX::RecordAVI_Start (%s)." LOG_CR, szFileName ));

	if ( g_Proc.m_Stats.m_SizeWnd.cx <= 8 || g_Proc.m_Stats.m_SizeWnd.cy <= 8 )
	{
		// No point to making a video this small!
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
			_T("Video Window Size TOO SMALL."));
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		return DISP_E_BUFFERTOOSMALL;
	}

	int iDiv = (sg_Config.m_bVideoHalfSize) ? 2 : 1;

	g_FrameRate.InitStart();

	// allocate buffer for pixel data
	CVideoFrameForm FrameForm;
	FrameForm.InitPadded(g_Proc.m_Stats.m_SizeWnd.cx/iDiv, g_Proc.m_Stats.m_SizeWnd.cy/iDiv);

	// What frame rate do we want for our movie?
	double fFrameRate = sg_Config.m_fFrameRateTarget;
	if (g_Proc.m_pCustomConfig)
	{
		fFrameRate = g_Proc.m_pCustomConfig->m_fFrameRate;
	}

	HRESULT hRes = g_AVIFile.OpenAVICodec( FrameForm, fFrameRate, sg_Config.m_VideoCodec );
	if ( FAILED(hRes))
	{
		// ? strerror()
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
			_T("Cant open AVI codec. Error=0x%x. Try a different video codec?"), hRes );
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		LOG_WARN(("g_AVIFile.OpenAVIFile FAILED 0x%x." LOG_CR, hRes ));
		return hRes;
	}
	
	hRes = g_AVIFile.OpenAVIFile( szFileName );
	if ( FAILED(hRes))
	{
		// ? strerror()
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
			_T("Cant open AVI file. Error=0x%x."), hRes );
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		LOG_WARN(("g_AVIFile.OpenAVIFile FAILED 0x%x." LOG_CR, hRes ));
		return hRes;
	}

	ASSERT( g_AVIFile.IsOpen());
	LOG_MSG(( "RecordAVI_Start Recording started." LOG_CR));

	hRes = g_AVIThread.StartAVIThread();
	if ( IS_ERROR(hRes))
	{
		_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
			_T("Cant create AVI thread. Error=0x%x."), hRes );
		g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);
		return hRes;
	}

	_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
		_T("AVI record started"));
	g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);

	sg_Shared.UpdateMaster();
	*/
	return S_OK;
}

void CTaksiGraphX::RecordAVI_Stop()
{
	/*
	if ( ! g_AVIFile.IsOpen())
		return;

	DEBUG_MSG(( "CTaksiGraphX:RecordAVI_Stop" LOG_CR));
	g_AVIThread.WaitForAllFrames();
	g_AVIFile.CloseAVI();

	_sntprintf( g_Proc.m_Stats.m_szLastError, sizeof(g_Proc.m_Stats.m_szLastError), 
		_T("AVI file recorded. Stopped.") );
	g_Proc.UpdateStat(TAKSI_PROCSTAT_LastError);

	sg_Shared.UpdateMaster();
	*/
}

bool CTaksiGraphX::RecordAVI_Frame()
{
	/*
	// We are actively recording the AVI. a frame is ready.
	ASSERT( g_AVIFile.IsOpen());
	if ( sg_Shared.m_bRecordPause )	// just skip.
		return true;

	// determine whether this frame needs to be grabbed when recording. or just skipped.
	DWORD dwFrameDups = g_FrameRate.CheckFrameRate();
	if ( dwFrameDups <= 0)	// i want this frame?
		return true;

	CAVIFrame* pFrame = g_AVIThread.WaitForNextFrame();	// dont get new frame til i finish the last one.
	if (pFrame==NULL)
	{
		LOG_WARN(("CTaksiGraphX::RecordAVI_Frame() FAILED" LOG_CR ));
		return false;
	}

	// NOTE: I cant safely use sg_Config.m_bVideoHalfSize in real time.
	bool bVideoHalfSize = ( pFrame->m_Size.cx < ( g_Proc.m_Stats.m_SizeWnd.cx - 4 ));

	CLOCK_START(b);
	// get pixels from the backbuffer into the new buffer
	HRESULT hRes = GetFrame( *pFrame, bVideoHalfSize );	// virtual
	if ( IS_ERROR(hRes))
	{
		m_bGotFrameInfo = false;	// must get it again.
	}
	CLOCK_STOP(b,"Clock GetFrame=%d");

	// Move compression work to another thread
	g_AVIThread.SignalFrameAdd(pFrame,dwFrameDups);	// ready to compress/write

	g_FrameRate.EndOfOverheadTime();	// keep track of recording overhead
	*/
	return true;
}

//********************************************************

void CTaksiGraphX::ProcessHotKey( TAKSI_HOTKEY_TYPE eHotKey )
{
	/*
	// Open AVI file
	switch (eHotKey)
	{
	case TAKSI_HOTKEY_ConfigOpen:	// Open the config dialog window.
		sg_Shared.HotKey_ConfigOpen();
		return;
	case TAKSI_HOTKEY_HookModeToggle:	// don't switch during video capture?
		sg_Shared.HotKey_HookModeToggle();
		return;
	case TAKSI_HOTKEY_IndicatorToggle:
		sg_Shared.HotKey_IndicatorToggle();
		return;

	case TAKSI_HOTKEY_RecordBegin:
		// toggle video capturing mode
		if ( g_AVIFile.IsOpen())
		{
			RecordAVI_Stop();
		}
		else
		{
			RecordAVI_Start();
		}
		break;
	case TAKSI_HOTKEY_RecordStop: // Close AVI file.
		RecordAVI_Stop();
		break;
	case TAKSI_HOTKEY_RecordPause:	// toggle pause.
		sg_Shared.m_bRecordPause = ! sg_Shared.m_bRecordPause;
		if ( ! g_AVIFile.IsOpen() && ! sg_Shared.m_bRecordPause )
		{
			RecordAVI_Start();
		}
		g_FrameRate.InitStart();
		break;
	case TAKSI_HOTKEY_Screenshot:	// make custom screen shot
		MakeScreenShot(false);
		break;
	case TAKSI_HOTKEY_SmallScreenshot: // make small screen shot
		MakeScreenShot(true);
		break;
	}
	*/
}

void CTaksiGraphX::PresentFrameBegin( bool bChange )
{
}

void CTaksiGraphX::PresentFrameEnd()
{
}

HRESULT CTaksiGraphX::AttachGraphXAPI()
{
	
	// DLL_PROCESS_ATTACH
	// Check the DLL that would support this graphics mode.
	// NOTE: Dont force graphics DLL to load. just look for its presense.
	// ONLY CALLED FROM: AttachGraphXAPIs()
	// Not fully attached til PresentFrameBegin() is called.
/*
	if ( ! sg_Config.m_abUseAPI[ get_GraphXAPI() ] )	// not allowed.
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
	}
*/
	get_GraphXAPI();
	const TCHAR* pszName = get_DLLName();	// virtual
	if ( !FindDll(pszName)) // already loaded?
	{
//		DEBUG_TRACE(( "AttachGraphXAPI NO '%s'" LOG_CR, pszName ));
		return HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
	}

	ASSERT( IsValidDll());
	HRESULT hRes = HookFunctions();	// virtual
	if ( FAILED(hRes))
	{
		m_bHookedFunctions = false;	
		return HRESULT_FROM_WIN32(ERROR_INVALID_HOOK_HANDLE);
	}

	m_bHookedFunctions = true;	
//	LOG_MSG(( "AttachGraphXAPI '%s'" LOG_CR, pszName ));
	// NOTE: a hook is not truly complete til PresentFrameBegin() is called.
	m_bGotFrameInfo = false;	// must update this ASAP.
	
	return S_OK;
}
