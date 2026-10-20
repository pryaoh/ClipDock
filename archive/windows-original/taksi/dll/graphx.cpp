//
// graphx.cpp
//
#include "../stdafx.h"
#include <tchar.h>
#include "TaksiDll.h"
#include "graphx.h"
#include "HotKeys.h"


#pragma comment(lib,".\\lib\\Kasumi.lib")
#pragma comment(lib,".\\lib\\system.lib")
#ifdef USE_GDIP	// use Gdiplus::Bitmap to save images as PNG (or JPEG)
#include "../common/CImageGDIP.h"

static HRESULT MakeScreenShotGDIP( TCHAR* pszFileName, CVideoFrame& frame )
{
	// use Gdiplus::Bitmap to save images as PNG (or JPEG)
	if ( ! g_gdiplus.m_Token )
	{
		g_gdiplus.AttachGDIPInt();
		if ( ! g_gdiplus.m_Token )
			return S_FALSE;
	}
	if ( sg_Config.m_szImageFormatExt[0] == '\0' )
		return S_FALSE;

	int iLenStr = g_Proc.MakeFileName( pszFileName, sg_Config.m_szImageFormatExt );

	static CLSID encoderClsid = {0};	// we should just hard code this ? ImageFormatPNG
	if ( encoderClsid.Data1 == 0 )
	{	
		HRESULT hRes = g_gdiplus.GetEncoderClsid( sg_Config.m_szImageFormatExt, &encoderClsid);
		if ( FAILED(hRes))
		{
			LOG_MSG(( "CTaksiGraphX::MakeScreenShot GDI+ GetEncoderClsid FAILED 0x%x" LOG_CR, hRes ));
			return hRes;
		}
	}

	BITMAPINFO bmi;
	frame.SetupBITMAPINFOHEADER(bmi.bmiHeader);
	Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromBITMAPINFO(&bmi,frame.m_pPixels);
	if ( pBitmap == NULL )
	{
		LOG_MSG(( "CTaksiGraphX::MakeScreenShot GDI+ FromBITMAPINFO FAILED" LOG_CR ));
		return CONVERT10_E_STG_DIB_TO_BITMAP;
	}

	WCHAR wFileName[_MAX_PATH];
#ifdef UNICODE
	lstrcpyn( wFileName, pszFileName, COUNTOF(wFileName)-1 );
	int iLenW = lstrlen( wFileName );
#else
	int iLenW = ::MultiByteToWideChar( CP_UTF8, 0, pszFileName, iLenStr+1, wFileName, COUNTOF(wFileName));
#endif

	// EncoderParameters encoderParams;
	Gdiplus::Status status = pBitmap->Save( wFileName, &encoderClsid, NULL );  

	HRESULT hRes;
	if ( status != Gdiplus::Ok )
	{
		if ( status == Gdiplus::Win32Error )
		{
			DWORD dwLastError = ::GetLastError();
			hRes = HRESULT_FROM_WIN32(dwLastError);
		}
		else
		{
			hRes = E_FAIL;
		}
		LOG_MSG(( "CTaksiGraphX::MakeScreenShot GDI+ err=0x%x" LOG_CR, hRes ));
	}
	else
	{
		hRes = S_OK;
	}
	delete pBitmap;	//done with it.
	return hRes;
}

#endif

char CodecName[][10]=
{
	{"DIVX"},
	{"JPEG"},
	{"DIVX"},
};


DWORD WINAPI _stdcall EncodingThreadProc(LPVOID lParam)
{

	CTaksiGraphX* pParentClass;
	pParentClass=(CTaksiGraphX *)lParam;
	pParentClass->iThreadStatus = THREAD_PAUSE;

	char szMP3FileName[256];
	double FrameRate;
	BYTE  * Buffer;
	int iMovieCount = 0;
	

	BITMAPINFO bi;

	register unsigned long time_old,time_start;
	unsigned long iTotalFrameCount = 0;
	unsigned long iTotalFrameErrorCount = 0;


//	register unsigned long time_old,time_start;
	long time_frame = 1000/sg_Shared.iFrameRate; 
	FrameRate = 1000.0/((double)time_frame) ;

	pParentClass->bFrameLocked = true;


	
	pParentClass->bFrameLocked = false;

	SYSTEMTIME time;
	::GetLocalTime(&time);

	//memset(sg_Shared.OutDirPath,0x00,255);
	//::GetCurrentDirectoryA(255,sg_Shared.OutDirPath);

	memset(sg_Shared.OutFilePath,0x00,sizeof(sg_Shared.OutFilePath));
	sprintf_s(sg_Shared.OutFilePath,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초_%d_1.avi",sg_Shared.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,iMovieCount);
	sprintf_s(szMP3FileName,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초.mp3",sg_Shared.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
    
	switch(ID_XVID)
	{
	case ID_BMP_TEMP:
		{
			pParentClass->m_h263Encoder.Init(sg_Shared.m_DestSize.cx,sg_Shared.m_DestSize.cy,sg_Shared.iKeyFrame);
			pParentClass->aviFile=AVI_open_output_file(sg_Shared.OutFilePath);
			AVI_set_video(pParentClass->aviFile,sg_Shared.m_DestSize.cx,sg_Shared.m_DestSize.cy,sg_Shared.iFrameRate,CodecName[ID_BMP_TEMP]);
			AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);
			AVI_set_Max_Len(80000000);
		}
		break;
	case ID_XVID:
		pParentClass->m_h263Encoder.Init(sg_Shared.m_DestSize.cx,sg_Shared.m_DestSize.cy,sg_Shared.iKeyFrame);
		pParentClass->aviFile=AVI_open_output_file(sg_Shared.OutFilePath);
		AVI_set_video(pParentClass->aviFile,sg_Shared.m_DestSize.cx,sg_Shared.m_DestSize.cy,sg_Shared.iFrameRate,CodecName[ID_XVID]);
		AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);
		AVI_set_Max_Len(2000000000);
		break;
	case ID_JEPG:
		break;
	}
	time_old=::timeGetTime();
	time_start = time_old;

	pParentClass->iThreadStatus = THREAD_START;

	while(sg_Shared.bRecordStart)
	{
		unsigned long now;

		now = ::timeGetTime();
		if(!pParentClass->bPause)
		if(now -time_old >= (unsigned long)time_frame)
		{
			time_old=now;
			if(!pParentClass->bFrameLocked)
			{
				//sg_Shared.SetRecord(true);
			}else {

				SIZE size;
				size = sg_Shared.m_DestSize;

				iTotalFrameCount ++;

			//sg_Shared.m_DestSize.cx , sg_Shared.m_DestSize.cy ,g_Proc.m_Stats.iBPP


				long time_now = time_old;
				long time_gap = time_now - time_start;
				int iTotalFrameDelayedCount;
				
				switch(ID_XVID)
				{
				case ID_BMP_TEMP:				
					{
						
					}
				case ID_XVID:

					int i;

					if(time_gap/(time_frame) > ((long)iTotalFrameCount))
					{
						iTotalFrameDelayedCount  = time_gap/time_frame - iTotalFrameCount;
						iTotalFrameErrorCount += iTotalFrameDelayedCount;
					} else if(time_gap/time_frame < (long)iTotalFrameCount){

					}
					pParentClass->m_h263Encoder.Compress(pParentClass->frame0.m_pPixels,true);
					
					/*
					if(pParentClass->bPreview)
					{
						bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
						bi.bmiHeader.biWidth=sg_Shared.m_DestSize.cx;
						bi.bmiHeader.biHeight=sg_Shared.m_DestSize.cy;
						bi.bmiHeader.biPlanes=1;
						bi.bmiHeader.biBitCount=24;
						bi.bmiHeader.biCompression=BI_RGB;
						bi.bmiHeader.biSizeImage=bi.bmiHeader.biBitCount/8*sg_Shared.m_DestSize.cx*sg_Shared.m_DestSize.cy;
						pParentClass->m_h263Encoder.DrawImage(sg_Shared.m_hPreviewWnd,NULL,pParentClass->pFrameBuffer,&bi);
					}
					*/

					pParentClass->bFrameLocked = false;
					
					
					if( AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount()))
					{
						AVI_close(pParentClass->aviFile);

						iMovieCount++;
						memset(sg_Shared.OutFilePath,0x00,sizeof(sg_Shared.OutFilePath));
						sprintf_s(sg_Shared.OutFilePath,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초_%d_1.avi",sg_Shared.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,iMovieCount);
						pParentClass->aviFile=AVI_open_output_file(sg_Shared.OutFilePath);
						AVI_set_video(pParentClass->aviFile,sg_Shared.m_DestSize.cx,sg_Shared.m_DestSize.cy,sg_Shared.iFrameRate,CodecName[ID_XVID]);
						AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);


						AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());
					}
					
					for(i=0;i<(long)iTotalFrameErrorCount;i++ )
					{
						pParentClass->m_h263Encoder.Compress(NULL,true);
						AVI_write_frame(pParentClass->aviFile,(char *)0,0);	
					}

					iTotalFrameCount += iTotalFrameErrorCount;
					iTotalFrameErrorCount = 0;
					break;
				case ID_JEPG:
					
						
					break;
				}
			}
		} 
		Sleep(1);
	}
	AVI_close(pParentClass->aviFile);
	pParentClass->iThreadStatus = THREAD_STOP;

	pParentClass->bFrameLocked = true;

	pParentClass->pFrameBuffer = NULL;
	pParentClass->bFrameLocked = false;

	return 0x03;
}
	

DWORD WINAPI _stdcall WatchThread(LPVOID lParam)
{
	DWORD iCount = 0;
	while((!sg_Shared.bReset)&&(iCount <1000))
	{
		Sleep(10);
		iCount++;
	}

	iCount = 0;

	if(::IsWindow(sg_Shared.m_hWndHookOld))
	{

		if ( g_Proc.m_bIsProcessSpecial )
		return 0;
		
		if ( g_Proc.AttachGraphXAPIs( (HWND) sg_Shared.m_hWndHookOld ) == S_OK )
		{
			g_Proc.CheckProcessCustom();	// determine frame capturing algorithm 
		}

		g_Proc.m_hWndHookTry = FindWindowTop((HWND) sg_Shared.m_hWndHookOld);	// top parent. not WS_CHILD.
		if ( g_Proc.m_hWndHookTry == NULL )
		{
			g_Proc.m_hWndHookTry = ((HWND) sg_Shared.m_hWndHookOld);

		}
	
	}

	return 1;
}


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
	return S_OK;
}

//*****************************************

void CTaksiGraphX::RecordAVI_Reset()
{
	
	sg_Shared.m_hWndHookOld = NULL;

	DWORD thread_ID;
	CreateThread(NULL,				//default security
				0,					//default stack
				WatchThread,			//use this thread function
				NULL,				//user data sent to thread
				0,					//creation flags, 0=start now
				&thread_ID);

	
	int iTemp;
	COPYDATASTRUCT MyCDS;
	MyCDS.dwData = 5; //send window infomation
	MyCDS.cbData = sizeof(iTemp);
	MyCDS.lpData = &iTemp;
	::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);

}

HRESULT CTaksiGraphX::RecordAVI_Start()
{
	
	return S_OK;
}

void CTaksiGraphX::RecordAVI_Stop()
{
	
}

bool CTaksiGraphX::RecordAVI_Frame()
{
	
	return true;
}

//********************************************************

void CTaksiGraphX::ProcessHotKey( TAKSI_HOTKEY_TYPE eHotKey )
{
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
}

void CTaksiGraphX::PresentFrameBegin( bool bChange )
{
	ASSERT( m_bHookedFunctions );

	// Switching to hook some other app?
	if ( sg_Shared.IsHookCBT())
	{
		// Success!! we hooked a process! make this the prime process.
		if ( ! g_Proc.IsProcPrime())
		{
			LOG_MSG(( "PresentFrameBegin: New Prime Focus." LOG_CR));
			sg_ProcStats.CopyProcStats( g_Proc.m_Stats );
			sg_Shared.HookCBT_Uninstall(); // thread safe?
		}
	}
	else if ( ! g_Proc.IsProcPrime())
	{
		// Dispose of myself! a new process has taken over.
		g_Proc.m_bStopGraphXAPI = true;
		return;
	}

	if ( sg_Shared.m_bMasterExiting )
		return;

	// Declare we have a working GraphXAPI hook.
	if ( ! g_Proc.StartGraphXAPI( get_GraphXAPI()))
	{
		// mode not allowed!
		return;	
	}

	DrawIndicator( (TAKSI_INDICATE_TYPE)sg_Shared.iIndicateType );
}

void CTaksiGraphX::PresentFrameEnd()
{
	// check unhook flag

	COPYDATASTRUCT MyCDS;

	if (!m_bGotFrameInfo)
	{
		// Determine HWND for our app window, and window/device dimensions. 
		DEBUG_TRACE(( "PresentFrameBegin.GetFrameInfo" LOG_CR ));
		g_Proc.m_Stats.m_hWndCap = GetFrameInfo( g_Proc.m_Stats.m_SizeWnd,g_Proc.m_Stats.iBPP );	// virtual
		if ( ! g_Proc.m_Stats.m_hWndCap )	
		{
			// This is fatal i can not proceed!
			LOG_WARN(( "PresentFrameBegin.GetFrameInfo FAILED!" LOG_CR));
			return;	// i can do nothing else here!
		}
		g_Proc.UpdateStat(TAKSI_PROCSTAT_Wnd);
		g_Proc.UpdateStat(TAKSI_PROCSTAT_SizeWnd);
		m_bGotFrameInfo = true;
	}

	// Force hotkeys on the prime process.
	if (!g_HotKeys.m_bAttachedHotKeys)
	{
		// determine how we are going to handle keyboard hot-keys:
		g_HotKeys.AttachHotKeysToApp();
	}
#ifdef USE_DX
	// Process DirectInput input. polled.
	if ( g_UserDI.m_bSetup)
	{
		g_UserDI.ProcessDirectInput(); // will call ScheduleHotKey()
	}
#endif

	DWORD dwHotKeyMask = g_HotKeys.m_dwHotKeyMask;
	g_HotKeys.m_dwHotKeyMask = 0;
	if ( sg_Shared.m_dwHotKeyMask && g_Proc.IsProcPrime())
	{
		dwHotKeyMask |= sg_Shared.m_dwHotKeyMask;
		sg_Shared.m_dwHotKeyMask = 0;
	}
	for ( int i=0; dwHotKeyMask; i++, dwHotKeyMask>>=1 )
	{
		if ( dwHotKeyMask & 1 )
		{
			ProcessHotKey( (TAKSI_HOTKEY_TYPE) i );
		}
	}

	MyCDS.dwData = 3; //send window infomation
	MyCDS.cbData = sizeof(HWND);
	MyCDS.lpData = &g_Proc.m_hWndHookTry;
	::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);

	if(sg_Shared.bRecordStart)
	{
		
		if(iThreadStatus == THREAD_WAIT)
		{
			bFrameLocked = false;
			DWORD thread_ID;
		//	pFrameBuffer = new BYTE[GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4];
	
			frame0.FreeFrame();
			frame1.FreeFrame();

			frame0.AllocPadXY( sg_Shared.m_DestSize.cx , sg_Shared.m_DestSize.cy ,g_Proc.m_Stats.iBPP);
			frame1.AllocPadXY( sg_Shared.m_DestSize.cx , sg_Shared.m_DestSize.cy ,g_Proc.m_Stats.iBPP);

			HANDLE hThread = CreateThread(NULL,				//default security
				0,					//default stack
				EncodingThreadProc,			//use this thread function
				this,				//user data sent to thread
				0,					//creation flags, 0=start now
				&thread_ID);
			//SetThreadPriority (hThread , THREAD_PRIORITY_TIME_CRITICAL);
			CloseHandle(hThread );	

			
		}		
		/*
		MyCDS.dwData = 0;
		MyCDS.cbData = sizeof(SIZE);
		MyCDS.lpData = &frame1.m_Size;
		::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);

		MyCDS.dwData = 1;
		MyCDS.cbData = sizeof(frame1.s_bbFormat);
		MyCDS.lpData = &frame1.s_bbFormat;
		::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);

		*/


		if((iThreadStatus == THREAD_START))
		{
			if(!bFrameLocked)
			{
				HRESULT hRes = GetFrame( frame0,0 );	// virtual
				//if(pFrameBuffer != NULL)
				//memcpy(pFrameBuffer,frame0.m_pPixels,frame1.m_Size.cx*frame1.m_Size.cy*frame1.m_iBPP);
				bFrameLocked = true;			
			} else {
				//HRESULT hRes = GetFrame( frame1,0 );	// virtual
			}
		}  
		
		/*
			MyCDS.dwData = 2;
			MyCDS.cbData = frame1.m_Size.cx*frame1.m_Size.cy*frame1.m_iBPP;
			MyCDS.lpData = frame1.m_pPixels;
			::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);
		*/
		
	} else {
		
		if(iThreadStatus == THREAD_STOP)
		{
			iThreadStatus = THREAD_WAIT;
		//	delete [] pFrameBuffer;
		}
	}
	
	

	if ( sg_Shared.m_bMasterExiting || g_Proc.m_bStopGraphXAPI )
	{
		DEBUG_TRACE(( "PresentFrameEnd:StopGraphXAPI." LOG_CR ));
		UnhookFunctions();					// we're ordered to unhook methods.
		g_Proc.StopGraphXAPIs();
		// the dll should now unload?
		return;
	}

	// check if we need to reconfigure
	if ( sg_Shared.m_dwConfigChangeCount != g_Proc.m_dwConfigChangeCount && 
		! g_AVIFile.IsOpen())
	{
		LOG_MSG(( "PresentFrameEnd:m_dwConfigChangeCount (%d)..." LOG_CR, g_Proc.m_dwConfigChangeCount ));
		g_Proc.CheckProcessCustom();
	}
}

HRESULT CTaksiGraphX::AttachGraphXAPI()
{
	// DLL_PROCESS_ATTACH
	// Check the DLL that would support this graphics mode.
	// NOTE: Dont force graphics DLL to load. just look for its presense.
	// ONLY CALLED FROM: AttachGraphXAPIs()
	// Not fully attached til PresentFrameBegin() is called.

	//::MessageBoxExA(NULL,"test","TEST",MB_OK,0);
	if ( ! sg_Config.m_abUseAPI[ get_GraphXAPI() ] )	// not allowed.
	{
		return HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND);
	}

	const TCHAR* pszName = get_DLLName();	// virtual

	
	//DEBUG_MSG(( "AttachGraphXAPI NO '%s'" LOG_CR, pszName ));
	if ( !FindDll(pszName)) // already loaded?
	{
		DEBUG_MSG(( "AttachGraphXAPI NO '%s'" LOG_CR, pszName ));
		//::MessageBoxExA(NULL,pszName,"TEST",MB_OK,0);
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
	LOG_MSG(( "AttachGraphXAPI '%s'" LOG_CR, pszName ));
	// NOTE: a hook is not truly complete til PresentFrameBegin() is called.
	m_bGotFrameInfo = false;	// must update this ASAP.

	COPYDATASTRUCT MyCDS;
	MyCDS.dwData = 4; //send window infomation
	MyCDS.cbData = sizeof(HWND);
	MyCDS.lpData = &g_Proc.m_hWndHookTry;
	::SendMessage(sg_Shared.m_hMasterWnd,WM_COPYDATA,NULL,(LPARAM)&MyCDS);

	iThreadStatus = THREAD_WAIT;
		 
	return S_OK;
}
