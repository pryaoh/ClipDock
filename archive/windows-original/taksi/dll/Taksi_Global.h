#ifdef TAKSIDLL_EXPORTS
#define LIBSPEC __declspec(dllexport)
#else
#define LIBSPEC __declspec(dllimport)
#endif // TAKSIDLL_EXPORTS

#include "../common/Common.h"
#include "../common/IRefPtr.h"
#include "../common/CLogBase.h"
#include "../common/CAVIFile.h"	// CAVIFile
#include "../common/CWaveACM.h"

#include "../TaksiVersion.h"
#include "../Config.h"

typedef void CALLBACK UserDrawImageCallBack(void * Data,unsigned long  DataLen);
typedef UserDrawImageCallBack * pUserDrawImageCallBack;

#define USE_DX
#ifndef TAKSIGLOBAL
#define TAKSIGLOBAL
struct LIBSPEC CTaksiLogFile : public CLogFiltered
{
public:
	CTaksiLogFile() 
	{
		g_pLog = this;
	}
	~CTaksiLogFile()
	{
		CloseLogFile();
	}
	HRESULT OpenLogFile( const TCHAR* pszFileName );
	void CloseLogFile();
	virtual int EventStr( LOG_GROUP_MASK dwGroupMask, LOGL_TYPE eLogLevel, const LOGCHAR* pszMsg );
private:
	CNTHandle m_File;
};
extern LIBSPEC CTaksiLogFile g_Log;

// Messages to the Master Window.
#define WM_APP_UPDATE		(WM_APP + 0) // WM_USER ?
#define WM_APP_REHOOKCBT	(WM_APP + 1) // WM_USER ?
#define WM_APP_TRAY_NOTIFICATION (WM_APP+2)

#define IDC_HOTKEY_FIRST 200	// WM_COMMAND message = IDC_HOTKEY_FIRST + TAKSI_HOTKEY_TYPE (same as IDB_ConfigOpen)

#define TAKSI_MASTER_CLASSNAME _T("TaksiMaster")

#ifdef USE_DX
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
#endif

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

enum TAKSI_PROCSTAT_TYPE
{
#define ProcStatProp(a,b,c,d) TAKSI_PROCSTAT_##a,
#include "../ProcStatProps.tbl"
#undef ProcStatProp
	TAKSI_PROCSTAT_QTY,
};

#pragma pack(4)	// try to be explicit about this since its shared.
struct LIBSPEC CTaksiProcStats
{
	// Record relevant stats on the current foreground app/process.
	// Display stats in the dialog in real time. SHARED

public:
	void InitProcStats();
	void CopyProcStats( const CTaksiProcStats& stats );
	void UpdateProcStat( TAKSI_PROCSTAT_TYPE eProp )
	{
		ASSERT( eProp >= 0 && eProp < TAKSI_PROCSTAT_QTY );
		m_dwPropChangedMask |= (1<<eProp);
	}
	void CopyProcStat( const CTaksiProcStats& stats, TAKSI_PROCSTAT_TYPE eProp )
	{
		// Move just a single property.
		ASSERT( eProp >= 0 && eProp < TAKSI_PROCSTAT_QTY );
		const WORD wOffset = sm_Props[eProp][0];
		ASSERT( wOffset >= 0 && wOffset < sizeof(CTaksiProcStats));
		const WORD wSize = sm_Props[eProp][1];
		ASSERT( wSize > 0 && wSize < sizeof(CTaksiProcStats));
		const WORD wEnd = wSize+wOffset;
		ASSERT( wEnd > 0 && wEnd <= sizeof(CTaksiProcStats));
		memcpy(((BYTE*)this) + wOffset, ((const BYTE*)&stats) + wOffset, wSize );
		UpdateProcStat(eProp);
	}

	float get_DataRecMeg() const
	{
		return ((float) m_dwDataRecorded ) / (1024*1024);
	}

public:
	DWORD m_dwProcessId;		// What process is this?
	TCHAR m_szProcessFile[ _MAX_PATH ];	// What is the file name/path for the current process.

	// dynamic info.
	TCHAR m_szLastError[ _MAX_PATH ];	// last error message (if action failed)

	HWND m_hWndCap;	// the window the graphics is in
	SIZE m_SizeWnd;	// the window/backbuffer size. (pixels)
	SIZE m_SizeTarget;	// the window/backbuffer size. (pixels)
	int iBPP;
	TAKSI_API_TYPE m_eGraphXAPI;

	TAKSI_INDICATE_TYPE m_eState;	// What are we doing with the process?
	float m_fFrameRate;			// measured frame rate. recording or not.
	DWORD m_dwDataRecorded;		// How much video data recorded in current stream (if any)

public:
	static const WORD sm_Props[ TAKSI_PROCSTAT_QTY ][2]; // offset,size
	DWORD m_dwPropChangedMask;		// TAKSI_PROCSTAT_TYPE mask
};
extern LIBSPEC CTaksiProcStats sg_ProcStats;	// For display in the Taksi.exe app.
#pragma pack()	// try to be explicit about this since its shared.

#pragma pack(4)	// try to be explicit about this since its shared.


struct LIBSPEC CTaksiShared
{
	// This structure is interprocess SHARED!
	// NOTE: So it CANT have a constructor or contain any data types that do!!
public:


	bool InitShared();
	void SetDestSize(SIZE size);
	void SetRecord(bool bStart);
	void ReConnect(HWND hWnd);
	void DestroyShared();

	HRESULT LogMessage( const TCHAR* pszPrefix );	// LOG_NAME_DLL
	void SendReHookMessage();

	bool IsHookCBT() const
	{
		return m_hHookCBT != NULL;
	}
	static LRESULT CALLBACK HookCBTProc(int nCode, WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK HookMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
	bool HookCBT_Install(void);
	bool HookCBT_Uninstall(void);

	bool InitMasterWnd(HWND hWnd);
	void UpdateMaster();
	void UpdateConfigCustom();

	void HotKey_ConfigOpen();
	void HotKey_IndicatorToggle();
	void HotKey_HookModeToggle();
	void SetUserCallback(void * pCallback);
	void UserCallback(void * Data,unsigned long  DataLen);


public:
	DWORD	m_dwVersionStamp;	// TAKSI_VERSION_N
	HWND	m_hMasterWnd;		// The master server EXE's window.
	HWND	m_hPreviewWnd;		
	bool	m_bMasterExiting;	// The master server is exiting so all attached DLL's should close down.
	HHOOK	m_hHookCBT;			// SetWindowsHookEx( WH_CBT ) for hooking new processes
	HHOOK	m_hHookMouse;		
	int		m_iProcessCount;	// how many processes attached?
	bool	m_bCallback;
	bool    bReset;
	
	unsigned int iCodec;
	unsigned int iKeyFrame;
	unsigned int iFrameRate;
	char OutDirPath[255];
	char OutFilePath[255];

	TCHAR m_szIniDir[_MAX_PATH];		// what dir to find the INI file and put the log files.

#define LOG_NAME_DLL	_T("TaksiDll.log")	// common log shared by all processes.

	DWORD m_dwConfigChangeCount;	// changed when the Custom stuff in m_Config changes.
	DWORD m_dwHotKeyMask;			// TAKSI_HOTKEY_TYPE mask from App. Schedule these in FrameBegin()
	bool m_bRecordPause;			// paused video record by command.
	int m_iMasterUpdateCount;		// how many WM_APP_UPDATE messages unprocessed.

	HWND m_hWndHookOld;				// The last window we tried to hook 

	bool bRecordStart;
	int  iIndicateType;
	SIZE m_DestSize;

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
extern LIBSPEC CTaksiShared sg_Shared;
#pragma pack()


#define Alloc(p,t) (t *)malloc((p)*sizeof(t))
#define For(i,n) for ((i)=0;(i)<(n);(i)++)
#define iFor(n) For (i,n)
#define jFor(n) For (j,n)

typedef struct {
 WORD x,y;  // dimensions
 WORD l;    // bytes per scan-line (32-bit allignment)
 int  iDepth;
 BYTE *b;   // bits of bitmap,3 bytes/pixel, BGR
} tWorkBMP;  // 24-bit working bitmap



#pragma once

#include <math.h> 

#define BOUND(x,a,b) \
    (((x) <= (a)) ? (a) : (((x) > (b)) ? (b) : (x)))

#define PS_MAX_DEPTH 4
              
// Based on Eran Yariv's TwoPassScale, modified for speed, bug fixes, and variable depths.

// Does 2 pass scaling of bitmaps
// Template based, can use various methods for interpolation.
// Could certainly be improved.

//    to use:
//    C2PassScale  ScaleEngine;
//    ScaleEngine.Scale ((UCHAR *)dibSrc.GetBits(),		  // A pointer to the source bitmap bits
//                        depth,						  // The size of a single pixel in bytes (both source and scaled image)
//                        dibSrc.Width(),				  // The width of a line of the source image to scale in pixels
//                        dibSrc.WidthBytes(),			  // The width of a single line of the source image in bytes (to allow for padding, etc.)
//                        dibSrc.Height(),				  // The height of the source image to scale in pixels.
//                        (UCHAR *)GetBits(),			  // A pointer to a buffer to hold the ecaled image
//                        Width(),						  // The desired width of a line of the scaled image in pixels
//                        WidthBytes(),					  // The width of a single line of the scaled image in bytes (to allow for padding, etc.)
//                        Height());					  // The desired height of the scaled image in pixels.
//    or AllocAndScale()

// Modified 1-17-05 to use more integer math -- much faater. [JRM]


class CGenericFilter
{
public:
    
    CGenericFilter (double dWidth) : m_dWidth (dWidth) {}
    virtual ~CGenericFilter() {}

    double GetWidth()                   { return m_dWidth; }
    void   SetWidth (double dWidth)     { m_dWidth = dWidth; }

    virtual double Filter (double dVal) = 0;

protected:

    #define FILTER_PI  double (3.1415926535897932384626433832795)
    #define FILTER_2PI double (2.0 * 3.1415926535897932384626433832795)
    #define FILTER_4PI double (4.0 * 3.1415926535897932384626433832795)

    double  m_dWidth;
};

class CBoxFilter : public CGenericFilter
{
public:

    CBoxFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBoxFilter() {}

    double Filter (double dVal) { return (fabs(dVal) <= m_dWidth ? 1.0 : 0.0); }
};

class CBilinearFilter : public CGenericFilter
{
public:

    CBilinearFilter (double dWidth = double(1.0)) : CGenericFilter(dWidth) {}
    virtual ~CBilinearFilter() {}

    double Filter (double dVal)
        {
            dVal = fabs(dVal);
            return (dVal < m_dWidth ? m_dWidth - dVal : 0.0);
        }
};

class CGaussianFilter : public CGenericFilter
{
public:

    CGaussianFilter (double dWidth = double(3.0)) : CGenericFilter(dWidth) {}
    virtual ~CGaussianFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            return exp (-dVal * dVal / 2.0) / sqrt (FILTER_2PI);
        }
};

class CHammingFilter : public CGenericFilter
{
public:

    CHammingFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CHammingFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            double dWindow = 0.54 + 0.46 * cos (FILTER_2PI * dVal);
            double dSinc = (dVal == 0) ? 1.0 : sin (FILTER_PI * dVal) / (FILTER_PI * dVal);
            return dWindow * dSinc;
        }
};

class CBlackmanFilter : public CGenericFilter
{
public:

    CBlackmanFilter (double dWidth = double(0.5)) : CGenericFilter(dWidth) {}
    virtual ~CBlackmanFilter() {}

    double Filter (double dVal)
        {
            if (fabs (dVal) > m_dWidth)
            {
                return 0.0;
            }
            double dN = 2.0 * m_dWidth + 1.0;
            return 0.42 + 0.5 * cos (FILTER_2PI * dVal / ( dN - 1.0 )) +
                   0.08 * cos (FILTER_4PI * dVal / ( dN - 1.0 ));
        }
};


typedef struct
{
   unsigned int*Weights;  // Normalized weights of neighboring pixels
   int Left,Right;   // Bounds of source pixels window
} ContributionType;  // Contirbution information for a single pixel

typedef struct
{
   ContributionType *ContribRow; // Row (or column) of contribution weights
   UINT WindowSize,              // Filter window size (of affecting source pixels)
        LineLength;              // Length of line (no. or rows / cols)
} LineContribType;               // Contribution information for an entire line (row or column)



typedef BOOL (*ProgressAnbAbortCallBack)(BYTE bPercentComplete);

template <class FilterClass>
class C2PassScale
{
public:

    C2PassScale (ProgressAnbAbortCallBack callback = NULL) :
        m_Callback (callback) {m_byteDepth = 3;}

    virtual ~C2PassScale() {}

    UCHAR * AllocAndScale (  
        UCHAR   *pOrigImage,
        UINT         pixelBytes,
        UINT        uOrigWidth,
        UINT        uOrigWidthBytes,
        UINT        uOrigHeight,
        UINT        uNewWidth,
        UINT        uNewWidthBytes,
        UINT        uNewHeight);

    UCHAR * Scale (  
        UCHAR   *pOrigImage,
        UINT         pixelBytes,
        UINT        uOrigWidth,
        UINT        uOrigWidthBytes,
        UINT        uOrigHeight,
        UCHAR   *pDstImage,
        UINT        uNewWidth,
        UINT        uNewWidthBytes,
        UINT        uNewHeight);

        int                            m_byteDepth;

private:

    ProgressAnbAbortCallBack    m_Callback;
    BOOL                        m_bCanceled;

    LineContribType *AllocContributions (   UINT uLineLength,
                                            UINT uWindowSize);

    void FreeContributions (LineContribType * p);

    LineContribType *CalcContributions (    UINT    uLineSize,
                                            UINT    uSrcSize,
                                            double  dScale);

    void ScaleRow ( UCHAR           *pSrc,
                    UINT                uSrcWidth,
                    UINT                 uSrcWidthBytes,
                    UCHAR           *pRes,
                    UINT                uResWidth,
                    UINT                 uDstWidthBytes,
                    UINT                uRow,
                    LineContribType    *Contrib);

    void HorizScale (   UCHAR           *pSrc,
                        UINT                uSrcWidth,
                        UINT                 uSrcWidthBytes,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResWidth,
                        UINT                 uResWidthBytes);

    void ScaleCol ( UCHAR           *pSrc,
                    UINT                uSrcWidth,
                    UINT                 uSrcWidthBytes,
                    UCHAR           *pRes,
                    UINT                uResWidth,
                    UINT                 uResWidthBytes,
                    UINT                uResHeight,
                    UINT                uCol,
                    LineContribType    *Contrib);

    void VertScale (    UCHAR           *pSrc,
                        UINT                uSrcWidth,
                        UINT                 uSrcWidthBytes,
                        UINT                uSrcHeight,
                        UCHAR           *pDst,
                        UINT                uResHeight);
};

template <class FilterClass>
LineContribType *
C2PassScale< FilterClass>::
AllocContributions (UINT uLineLength, UINT uWindowSize)
{
    LineContribType *res = new LineContribType;
        // Init structure header
    res->WindowSize = uWindowSize;
    res->LineLength = uLineLength;
        // Allocate list of contributions
    res->ContribRow = new ContributionType[uLineLength];
    for (UINT u = 0 ; u < uLineLength ; u++)
    {
        // Allocate contributions for every pixel
        res->ContribRow[u].Weights = new unsigned int[uWindowSize];
    }
    return res;
}

template <class FilterClass>
void
C2PassScale< FilterClass>::
FreeContributions (LineContribType * p)
{
    for (UINT u = 0; u < p->LineLength; u++)
    {
        // Free contribs for every pixel
        delete [] p->ContribRow[u].Weights;
    }
    delete [] p->ContribRow;    // Free list of pixels contribs
    delete p;                   // Free contribs header
}

template <class FilterClass>
LineContribType *
C2PassScale< FilterClass>::
CalcContributions (UINT uLineSize, UINT uSrcSize, double dScale)
{
    FilterClass CurFilter;

    double dWidth;
    double dFScale = 1.0;
    double dFilterWidth = CurFilter.GetWidth();
	int iSrc ;

    if (dScale < 1.0)
    {    // Minification
        dWidth = dFilterWidth / dScale;
        dFScale = dScale;
    }
    else
    {    // Magnification
        dWidth= dFilterWidth;
    }

    // Window size is the number of sampled pixels
//    int iWindowSize = 2 * (int)ceil(dWidth) + 1;
    int iWindowSize = 2 * ((int)ceil(dWidth) + 1);     // changed ... causing crash with bi-linear filiter?? [JRM]

    // Allocate a new line contributions strucutre
    LineContribType *res = AllocContributions (uLineSize, iWindowSize);

    double *dWeights = new double[iWindowSize];

    for (UINT u = 0; u < uLineSize; u++)
    {   // Scan through line of contributions
        double dCenter = (double)u / dScale;   // Reverse mapping
        // Find the significant edge points that affect the pixel
        int iLeft = max (0, (int)floor (dCenter - dWidth));
        int iRight = min ((int)ceil (dCenter + dWidth), int(uSrcSize) - 1);

        // Cut edge points to fit in filter window in case of spill-off
        if (iRight - iLeft + 1 > iWindowSize)
        {
            if (iLeft < (int(uSrcSize) - 1 / 2))
            {
                iLeft++;
            }
            else
            {
                iRight--;
            }
        }
        int nFallback = iLeft;    

        BOOL bNonZeroFound = false;
        double dTotalWeight = 0.0;  // Zero sum of weights
        double dVal;
        for (iSrc = iLeft; iSrc <= iRight; iSrc++)
        {   // Calculate weights
            dVal = CurFilter.Filter (dFScale * (dCenter - (double)iSrc));
            if (dVal > 0.0)
                dVal *= dFScale;
            else
            {
                dVal = 0.0;
                // zero conribution, trim
                if (!bNonZeroFound)
                {
                    // we are on the left side, trim left
                    iLeft = iSrc+1;
                    continue;
                }
                else
                {
                    // we are on the right side, trim right
                    iRight = iSrc-1;
                    break;
                }
            }
            bNonZeroFound = true;
            dTotalWeight += dVal;
            dWeights[iSrc-iLeft] = dVal;
        }

        if (iLeft > iRight)
        {
            ASSERT(FALSE);
            iLeft = iRight = nFallback;
            dWeights[0] = 0.0;
        }
        res->ContribRow[u].Left = iLeft;
        res->ContribRow[u].Right = iRight;

        ASSERT (dTotalWeight >= 0.0);   // An error in the filter function can cause this
        if (dTotalWeight > 0.0)
        {   // Normalize weight of neighbouring points
            for (iSrc = iLeft; iSrc <= iRight; iSrc++)
            {   // Normalize point
                dWeights[iSrc-iLeft] /= dTotalWeight;
            }
        }
        // scale weights to integers weights for effeciency
        for (iSrc = iLeft; iSrc <= iRight; iSrc++)
            res->ContribRow[u].Weights[iSrc-iLeft] = (unsigned int)(dWeights[iSrc-iLeft] * 0xffff);
   }
   delete [] dWeights;
   return res;
}


template <class FilterClass>
void
C2PassScale< FilterClass>::
ScaleRow (  UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UCHAR           *pRes,
            UINT                uResWidth,
            UINT                 uResWidthBytes,
            UINT                uRow,
            LineContribType    *Contrib)
{
    UCHAR * const pSrcRow = &(pSrc[uRow * uSrcWidthBytes]);
    UCHAR * const pDstRow = &(pRes[uRow * uResWidthBytes]);
    UCHAR *pSrcLoc;
    UCHAR *pDstLoc;
    unsigned int vals[PS_MAX_DEPTH];

    for (UINT x = 0; x < uResWidth; x++)
    {   // Loop through row
        int v, i;
        for (v= 0; v < m_byteDepth; v++)
            vals[v] = 0;
        int iLeft = Contrib->ContribRow[x].Left;    // Retrieve left boundries
        int iRight = Contrib->ContribRow[x].Right;  // Retrieve right boundries
        pSrcLoc = &pSrcRow[iLeft*m_byteDepth];
        for (i = iLeft; i <= iRight; i++)
        {   // Scan between boundries
            #ifdef _DEBUG
                ASSERT(i-iLeft < (int)Contrib->WindowSize);
            #endif
            // Accumulate weighted effect of each neighboring pixel
            for (v= 0; v < m_byteDepth; v++)
                vals[v] += Contrib->ContribRow[x].Weights[i-iLeft] * *pSrcLoc++;
        }
        pDstLoc = &pDstRow[x*m_byteDepth];
        for (v= 0; v < m_byteDepth; v++)
        {
            // copy to destination, and scale back down by BYTE
            *pDstLoc++ = BOUND(vals[v] >> 16, 0, 0xff); // Place result in destination pixel
        }
    }
}

template <class FilterClass>
void
C2PassScale<FilterClass>::
HorizScale (    UCHAR           *pSrc,
                UINT                uSrcWidth,
                UINT                 uSrcWidthBytes,
                UINT                uSrcHeight,
                UCHAR           *pDst,
                UINT                uResWidth,
                UINT                 uResWidthBytes)
{
    // Assumes heights are the same
//    TRACE ("Performing horizontal scaling...\n");
    if (uResWidth == uSrcWidth)
    {   // No scaling required, just copy
        if(uSrcHeight <= 0) return;
        if (uResWidthBytes == uSrcWidthBytes)
        {
            int copy = ((uSrcHeight -1) * uSrcWidthBytes) + uSrcWidth*m_byteDepth; // avoids overrun if starting in middle of image.
               memcpy (pDst, pSrc, copy);
            return;
        }
        else
        {
            for (UINT y = 0; y < uSrcHeight; y++)
                memcpy(pDst+uResWidthBytes*y, pSrc+uSrcWidthBytes*y, uSrcWidth*m_byteDepth);
            return;
        }
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResWidth, uSrcWidth, double(uResWidth) / double(uSrcWidth));
    for (UINT u = 0; u < uSrcHeight; u++)
    {   // Step through rows
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uSrcHeight) * 50.0)))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
                
        ScaleRow (  pSrc,
                    uSrcWidth,
                    uSrcWidthBytes,
                    pDst,
                    uResWidth,
                    uResWidthBytes,
                    u,
                    Contrib);    // Scale each row
    }
    FreeContributions (Contrib);  // Free contributions structure
}

template <class FilterClass>
void
C2PassScale< FilterClass>::
ScaleCol (  UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UCHAR           *pRes,
            UINT                uResWidth,
            UINT                 uResWidthBytes,
            UINT                uResHeight,
            UINT                uCol,
            LineContribType    *Contrib)
{
    UCHAR *pSrcLoc;
    UCHAR *pDstLoc;
    unsigned int vals[PS_MAX_DEPTH];

    // assumes same height
    for (UINT y = 0; y < uResHeight; y++)
    {    // Loop through column
        int v, i;
        for (v= 0; v < m_byteDepth; v++)
            vals[v] = 0;

        int iLeft = Contrib->ContribRow[y].Left;    // Retrieve left boundries
        int iRight = Contrib->ContribRow[y].Right;  // Retrieve right boundries
        pSrcLoc = pSrc + uSrcWidthBytes*iLeft + uCol* m_byteDepth;
        for (i = iLeft; i <= iRight; i++)
        {   // Scan between boundries
            // Accumulate weighted effect of each neighboring pixel
            UCHAR *pCurSrc = pSrc + uSrcWidthBytes*i + uCol* m_byteDepth;
            #ifdef _DEBUG
                ASSERT(i-iLeft < (int)Contrib->WindowSize);
            #endif
            for (v= 0; v < m_byteDepth; v++)
                vals[v] += Contrib->ContribRow[y].Weights[i-iLeft] * pSrcLoc[v];
            pSrcLoc += uSrcWidthBytes;
        }
        pDstLoc = pRes + (y * uResWidthBytes) + uCol*m_byteDepth;
        for (v= 0; v < m_byteDepth; v++)
        {
            // scale back
            *pDstLoc++ = BOUND( vals[v] >> 16, 0, 0xff);   // Place result in destination pixel
        }
    }
}


template <class FilterClass>
void
C2PassScale< FilterClass>::
VertScale ( UCHAR           *pSrc,
            UINT                uSrcWidth,
            UINT                 uSrcWidthBytes,
            UINT                uSrcHeight,
            UCHAR           *pDst,
            UINT                uResHeight)
{
//    TRACE ("Performing vertical scaling...");

    // assumes widths are the same!
    if (uSrcHeight == uResHeight)
    {   // No scaling required, just copy
        if (uSrcHeight <= 0) return;
        int copy = ((uSrcHeight -1) * uSrcWidthBytes) + uSrcWidth*m_byteDepth; // avoids overrun if starting in middle of image.
        memcpy (pDst, pSrc, copy);
        return;
    }
    // Allocate and calculate the contributions
    LineContribType * Contrib = CalcContributions (uResHeight, uSrcHeight, double(uResHeight) / double(uSrcHeight));
    for (UINT u = 0; u < uSrcWidth; u++)
    {   // Step through columns
        if (NULL != m_Callback)
        {
            //
            // Progress and report callback supplied
            //
            if (!m_Callback (BYTE(double(u) / double (uSrcWidth) * 50.0) + 50))
            {
                //
                // User wished to abort now
                //
                m_bCanceled = TRUE;
                FreeContributions (Contrib);  // Free contributions structure
                return;
            }
        }
        ScaleCol (  pSrc,
                    uSrcWidth,
                    uSrcWidthBytes,
                    pDst,
                    uSrcWidth,
                    uSrcWidthBytes,
                    uResHeight,
                    u,
                    Contrib);   // Scale each column
    }
    FreeContributions (Contrib);     // Free contributions structure
}

template <class FilterClass>
UCHAR *
C2PassScale< FilterClass>::
AllocAndScale (
    UCHAR   *pOrigImage,			// A pointer to the source bitmap bits
    UINT         pixelBytes,		// The size of a single pixel in bytes (both source and scaled image)
    UINT        uOrigWidth,			// The width of a line of the source image to scale in pixels
    UINT        uOrigWidthBytes,	// The width of a single line of the source image in bytes (to allow for padding, etc.)
    UINT        uOrigHeight,		// The height of the source image to scale in pixels.
    UINT        uNewWidth,			// The desired width of a line of the scaled image in pixels
    UINT        uNewWidthBytes,		// The width of a single line of the scaled image in bytes (to allow for padding, etc.)
    UINT        uNewHeight)			// The desired height of the scaled image in pixels.
{
    // Scale source image horizontally into temporary image
    m_byteDepth = pixelBytes;
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [uNewWidthBytes * uOrigHeight];
    HorizScale (pOrigImage,
                uOrigWidth,
                uOrigWidthBytes,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uNewWidthBytes);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }
    // Scale temporary image vertically into result image    
    UCHAR *pRes = new UCHAR [uNewWidth * uNewHeight *m_byteDepth];
    VertScale ( pTemp,
                uNewWidth,
                uNewWidthBytes,
                uOrigHeight,
                pRes,
                uNewHeight);
    if (m_bCanceled)
    {
        delete [] pTemp;
        delete [] pRes;
        return NULL;
    }
    delete [] pTemp;
    return pRes;
}

template <class FilterClass>
UCHAR *
C2PassScale< FilterClass>::
Scale (
    UCHAR   *pOrigImage,			// A pointer to the source bitmap bits
    UINT         pixelBytes,		// The size of a single pixel in bytes (both source and scaled image)
    UINT        uOrigWidth,			// The width of a line of the source image to scale in pixels
    UINT        uOrigWidthBytes,	// The width of a single line of the source image in bytes (to allow for padding, etc.)
    UINT        uOrigHeight,		// The height of the source image to scale in pixels.
    UCHAR   *pDstImage,				// A pointer to a buffer to hold the ecaled image
    UINT        uNewWidth,			// The desired width of a line of the scaled image in pixels
    UINT        uNewWidthBytes,		// The width of a single line of the scaled image in bytes (to allow for padding, etc.)
    UINT        uNewHeight)			// The desired height of the scaled image in pixels.
{
    // Scale source image horizontally into temporary image
    ASSERT(PS_MAX_DEPTH >= pixelBytes);
    m_byteDepth = pixelBytes;
    m_bCanceled = FALSE;
    UCHAR *pTemp = new UCHAR [ uOrigHeight *uNewWidthBytes];
    HorizScale (pOrigImage,
                uOrigWidth,
                uOrigWidthBytes,
                uOrigHeight,
                pTemp,
                uNewWidth,
                uNewWidthBytes);
    if (m_bCanceled)
    {
        delete [] pTemp;
        return NULL;
    }

    // Scale temporary image vertically into result image    
    VertScale ( pTemp,
                uNewWidth,
                uNewWidthBytes,
                uOrigHeight,
                pDstImage,
                uNewHeight);
    delete [] pTemp;
    if (m_bCanceled)
    {
        return NULL;
    }
    return pDstImage;
}

void CreateWorkingBitmap (WORD dx,WORD dy,tWorkBMP *w);
void ShrinkWorkingBitmap (tWorkBMP *a,tWorkBMP *b,WORD bx,WORD by);
HBITMAP CreateEmptyBitmap (WORD dx,WORD dy);
void SetBMIHeader (BITMAPINFO *b,short dx,short dy);
void SaveWorkingBitmap (tWorkBMP *w,HBITMAP b);





#endif