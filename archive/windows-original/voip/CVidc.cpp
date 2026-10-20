// CVIDC.CPP
// VIDEO CODEC MANAGER
//
//
// (c)V.Gorbatenko	1996-2000
// All rights reserved
// gvv@mail.tomsknet.ru
//

#include "stdafx.h"
#include "cvidc.h"
#include "Vfw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma comment(lib, "vfw32.lib")

LPCSTR	_Wstr2Mstr(LPCWSTR InStr);
int		_fstr(CStringArray &str, LPCSTR);


/////////////////////////////////////////////
LPCSTR _Wstr2Mstr(LPCWSTR InStr)
{

	static char OutStr[256];

	WideCharToMultiByte(	CP_ACP,
 			WC_COMPOSITECHECK|WC_DEFAULTCHAR, 
 			InStr, 
 			-1, 
 			OutStr, 
 			256, 
 			NULL, 
 			NULL);
	return 	   OutStr;
}

int	_fstr(CStringArray &str, LPCSTR s)
{
	for(int i=0;i<str.GetSize();i++)
		if(str[i]==s)	return i;
	return -1;
}


////////////////////////////////////////////
// CVidc	class	implementation
//
CVidc::CVidc()
{
	_selfInit();
}


CVidc::~CVidc()
{
	Close();
}

//static method
LPSTR	CVidc::GetCompressorName(DWORD dwfccOut)
{
	//open
	HIC hic = ICOpen(ICTYPE_VIDEO, dwfccOut,  ICMODE_QUERY);
 	if(!hic)	return NULL;

	ICINFO icinfo;
    
	//get info
	ICGetInfo(hic, &icinfo, sizeof(icinfo));
    
	//convert "info"
    LPSTR ret= (LPSTR)_Wstr2Mstr(icinfo.szDescription);
	
	ICClose(hic);
	return ret;
}

BOOL	CVidc::OpenCompressor(DWORD dwfccOut, BOOL fQuery)
{
		

		DWORD mode;

		if(fQuery)
		
			mode=ICMODE_QUERY;
		
		else
		
			mode=ICMODE_FASTCOMPRESS; //Although,  i've found no diff's with ICMODE_COMPRESS!!! :-)
		

		if(m_hIC)	
			if( mode==m_dwMode && m_dwFcc==dwfccOut)
				return TRUE;
			else	return FALSE;
	
	    m_dwMode=0;
		m_dwFcc =0;
	
	    m_hIC = ICOpen(ICTYPE_VIDEO, dwfccOut, mode);
	
		if(m_hIC)
		{
		 _getName();
			m_dwMode=mode;			// actual mode
				m_dwFcc=dwfccOut;	// actual fcc
		}
	
	return (BOOL)m_hIC;
}

BOOL	CVidc::OpenDecompressor(DWORD fcc)
{

	
	DWORD mode=ICMODE_FASTDECOMPRESS; //same note as for OpenCompressor!
	
	//is already opened?
	if(m_hIC)	
		if(mode==m_dwMode&&m_dwFcc==fcc)
			return TRUE;
				else	return FALSE;
	
	    	m_dwMode=0;
			m_dwFcc =0;
	
	    m_hIC = ICOpen(ICTYPE_VIDEO, fcc, mode);
	
	if(m_hIC) {
				_getName();
				m_dwMode=mode;
				m_dwFcc=fcc;
				}
		
		return (BOOL)m_hIC;

}

BOOL	CVidc::OpenDecompressor(LPBITMAPINFOHEADER lpInputBmp)
{

		ICINFO icinfo;
		if(!lpInputBmp||m_hIC)	return FALSE;
		
		//walk over all codecs, find desired 
		for (int i=0; ICInfo(ICTYPE_VIDEO, i, &icinfo); i++)
		{
		m_hIC=ICLocate(icinfo.fccType, icinfo.fccHandler, lpInputBmp, 
			NULL, ICMODE_FASTDECOMPRESS);
	
			if (m_hIC) {
				//this one!
	    		_getName();
	    		m_dwMode=ICMODE_FASTDECOMPRESS;
	    		m_dwFcc=icinfo.fccHandler;
	    		return TRUE;}
		}	
		return FALSE;

}

/////////////////////////////////////////////////////////////
#define DibPtr(lpbi)            (LPBYTE)(DibColors(lpbi) + (UINT)(lpbi)->biClrUsed)
#define DibColors(lpbi)         ((LPRGBQUAD)((LPBYTE)(lpbi) + (int)(lpbi)->biSize))

int DIBNumColors (LPBITMAPINFOHEADER lpbi)
{
      if (lpbi->biClrUsed)
         return (int)lpbi->biClrUsed;
   switch (lpbi->biBitCount)
      {
      case 1:
         return 2;
      case 4:
         return 16;
      case 8:
         return 256;
      default:
         return 0;	//16,24,32 bits bitmap has no color table
      }
}

// static
// Just a small service
LPBYTE	CVidc::LockDIB(HDIB dib, LPBITMAPINFO *lpBI)
{
	if(!dib)	return NULL;
    *lpBI=(LPBITMAPINFO)GlobalLock(dib);
	LPBITMAPINFOHEADER  lpBIF=(LPBITMAPINFOHEADER)(*lpBI);
    return	DibPtr(lpBIF);
}
HDIB	CVidc::UnlockDIB(HDIB hdib)
{
	if(!hdib)	return NULL;
    while(GlobalUnlock(hdib));
    return	hdib;
}
///
#define WIDTHBYTES(bits)        (((bits) + 31) / 32 * 4)
#define IM_SBI_RB(lpbi)			(WIDTHBYTES(((LPBITMAPINFOHEADER)lpbi)->biWidth * ((LPBITMAPINFOHEADER)lpbi)->biBitCount)*((LPBITMAPINFOHEADER)lpbi)->biHeight)
#define IM_S(lpbi)	((LPBITMAPINFOHEADER)lpbi)->biCompression == BI_RGB ?\
					IM_SBI_RB(lpbi) :((LPBITMAPINFOHEADER)lpbi)->biSizeImage

HDIB	CVidc::Compress(LPBITMAPINFO lpInBmp,LPBYTE lpData, BOOL bUse_any)
{
	

	if(!m_hIC) return FALSE;
	if( m_dwMode!=ICMODE_FASTCOMPRESS&&
	    m_dwMode!=ICMODE_COMPRESS)	
	   	return FALSE;
	if(!lpInBmp||!lpData)	return FALSE;
	
	HDIB retd;

	m_dwMode=ICMODE_COMPRESS;
	
	if(!bUse_any) 
	{
		retd =  (HDIB)ICImageCompress(
                            m_hIC,			// compressor to use (this!)
                            0,
                            lpInBmp, 		// format to compress from
                            lpData,  		// bits to compress
                            NULL,           // output format (default)
                            100*m_iCompressionQuality,  // quality to use.
                            NULL);          // size of output (whatever)

	}
	else
	{
	//Let ICM to select codec...
	retd =(HDIB)ICImageCompress(
                            NULL,		// Any codec!
                            0,          
                            lpInBmp, 	// format to compress from
                            lpData,  	// bits to compress
                            NULL,       // output format (default)
                            100*m_iCompressionQuality,// quality to use (0-10000)
                            NULL);		// size of output (whatever)
	}
	
	if(!retd)	
	{// strange things...

	TRACE("WARNING! Compression with %s failed. preforms direct copy",GetName());
	return NULL;

	}

	return retd;		
}

////
HDIB	CVidc::Decompress(LPBITMAPINFO lpInputBmp,
						  LPBITMAPINFO lpOutputBmp, 
						  LPBYTE lpInputBits)
{

	if( m_dwMode!=ICMODE_FASTDECOMPRESS&&
	    m_dwMode!=ICMODE_DECOMPRESS)	

		return NULL;

	if(((LPBITMAPINFOHEADER)lpInputBmp)->biCompression == BI_RGB)

		return NULL;
	
	if(!lpInputBmp||!lpInputBits)	
		
		return NULL;
	
	HDIB retd;
	
    retd = (HDIB)ICImageDecompress(
                            m_hIC,          // codec to use
                            0,				// codec flags
                            lpInputBmp,		// format to decompress from
                            lpInputBits,	// bits to decompress
                            lpOutputBmp);	// format to compress to.

	if(retd )	
		return retd;
	else
	//Let ICM to select codec...
	retd = (HDIB)ICImageDecompress(
                            NULL,			// Any codec!
                            0,				// codec flags
                            lpInputBmp,		// format to decompress from
                            lpInputBits,	// bits to decompress
                            NULL);			// format to compress to (any!).
  return retd;		

}

//Other version for decompressing with "key frames"
BOOL	CVidc::DecompressEx(LPBITMAPINFO lpInputBmp, LPBYTE lpInputBits, 
							LPBITMAPINFO lpOutputBmp, LPBYTE lpOutputBits, DWORD dwFlags)
{
	if( m_dwMode!=ICMODE_FASTDECOMPRESS&&
	    m_dwMode!=ICMODE_DECOMPRESS)	

		return FALSE;

	if(((LPBITMAPINFOHEADER)lpInputBmp)->biCompression == BI_RGB)

		return FALSE;
	
	if( !lpInputBmp|| !lpInputBits ||
		!lpOutputBmp || !lpOutputBits)	
		
		return FALSE;
	return ICDecompressEx(	m_hIC,
							dwFlags,
							(LPBITMAPINFOHEADER)lpInputBmp,
							lpInputBits,
							0,
							0,
							((LPBITMAPINFOHEADER)lpInputBmp)->biHeight,  
							((LPBITMAPINFOHEADER)lpInputBmp)->biWidth,   
							(LPBITMAPINFOHEADER) lpOutputBmp,  
							lpOutputBits,                
							0,                    
							0,                    
							((LPBITMAPINFOHEADER)lpInputBmp)->biHeight,
							((LPBITMAPINFOHEADER)lpInputBmp)->biWidth) == ICERR_OK;                    

}

VOID	CVidc::Close()
{
	if(m_hIC)
			ICClose(m_hIC);
	//clean up manager state
	_selfInit();
}

BOOL	CVidc::SetProgressCallback(LONG lParam, VOID *cb)
{
	if(!m_hIC)	return FALSE;
	return ICSetStatusProc(m_hIC, 0, 
		lParam, (long(__stdcall *)(long,unsigned int,long))cb)==ICERR_OK;
}


//static
//Fill string array with installed codecs names
BOOL	CVidc::EnumCodecs(CStringArray &names, 
						  CDWordArray &fcc, 
						  CDWordArray &QualitySupport)
{
	HIC hic;
	ICINFO icinfo;
	
	names.RemoveAll( );
	 fcc.RemoveAll( );
	  QualitySupport.RemoveAll();

	for (int i=0; ICInfo(ICTYPE_VIDEO, i, &icinfo); i++)
	{
    hic = ICOpen(icinfo.fccType, icinfo.fccHandler, ICMODE_QUERY);

     if (hic)
     {
        // find out the compressor name.
        ICGetInfo(hic, &icinfo, sizeof(icinfo));
        DWORD sup=icinfo.dwFlags&VIDCF_QUALITY;
        // add codec name to the list
        LPCSTR	my=_Wstr2Mstr(icinfo.szDescription);
	
        int ff=_fstr(names, my);
		if(ff!=-1)
		{//remove duplicate
		 names.RemoveAt( ff);
		  fcc.RemoveAt( ff);
			QualitySupport.RemoveAt( ff);
		}

     names.Add(my);
	  fcc.Add(icinfo.fccHandler);
	   QualitySupport.Add(sup);
    	ICClose(hic);
     }
	}//end loop

	return (BOOL)fcc.GetSize();
}


BOOL	CVidc::QueryAboutDlg()
{
		if(!m_hIC)	return FALSE;
		// WARNING! Here is a mistake in SDK documentaion!
		// This macro returns TRUE/FALSE instead of ICERR_OK/ICERR_UNSUPPORTED 
		// see also definition in "VFW.H"
		return ICQueryAbout(m_hIC);
}

BOOL	CVidc::QueryConfigDlg()
{
		if(!m_hIC)	return FALSE;
		// WARNING! Here is a mistake in SDK documentaion!
		// This macro returns TRUE/FALSE instead of ICERR_OK/ICERR_UNSUPPORTED 
		// see also definition in "VFW.H"
		return ICQueryConfigure(m_hIC);
}


BOOL	CVidc::QueryCompressInputFormat(LPBITMAPINFOHEADER   lpInputBmp)
{
	if(!m_hIC) return FALSE;
	if( m_dwMode==ICMODE_FASTDECOMPRESS||
	    m_dwMode==ICMODE_DECOMPRESS)
	   	return FALSE;

	return ICCompressQuery(m_hIC, lpInputBmp, NULL)==ICERR_OK;
}

BOOL	CVidc::AboutDlg(HWND hwnd)
{
	if(!m_hIC)	return FALSE;
	return ICAbout(m_hIC, hwnd) == ICERR_OK;
}

BOOL	CVidc::ConfigDlg(HWND hParentWnd)
{
	if(!m_hIC)	return FALSE;
	
	if(ICConfigure(m_hIC, hParentWnd)==ICERR_OK){
			m_iCompressionQuality=ICGetDefaultQuality(m_hIC)/1000;
			return TRUE;
	}
	return FALSE;
}

///////////////////// PROTECTED ////////////////////////////////
HIC	CVidc::_open4Query(int Indx)
{

	ICINFO icinfo;
	if(!ICInfo(ICTYPE_VIDEO, Indx, &icinfo))
				return FALSE;
    return ICOpen(icinfo.fccType, icinfo.fccHandler, ICMODE_QUERY);
}

void CVidc::_selfInit()
{
 	m_dwMode	= 0;
	m_dwFcc		= BI_RGB;
	m_hIC		= NULL;
	m_Name		= "";
	m_iCompressionQuality	= 100;//maximum
}

void	CVidc::_getName()
{
		ICINFO icinfo;
        ICGetInfo(m_hIC, &icinfo, sizeof(icinfo));
        
		m_Name=_Wstr2Mstr(icinfo.szDescription);
		m_dwFcc=icinfo.fccHandler;
}


