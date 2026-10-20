// CVIDC.H
// VIDEO CODEC MANAGER
//
// Public Class definition header
// RELEASE 1099 :
//		
// Last Edition 06.10.00
//__________________________________________________________________________________________

#ifndef _VIDC_INCL


#if !defined(_INC_VFW)
    #define	NODRAWDIB
    #define	NOAVIFMT
    #define	NOMMREG
    #define	NOAVIFILE
    #define	NOMCIWND
    #define	NOMSACM
    #define	NOVIDEO
    #include "vfw.h"
#endif

#define _VIDC_INCL

typedef HANDLE HDIB;

class CVidc
{

public:
	CVidc();
	~CVidc();
	
	BOOL	OpenCompressor(DWORD dwFccOutput, BOOL fQuery=FALSE);
	BOOL	OpenDecompressor(LPBITMAPINFOHEADER lpInBmp);
	BOOL	OpenDecompressor(DWORD dwFccInput);
	BOOL	SetProgressCallback(LONG lParam, VOID *cb);


	HDIB	Compress(LPBITMAPINFO lpInputBmp, LPBYTE lpData, BOOL fUse_any=TRUE);
	HDIB	Decompress(LPBITMAPINFO lpInputBmp, LPBITMAPINFO lpOutputBmp, LPBYTE lpInb);
	BOOL	DecompressEx(LPBITMAPINFO lpInputBmp, LPBYTE lpInb, LPBITMAPINFO lpOutputBmp, LPBYTE lpOutb, DWORD dwFlags);
	
	VOID	Close();

	LPCSTR	GetName();
	DWORD	GetFcc();
	BOOL	IsOpened();
	
	//static methods
static	BOOL	EnumCodecs(CStringArray &names, CDWordArray &fcc,CDWordArray &QualitySupport);
static	LPSTR	GetCompressorName(DWORD dwFccOut);

	//misc
static	LPBYTE	LockDIB(HDIB hdib, LPBITMAPINFO *lpBI);
static	HDIB	UnlockDIB(HDIB hdib);
static	VOID	FreeDIB(HDIB hdib)	{if(hdib) GlobalFree(hdib);}

	//Query
	BOOL	QueryAboutDlg();
	BOOL	QueryConfigDlg();
	BOOL	QueryCompressInputFormat(LPBITMAPINFOHEADER   lpInBmp);
	BOOL	QueryDecompressFormat(LPBITMAPINFOHEADER lpInBmp, LPBITMAPINFOHEADER lpOutBmp=NULL);
	
	//dialogs
	BOOL	AboutDlg(HWND hwnd);
	BOOL	ConfigDlg(HWND hwnd);

	//attribute
	INT		m_iCompressionQuality;

protected:
	HIC		_open4Query(int indx);
	void	_selfInit();
	void	_getName();

	DWORD	m_dwFcc;
	HIC		m_hIC;
	CString	m_Name;
	ICINFO	m_icinfo;
	DWORD	m_dwMode;
};

//inlines
inline 	LPCSTR	CVidc::GetName()
	{return (LPCSTR)m_Name;}
inline	DWORD	CVidc::GetFcc()
	{return m_dwFcc;}
inline	BOOL	CVidc::IsOpened()
	{return (BOOL)m_hIC;}

#endif
