// H263Dec.cpp: implementation of the CH263Dec class.
//

#include <windows.h>
#include "H263Dec.h"

static int XDIM = 0;
static int YDIM = 0;
static int ARG_SAVEDECOUTPUT = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static char *ARG_INPUTFILE = NULL;
static int CSP = XVID_CSP_I420;
static int BPP = 1;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
unsigned char CH263Dec::rgbdata[1280*1024*3];
CH263Dec::CH263Dec()
{
	
}

CH263Dec::~CH263Dec()
{
//	Destroy();
}

void CH263Dec::Init(unsigned int uiWidth,unsigned int uiHeight)
{
	buffersize=1280*1024*3;


	XDIM=uiWidth;
	YDIM=uiHeight;

	CSP = XVID_CSP_BGR;
	BPP = 3;

	dec_init(true,0);

}

int CH263Dec::Decompress(unsigned char *pData, int size)
{
	xvid_dec_stats_t xvid_dec_stats;

	int retvalue=0;
	memset(rgbdata,0x00,buffersize);

	do {
		retvalue = dec_main((unsigned char*)pData+retvalue,rgbdata,size,&xvid_dec_stats);
		
		if(xvid_dec_stats.type==-1)
		{
			XDIM=xvid_dec_stats.data.vol.width;
			YDIM=xvid_dec_stats.data.vol.height;
		}

	}while (xvid_dec_stats.type < 0  );

	return retvalue;
}

void CH263Dec::DrawImage(HWND hWnd,BITMAPINFO * pInfo)
{
	RECT Rc;
	HDC hdc;
	hdc = ::GetDC(hWnd);
	::SetStretchBltMode(hdc, HALFTONE);
	unsigned int TotalSize=pInfo->bmiHeader.biWidth*pInfo->bmiHeader.biHeight*3;
	if(buffersize<TotalSize)
	{
		buffersize=TotalSize;
	}

	::GetWindowRect(hWnd,&Rc);
	int re=0;
	re = ::StretchDIBits(hdc, 0, 0, Rc.right-Rc.left, Rc.bottom-Rc.top, 0, 0, pInfo->bmiHeader.biWidth,pInfo->bmiHeader.biHeight,rgbdata, 
		(BITMAPINFO*)pInfo, DIB_RGB_COLORS, SRCCOPY);


	::ReleaseDC(hWnd, hdc);
}

void CH263Dec::Destroy()
{
}

int CH263Dec::dec_init(int use_assembler, int debug_level)
{
	dec_handle = NULL;

	int ret;

	xvid_gbl_init_t   xvid_gbl_init;
	xvid_dec_create_t xvid_dec_create;

	/* Reset the structure with zeros */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init_t));
	memset(&xvid_dec_create, 0, sizeof(xvid_dec_create_t));

	/*------------------------------------------------------------------------
	 * XviD core initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_gbl_init.version = XVID_VERSION;

	/* Assembly setting */
	if(use_assembler)
#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_IA64;
#else
	xvid_gbl_init.cpu_flags = 0;
#endif
	else
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;

	xvid_gbl_init.debug = debug_level;

	xvid_global(NULL, 0, &xvid_gbl_init, NULL);

	/*------------------------------------------------------------------------
	 * XviD encoder initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_dec_create.version = XVID_VERSION;

	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
	xvid_dec_create.width = 0;
	xvid_dec_create.height = 0;

	ret = xvid_decore(NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL);

	dec_handle = xvid_dec_create.handle;

	return(ret);
}

int CH263Dec::dec_main(unsigned char *istream,
		 unsigned char *ostream,
		 int istream_size,
		 xvid_dec_stats_t *xvid_dec_stats)
{

	int ret;

	xvid_dec_frame_t xvid_dec_frame;

	/* Reset all structures */
	memset(&xvid_dec_frame, 0, sizeof(xvid_dec_frame_t));
	memset(xvid_dec_stats, 0, sizeof(xvid_dec_stats_t));

	/* Set version */
	xvid_dec_frame.version = XVID_VERSION;
	xvid_dec_stats->version = XVID_VERSION;

	/* No general flags to set */
	xvid_dec_frame.general          = 0;

	/* Input stream */
	xvid_dec_frame.bitstream        = istream;
	xvid_dec_frame.length           = istream_size;

	/* Output frame structure */
	xvid_dec_frame.output.plane[0]  = ostream;
	xvid_dec_frame.output.stride[0] = XDIM*BPP;
	xvid_dec_frame.output.csp = CSP;

	ret = xvid_decore(dec_handle, XVID_DEC_DECODE, &xvid_dec_frame, xvid_dec_stats);

	return(ret);
}
