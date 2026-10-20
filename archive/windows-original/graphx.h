//
// graphx.h
//
#pragma once

#include "Cdll.h"
#include "iRefPtr.h"

#define USE_DX 

// indicator dimensions/position
#define INDICATOR_X 8
#define INDICATOR_Y 8
#define INDICATOR_Width 16
#define INDICATOR_Height 16


#ifndef COUNTOF			// similar to MFC _countof()
#define COUNTOF(a) 		(sizeof(a)/sizeof((a)[0]))	// dimensionof() ? = count of elements of an array
#endif
#define ISINTRESOURCE(p)	(HIWORD((DWORD)(p))==0)	// MAKEINTRESOURCE()
#define GETINTRESOURCE(p)	LOWORD((DWORD)(p))
inline bool FILE_IsDirSep( TCHAR ch ) { return(( ch == '/')||( ch == '\\')); }

enum TAKSI_INTF_TYPE
{
	// DX interface entry offsets.
	TAKSI_INTF_QueryInterface = 0,	// always 0
	TAKSI_INTF_AddRef = 1,
	TAKSI_INTF_Release = 2,

	TAKSI_INTF_DX8_Reset = 14,
	TAKSI_INTF_DX8_Present = 15,

	TAKSI_INTF_DX9_Reset = 16,
	TAKSI_INTF_DX9_Present = 17,
};


enum TAKSI_INDICATE_TYPE
{
	// Indicate my current state to the viewer
	// Color a square to indicate my mode.
	TAKSI_INDICATE_Ready = 0,
	TAKSI_INDICATE_Hooked,	// CBT is hooked. (looking for new app to load)
	TAKSI_INDICATE_Recording,	// actively recording right now
	TAKSI_INDICATE_RecordPaused,	
	//TAKSI_INDICATE_Error,		// sit in error state til cleared ??
	TAKSI_INDICATE_QTY,
};


enum TAKSI_HOTKEY_TYPE
{
	// Monitor these hot keys.
	TAKSI_HOTKEY_ConfigOpen,
	TAKSI_HOTKEY_HookModeToggle,
	TAKSI_HOTKEY_IndicatorToggle,
	TAKSI_HOTKEY_RecordBegin,
	TAKSI_HOTKEY_RecordPause,
	TAKSI_HOTKEY_RecordStop,
	TAKSI_HOTKEY_Screenshot,
	TAKSI_HOTKEY_SmallScreenshot,
	TAKSI_HOTKEY_QTY,
};

enum TAKSI_CUSTOM_TYPE
{
	TAKSI_CUSTOM_FrameRate,
	TAKSI_CUSTOM_FrameWeight,
	TAKSI_CUSTOM_Pattern,
	TAKSI_CUSTOM_QTY,
};

enum TAKSI_API_TYPE
{
	// enumerate the available modes we support.
	TAKSI_API_NONE = 0,
	TAKSI_API_GDI,	// prefer all other modes over this.
	TAKSI_API_OGL,	// a static linked dll, so can get a false positive.
#ifdef USE_DX
	TAKSI_API_DX8,
	TAKSI_API_DX9,	// highest priority, always pick DX9 (over others) if it is supported.
#endif
	TAKSI_API_QTY,
};



TCHAR* GetFileTitlePtr( TCHAR* pszPath );
HRESULT CreateDirectoryX( const TCHAR* pszDir );
bool Str_IsSpace( char ch );
char* Str_SkipSpace( const char* pszNon );

#ifndef CHOOKJUMP
#define CHOOKJUMP
struct CHookJump
{
	// Create a relative jump to hook an existing api call.
	// is this 5 bytes in 64 bit mode as well?
	// NOTE: NOT for use in hooking an interface or a vtable. that doesnt require a jump.
#define CHookJump_LEN 5 
public:
	CHookJump()
		: m_dwOldProtection(0)
	{
		m_Jump[0] = 0;
	}

	bool IsHookInstalled() const
	{
		return( m_Jump[0] != 0 );
	}
	bool InstallHook( LPVOID pFunc, LPVOID pFuncNew );
	void RemoveHook( LPVOID pFunc );
	
	void SwapOld( LPVOID pFunc )
	{
		// put back saved code fragment
		ASSERT(pFunc);
		memcpy(pFunc, m_OldCode, sizeof(m_OldCode));
	}
	void SwapReset( LPVOID pFunc )
	{
		// put back JMP instruction again
		if ( ! IsHookInstalled())	// hook has since been destroyed!
			return;
		ASSERT(pFunc);
		memcpy(pFunc, m_Jump, sizeof(m_Jump));
	}
public:
	DWORD m_dwOldProtection;			// used by VirtualProtect()
	BYTE m_OldCode[CHookJump_LEN];	// what was there previous.
	BYTE m_Jump[CHookJump_LEN];		// what do i want to replace it with.
};
#endif

struct CVideoFrameForm
{
	// The format of a single video frame.
	CVideoFrameForm()
		: m_iPitch(0)
		, m_iBPP(3)
	{
		m_Size.cx = 0;
		m_Size.cy = 0;
	}
	bool IsFrameFormInit() const
	{
		if ( m_Size.cx <= 0 )
			return false;
		if ( m_Size.cy <= 0 )
			return false;
		if ( m_iPitch <= 0 )
			return false;
		if ( m_iBPP <= 0 )
			return false;
		return true;
	}
	int get_SizeBytes() const
	{
		return m_iPitch * m_Size.cy;
	}
	void InitPadded( int cx, int cy, int iBPP=3, int iPad=sizeof(DWORD));

public:
	SIZE m_Size;		// Frame size in pixels. (check m_iPitch for padding)
	int m_iPitch;		// Bytes for a row with padding.
	int m_iBPP;			// Bytes per pixel.
};


struct CVideoFrame : public CVideoFrameForm
{
	// buffer to keep a single video frame 
public:
	CVideoFrame()
		: m_pPixels(NULL)
	{
	}
	~CVideoFrame()
	{
		FreeFrame();
	}

	bool IsValidFrame() const
	{
		return m_pPixels != NULL;
	}

	void FreeFrame();
	bool AllocForm( const CVideoFrameForm& form );
	bool AllocPadXY( int cx, int cy, int iBPP=3, int iPad=sizeof(DWORD));	// padded to 4 bytes

	void SetupBITMAPINFOHEADER( BITMAPINFOHEADER& bmih ) const;

	HRESULT SaveAsBMP( const TCHAR* pszFilename ) const;

public:
	BYTE* m_pPixels;	// Pixel data for the frame buffer.
};


struct CShared {
public:
	bool InitShared();
	void DestroyShared();

	HRESULT LogMessage( const TCHAR* pszPrefix );	// LOG_NAME_DLL
	void SendReHookMessage();

	bool IsHookCBT() const
	{
		return m_hHookCBT != NULL;
	}
	static LRESULT CALLBACK HookCBTProc(int nCode, WPARAM wParam, LPARAM lParam);
	bool HookCBT_Install(void);
	bool HookCBT_Uninstall(void);

	bool InitMasterWnd(HWND hWnd);
	void UpdateMaster();
	void UpdateConfigCustom();

	void HotKey_ConfigOpen();
	void HotKey_IndicatorToggle();
	void HotKey_HookModeToggle();

public:
	DWORD	m_dwVersionStamp;	// TAKSI_VERSION_N
	HWND	m_hMasterWnd;		// The master server EXE's window.
	bool	m_bMasterExiting;	// The master server is exiting so all attached DLL's should close down.
	HHOOK	m_hHookCBT;			// SetWindowsHookEx( WH_CBT ) for hooking new processes
	int		m_iProcessCount;	// how many processes attached?

	TCHAR m_szIniDir[_MAX_PATH];		// what dir to find the INI file and put the log files.

#define LOG_NAME_DLL	_T("TaksiDll.log")	// common log shared by all processes.

	DWORD m_dwConfigChangeCount;	// changed when the Custom stuff in m_Config changes.
	DWORD m_dwHotKeyMask;			// TAKSI_HOTKEY_TYPE mask from App. Schedule these in FrameBegin()
	bool m_bRecordPause;			// paused video record by command.
	int m_iMasterUpdateCount;		// how many WM_APP_UPDATE messages unprocessed.

#ifdef USE_DX
	// keep Direct3D-related pointers, tested just once for all.
	// DX8
	UINT_PTR m_nDX8_Present; // offset from start of DLL to the interface element i want.
	UINT_PTR m_nDX8_Reset;
	// DX9
	UINT_PTR m_nDX9_Present;
	UINT_PTR m_nDX9_Reset;
#endif
};

extern HINSTANCE g_hInst;
extern CSize m_SizeWnd;
extern CShared sg_Shared;
struct CTaksiGraphX : public CDllFile
{
	// a generic graphics mode base class.
	// Base for: CTaksiDX8, CTaksiDX9, CTaksiGDI etc,
	
public:
	CTaksiGraphX()
		: m_bHookedFunctions(false)
		, m_bGotFrameInfo(false)
	{
	}

	

	virtual const TCHAR* get_DLLName() const = 0;
	virtual TAKSI_API_TYPE get_GraphXAPI() const = 0;

	HRESULT AttachGraphXAPI();
	void PresentFrameBegin( bool bChange );
	void PresentFrameEnd();
	void RecordAVI_Reset();

protected:
	virtual HRESULT HookFunctions()
	{
		// ONLY CALLED FROM AttachGraphXAPI()
		ASSERT( IsValidDll());

		m_bHookedFunctions = true;
		return S_OK;
	}
	virtual void UnhookFunctions()
	{
		
		m_bHookedFunctions = false;
	}

	virtual HWND GetFrameInfo( SIZE& rSize ) = 0;
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize ) = 0;
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate ) = 0;

private:
	HRESULT RecordAVI_Start();
	void RecordAVI_Stop();
	bool RecordAVI_Frame();

	HRESULT MakeScreenShot( bool bHalfSize );

	void ProcessHotKey( TAKSI_HOTKEY_TYPE eHotKey );

public:
	static const DWORD sm_IndColors[TAKSI_INDICATE_QTY];
	bool m_bHookedFunctions;	// HookFunctions() called and returned true. may not have called PresentFrameBegin() yet tho.
	bool m_bGotFrameInfo;		// GetFrameInfo() called. set to false to re-read the info. PresentFrameBegin success.
};



#ifdef USE_DX

interface IDirect3DDevice8;
struct CTaksiDX8 : public CTaksiGraphX
{
	// TAKSIMODE_DX8
public:
	CTaksiDX8();
	~CTaksiDX8();

	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("d3d8.dll");
	}
	virtual TAKSI_API_TYPE get_GraphXAPI() const
	{
		return TAKSI_API_DX8;
	}

	virtual HRESULT HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects(bool bDetaching);

public:
	IDirect3DDevice8* m_pDevice;	// use this carefully, it is not IRefPtr locked
	ULONG m_iRefCount;	// Actual RefCounts i see by hooking the AddRef()/Release() methods.
	ULONG m_iRefCountMe;	// RefCounts that i know i have made.

	CHookJump m_Hook_Present;
	CHookJump m_Hook_Reset;
	UINT_PTR* m_Hook_AddRef;
	UINT_PTR* m_Hook_Release;
};
extern CTaksiDX8 g_DX8;

interface IDirect3DDevice9;
struct CTaksiDX9 : public CTaksiGraphX
{
	// TAKSIMODE_DX9
public:
	CTaksiDX9();
	~CTaksiDX9();

	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("d3d9.dll");
	}
	virtual TAKSI_API_TYPE get_GraphXAPI() const
	{
		return TAKSI_API_DX9;
	}

	virtual HRESULT HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

	HRESULT RestoreDeviceObjects();
	HRESULT InvalidateDeviceObjects();

public:
	IDirect3DDevice9* m_pDevice;	// use this carefully, it is not IRefPtr locked
	ULONG m_iRefCount;	// Actual RefCounts i see by hooking the AddRef()/Release() methods.
	ULONG m_iRefCountMe;	// RefCounts that i know i have made.

	CHookJump m_Hook_Present;
	CHookJump m_Hook_Reset;
	UINT_PTR* m_Hook_AddRef;
	UINT_PTR* m_Hook_Release;
};
extern CTaksiDX9 g_DX9;

#endif // USE_DX


/*
struct CTaksiGDI : public CTaksiGraphX
{
	// TAKSIMODE_GDI
public:
	CTaksiGDI()
		: m_hWnd(NULL)
		, m_WndProcOld(NULL)
		, m_iReentrant(0)
	{
	}
	
	virtual const TCHAR* get_DLLName() const
	{
		return TEXT("user32.dll");
	}
	virtual TAKSI_API_TYPE get_GraphXAPI() const
	{
		return TAKSI_API_GDI;
	}

	virtual HRESULT HookFunctions();
	virtual void UnhookFunctions();
	virtual void FreeDll();

	virtual HWND GetFrameInfo( SIZE& rSize );
	virtual HRESULT DrawIndicator( TAKSI_INDICATE_TYPE eIndicate );
	virtual HRESULT GetFrame( CVideoFrame& frame, bool bHalfSize );

private:
	void DrawMouse( HDC hMemDC, bool bHalfSize );
	static LRESULT CALLBACK WndProcHook( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

public:
	HWND m_hWnd;	// best window for the current process.
	RECT m_WndRect;	// system rectangle for the window.
	WNDPROC m_WndProcOld;	// the old handler before i hooked it.
	int m_iReentrant;
	DWORD m_dwTimeLastDrawIndicator;	// GetTickCount() of last.
	UINT_PTR m_uTimerId;	// did we set up the timer ?
};
extern CTaksiGDI g_GDI;
*/
inline void SwapColorsRGB( BYTE* pPixel )
{
	//ASSUME pixel format is in 8bpp mode.
	BYTE r = pPixel[0];
	pPixel[0] = pPixel[2];
	pPixel[2] = r;
}

bool Test_DirectX9( HWND hWnd );