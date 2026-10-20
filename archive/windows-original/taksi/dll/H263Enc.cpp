// H263Enc.cpp: implementation of the CH263Enc class.
//
//////////////////////////////////////////////////////////////////////
#include "../stdafx.h"
#include "H263Enc.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define CONFIG_KBPS 1000

/*****************************************************************************
 *                            Quality presets
 ****************************************************************************/

HANDLE hFile;
static const int motion_presets[] = {
	0, 0, 0, 0,
	0 | XVID_ME_HALFPELREFINE16 | 0,
	0 | XVID_ME_HALFPELREFINE16 | 0 |
	XVID_ME_ADVANCEDDIAMOND16, XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |
	XVID_ME_HALFPELREFINE8 | 0 | XVID_ME_USESQUARES16
};
#define ME_ELEMENTS (sizeof(motion_presets)/sizeof(motion_presets[0]))

static const int vop_presets[] = {
	/* quality 0 */
	0,

	/* quality 1 */
	0,

	/* quality 2 */
	XVID_VOP_HALFPEL,

	/* quality 3 */
	XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

	/* quality 4 */
	XVID_VOP_HALFPEL | XVID_VOP_INTER4V,

	/* quality 5 */
	XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
	XVID_VOP_TRELLISQUANT,

	/* quality 6 */
	XVID_VOP_HALFPEL | XVID_VOP_INTER4V |
	XVID_VOP_TRELLISQUANT | XVID_VOP_HQACPRED,

};
#define VOP_ELEMENTS (sizeof(vop_presets)/sizeof(vop_presets[0]))

#define FRAMERATE_INCR 1001

static unsigned char qmatrix_intra[64];
static unsigned char qmatrix_inter[64];

static const BYTE default_qmatrix_intra[] = {
	8, 17,18,19,21,23,25,27,
	17,18,19,21,23,25,27,28,
	20,21,22,23,24,26,28,30,
	21,22,23,24,26,28,30,32,
	22,23,24,26,28,30,32,35,
	23,24,26,28,30,32,35,38,
	25,26,28,30,32,35,38,41,
	27,28,30,32,35,38,41,45
};

static const BYTE default_qmatrix_inter[] = {
	16,17,18,19,20,21,22,23,
	17,18,19,20,21,22,23,24,
	18,19,20,21,22,23,24,25,
	19,20,21,22,23,24,26,27,
	20,21,22,23,25,26,27,28,
	21,22,23,24,26,27,28,30,
	22,23,24,26,27,28,30,31,
	23,24,25,27,28,30,31,33
};

/*****************************************************************************
 *                     Command line global variables
 ****************************************************************************/

#define MAX_ZONES   64
#define ARCH_IS_IA64	

static xvid_enc_zone_t ZONES[MAX_ZONES];
static int NUM_ZONES = 0;
/* Maximum number of frames to encode */
#define ABS_MAXFRAMENR 9999

static int ARG_STATS = 0;
static int ARG_DUMP = 0;
static int ARG_LUMIMASKING = 0;
static int ARG_BITRATE = 30000*CONFIG_KBPS;
static int ARG_SINGLE = 1;
static char *ARG_PASS1 = 0;//"video.pass";
static char *ARG_PASS2 = 0;//;"video.pass";
static int ARG_QUALITY =0;// ME_ELEMENTS - 1;
static float ARG_FRAMERATE = 20.00f;
static int ARG_MAXFRAMENR = ABS_MAXFRAMENR;
static int ARG_MAXKEYINTERVAL = 0;
static char *ARG_INPUTFILE = NULL;
static int ARG_INPUTTYPE = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static int ARG_SAVEINDIVIDUAL = 0;
static char *ARG_OUTPUTFILE = NULL;
static int XDIM = 0;
static int YDIM = 0;
static int ARG_BQRATIO = 300;
static int ARG_BQOFFSET = 100;
static int ARG_MAXBFRAMES = 0;
static int ARG_PACKED = 1;
static int ARG_DEBUG = 0;
static int ARG_VOPDEBUG = 0;
static int ARG_GREYSCALE = 0;
static int ARG_QTYPE = 1;
static int ARG_QMATRIX = 1;
static int ARG_GMC = 0;
static int ARG_INTERLACING = 0;
static int ARG_QPEL = 0;
static int ARG_TURBO = 1;
static int ARG_VHQMODE = 0;
static int ARG_BVHQ = 0;
static int ARG_CLOSED_GOP = 0;

#pragma comment(lib,"xvid/libxvidcore.lib")
#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#define SMALL_EPS (1e-10)

#define SWAP(a) ( (((a)&0x000000ff)<<24) | (((a)&0x0000ff00)<<8) | \
                  (((a)&0x00ff0000)>>8)  | (((a)&0xff000000)>>24) )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

int CH263Enc::count;
int CH263Enc::iBufferSize;
unsigned char CH263Enc::cdata[1600*1200*4];

void CH263Enc::OwnWriteFunction(int byte)
{
	if(count<iBufferSize)
	{
		cdata[count]=(unsigned char)byte;
		count++;
	} else {
		int a=0;
	}
}

CH263Enc::CH263Enc()
{
	count=0;
	memset(cdata,0x00,sizeof(cdata));
	iBufferSize=sizeof(cdata);
	key=0;
	enc_handle = NULL;
}

CH263Enc::~CH263Enc()
{
//	Destroy();
}

void CH263Enc::SetBItrate(int iBit)
{
	ARG_BITRATE = iBit * CONFIG_KBPS;
}

void CH263Enc::Init(unsigned int uiWidth,unsigned int uiHeight,int keyframe,char * PASS1,char * PASS2)
{
	m_uiWidth=uiWidth;
	m_uiHeight=uiHeight;

	//InitH263Encoder(&m_Param);
	//WriteByteFunction = OwnWriteFunction;

	this->enc_init(true,keyframe, PASS1,PASS2);
	
	/*
	out_file = NULL;
	if ((out_file = fopen("Out.avi", "w+b")) == NULL) {
		fprintf(stderr, "Error opening output file %s\n", ARG_OUTPUTFILE);
	}
	*/
}

void CH263Enc::Compress(void * pData,bool bInter)
{

	int sse[3];

	memset(cdata,0x00,sizeof(cdata));

	count = enc_main((unsigned char *)pData, cdata, &key, &stats_type,
			   &stats_quant, &stats_length, sse);

}

void CH263Enc::Destroy()
{
	//ExitH263Encoder(&m_Param);
	enc_stop();
}

unsigned char * CH263Enc::GetData()
{
	return &cdata[0];
}

unsigned int CH263Enc::GetCount()
{
	return count;
}

void CH263Enc::DrawImage(HWND hWnd,HWND hTargetWnd, void * rgbdata,BITMAPINFO * pInfo)
{
	RECT Rc;
	HDC hdc;
	
	RECT m_WndRect;
	::GetWindowRect(hWnd,&m_WndRect);


	/*
	POINT xPoint; 
	::GetCursorPos( &xPoint ); 
	xPoint.x -= m_WndRect.left;
	xPoint.y -= m_WndRect.top;

	HCURSOR hCursor = ::GetCursor();
	ICONINFO iconinfo;	
	BOOL bRet = ::GetIconInfo( hCursor,  &iconinfo );
	if (bRet) 
	{
		xPoint.x -= iconinfo.xHotspot;
		xPoint.y -= iconinfo.yHotspot;

		//need to delete the hbmMask and hbmColor bitmaps
		//otherwise the program will crash after a while after running out of resource
		if (iconinfo.hbmMask) 
			::DeleteObject(iconinfo.hbmMask);
		if (iconinfo.hbmColor) 
			::DeleteObject(iconinfo.hbmColor);
	}		
	*/	
	


	hdc = ::GetDC(hWnd);
	::SetStretchBltMode(hdc, HALFTONE);
	
	::GetWindowRect(hWnd,&Rc);
	int re=0;
	re = ::StretchDIBits(hdc, 0, 0, Rc.right-Rc.left, Rc.bottom-Rc.top, 0, 0, pInfo->bmiHeader.biWidth,pInfo->bmiHeader.biHeight,rgbdata, 
		(BITMAPINFO*)pInfo, DIB_RGB_COLORS, SRCCOPY);

		//::DrawIcon(hdc, xPoint.x, xPoint.y, hCursor);
	//::DrawIcon(hdc, 0, 0, hCursor);
	::ReleaseDC(hWnd, hdc);
}




int CH263Enc::enc_init(int use_assembler,int keyframe,char * PASS1,char * PASS2)
{
	int xerr;

	if((!PASS1)&&(!PASS2))
	{
		ARG_SINGLE =1;
		ARG_PASS1 = NULL;
		ARG_PASS2 = NULL;
	} else {
		ARG_SINGLE = 0;
		ARG_PASS1 = PASS1;
		ARG_PASS2 = PASS2;
	}
	ARG_SINGLE =1;
	ARG_PASS1 = NULL;
	ARG_PASS2 = NULL;
		
	//xvid_plugin_cbr_t cbr;
    xvid_plugin_single_t single;
	xvid_plugin_2pass1_t rc2pass1;
	xvid_plugin_2pass2_t rc2pass2;
	//xvid_plugin_fixed_t rcfixed;
	xvid_enc_plugin_t plugins[7];
	xvid_gbl_init_t xvid_gbl_init;
	xvid_enc_create_t xvid_enc_create;

	/*------------------------------------------------------------------------
	 * XviD core initialization
	 *----------------------------------------------------------------------*/

	/* Set version -- version checking will done by xvidcore */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
	xvid_gbl_init.version = XVID_VERSION;
    xvid_gbl_init.debug = ARG_DEBUG;


	/* Do we have to enable ASM optimizations ? */
	if (use_assembler) {

#ifdef ARCH_IS_IA64


//#define XVID_CPU_MMX      (1<< 0) /*       mmx : pentiumMMX,k6 */
//#define XVID_CPU_MMXEXT   (1<< 1) /*   mmx-ext : pentium2, athlon */
//#define XVID_CPU_SSE      (1<< 2) /*       sse : pentium3, athlonXP */
//#define XVID_CPU_SSE2     (1<< 3) /*      sse2 : pentium4, athlon64 */
//#define XVID_CPU_3DNOW    (1<< 4) /*     3dnow : k6-2 */
//#define XVID_CPU_3DNOWEXT (1<< 5) /* 3dnow-ext : athlon */
//#define XVID_CPU_TSC      (1<< 6) /*       tsc : Pentium */

		xvid_gbl_init.cpu_flags =  XVID_CPU_FORCE | XVID_CPU_ASM|XVID_CPU_MMXEXT|XVID_CPU_MMX|XVID_CPU_SSE|XVID_CPU_SSE2 ;
#else
		xvid_gbl_init.cpu_flags = 0;
#endif
	} else {
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;
	}

	/* Initialize XviD core -- Should be done once per __process__ */
	xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);

	/*------------------------------------------------------------------------
	 * XviD encoder initialization
	 *----------------------------------------------------------------------*/

	/* Version again */
	memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
	xvid_enc_create.version = XVID_VERSION;

	/* Width and Height of input frames */
	xvid_enc_create.width = m_uiWidth;
	xvid_enc_create.height = m_uiHeight;
	xvid_enc_create.profile = XVID_PROFILE_AS_L4;

	/* init plugins  */
    xvid_enc_create.zones = ZONES;
    xvid_enc_create.num_zones = NUM_ZONES;

	xvid_enc_create.plugins = plugins;
	xvid_enc_create.num_plugins = 0;

	if (ARG_SINGLE) {
		memset(&single, 0, sizeof(xvid_plugin_single_t));
		single.version = XVID_VERSION;
		single.bitrate = ARG_BITRATE;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
		plugins[xvid_enc_create.num_plugins].param = &single;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_PASS2) {
		memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
		rc2pass2.version = XVID_VERSION;
		rc2pass2.filename = ARG_PASS2;
		rc2pass2.bitrate = ARG_BITRATE;

		hFile = CreateFile(rc2pass2.filename, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);	

		rc2pass2.keyframe_boost = 20;   /* keyframe boost percentage: [0..100...]; */
		rc2pass2.curve_compression_high = 25;
		rc2pass2.curve_compression_low = 10;
		rc2pass2.overflow_control_strength = 240;
		rc2pass2.max_overflow_improvement = 60;
		rc2pass2.max_overflow_degradation = 60;
		rc2pass2.kfreduction = 30;
		rc2pass2.kfthreshold = 10;
		rc2pass2.container_frame_overhead = 24;	/* AVI */

		/* VBV */
		rc2pass2.vbv_size = 1000000;
		rc2pass2.vbv_initial = (rc2pass2.vbv_size*3)/4; /* 75% */
		rc2pass2.vbv_maxrate = 5642000;
		rc2pass2.vbv_peakrate = rc2pass2.vbv_initial*3;

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
		xvid_enc_create.num_plugins++;
		}

	if (ARG_PASS1) {
		memset(&rc2pass1, 0, sizeof(xvid_plugin_2pass1_t));
		rc2pass1.version = XVID_VERSION;
		rc2pass1.filename = ARG_PASS1;
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_LUMIMASKING) {
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_lumimasking;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_DUMP) {
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_dump;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

#if 0
	if (ARG_DEBUG) {
		plugins[xvid_enc_create.num_plugins].func = rawenc_debug;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}
#endif

	/* No fancy thread tests */
	xvid_enc_create.num_threads = 0;

	/* Frame rate - Do some quick float fps = fincr/fbase hack */
	if ((ARG_FRAMERATE - (int) ARG_FRAMERATE) < SMALL_EPS) {
		xvid_enc_create.fincr = 1;
		xvid_enc_create.fbase = (int) ARG_FRAMERATE;
	} else {
		xvid_enc_create.fincr = FRAMERATE_INCR;
		xvid_enc_create.fbase = (int) (FRAMERATE_INCR * ARG_FRAMERATE);
	}
	ARG_MAXKEYINTERVAL = keyframe;

	/* Maximum key frame interval */
    if (ARG_MAXKEYINTERVAL > 0) {
        xvid_enc_create.max_key_interval = ARG_MAXKEYINTERVAL;
    }else {
	    xvid_enc_create.max_key_interval = (int) ARG_FRAMERATE *10;
    }

	/* Bframes settings */
	xvid_enc_create.max_bframes = ARG_MAXBFRAMES;
	xvid_enc_create.bquant_ratio = ARG_BQRATIO;
	xvid_enc_create.bquant_offset = ARG_BQOFFSET;

	/* Dropping ratio frame -- we don't need that */
	xvid_enc_create.frame_drop_ratio = 0;

	/* Global encoder options */
	xvid_enc_create.global = 0;

	if (ARG_PACKED)
		xvid_enc_create.global |= XVID_GLOBAL_PACKED;

	if (ARG_CLOSED_GOP)
		xvid_enc_create.global |= XVID_GLOBAL_CLOSED_GOP;

	if (ARG_STATS)
		xvid_enc_create.global |= XVID_GLOBAL_EXTRASTATS_ENABLE;

	memcpy(qmatrix_intra,default_qmatrix_intra,64);
	memcpy(qmatrix_inter,default_qmatrix_inter,64);

	xvid_enc_create.min_quant[0] = 1;
	xvid_enc_create.max_quant[0] = 31;
	xvid_enc_create.min_quant[1] = 1;
	xvid_enc_create.max_quant[1] = 31;
	xvid_enc_create.min_quant[2] = 1;
	xvid_enc_create.max_quant[2] = 31;


	/* I use a small value here, since will not encode whole movies, but short clips */
	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);

	/* Retrieve the encoder instance from the structure */
	enc_handle = xvid_enc_create.handle;

	return (xerr);
}
int CH263Enc::enc_stop()
{
	int xerr;

	/* Destroy the encoder instance */
	xerr = xvid_encore(enc_handle, XVID_ENC_DESTROY, NULL, NULL);

	CloseHandle(hFile);

	return (xerr);
}

int CH263Enc::enc_main(unsigned char *image,
					unsigned char *bitstream,
					int *key,
					int *stats_type,
					int *stats_quant,
					int *stats_length,
					int sse[3])
{
	int ret;


/**/
/**/

	xvid_enc_frame_t xvid_enc_frame;
	xvid_enc_stats_t xvid_enc_stats;

	/* Version for the frame and the stats */
	memset(&xvid_enc_frame, 0, sizeof(xvid_enc_frame));
	xvid_enc_frame.version = XVID_VERSION;

	memset(&xvid_enc_stats, 0, sizeof(xvid_enc_stats));
	xvid_enc_stats.version = XVID_VERSION;

	/* Bind output buffer */
	xvid_enc_frame.bitstream = bitstream;
	xvid_enc_frame.length = -1;

	/* Initialize input image fields */
	if (image) {
		xvid_enc_frame.input.plane[0] = image;
//#ifndef READ_PNM
//		xvid_enc_frame.input.csp = XVID_CSP_I420;
//		xvid_enc_frame.input.stride[0] = m_uiWidth;
//#else
		xvid_enc_frame.input.csp = XVID_CSP_BGRA;
		xvid_enc_frame.input.stride[0] = m_uiWidth*4;

		//xvid_enc_frame.input.csp = XVID_CSP_BGR;
		//xvid_enc_frame.input.stride[0] = m_uiWidth*3;
		
//#endif
	} else {
		xvid_enc_frame.input.csp = XVID_CSP_NULL;
	}

	/* Set up core's general features */
	xvid_enc_frame.vol_flags = 0;
	if (ARG_STATS)
		xvid_enc_frame.vol_flags |= XVID_VOL_EXTRASTATS;
	if (ARG_QTYPE)
		xvid_enc_frame.vol_flags |= XVID_VOL_MPEGQUANT;
	if (ARG_QPEL)
		xvid_enc_frame.vol_flags |= XVID_VOL_QUARTERPEL;
	if (ARG_GMC)
		xvid_enc_frame.vol_flags |= XVID_VOL_GMC;
	if (ARG_INTERLACING)
		xvid_enc_frame.vol_flags |= XVID_VOL_INTERLACING;

	/* Set up core's general features */
	xvid_enc_frame.vop_flags = vop_presets[ARG_QUALITY];

    if (ARG_VOPDEBUG) {
        xvid_enc_frame.vop_flags |= XVID_VOP_DEBUG;
    }

    if (ARG_GREYSCALE) {
        xvid_enc_frame.vop_flags |= XVID_VOP_GREYSCALE;
    }

	/* Frame type -- let core decide for us */
	xvid_enc_frame.type = XVID_TYPE_AUTO;

	/* Force the right quantizer -- It is internally managed by RC plugins */
	xvid_enc_frame.quant = 0;

	/* Set up motion estimation flags */
		xvid_enc_frame.motion = motion_presets[ARG_QUALITY];

	if (ARG_GMC)
		xvid_enc_frame.motion |= XVID_ME_GME_REFINE;

	if (ARG_QPEL)
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16;
	if (ARG_QPEL && (xvid_enc_frame.vop_flags & XVID_VOP_INTER4V))
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8;

	if (ARG_TURBO)
		xvid_enc_frame.motion |= XVID_ME_FASTREFINE16 | XVID_ME_FASTREFINE8 | 
								 XVID_ME_SKIP_DELTASEARCH | XVID_ME_FAST_MODEINTERPOLATE | 
								 XVID_ME_BFRAME_EARLYSTOP;

	if (ARG_BVHQ) 
		xvid_enc_frame.vop_flags |= XVID_VOP_RD_BVOP;

	switch (ARG_VHQMODE) /* this is the same code as for vfw */
	{
	case 1: /* VHQ_MODE_DECISION */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		break;

	case 2: /* VHQ_LIMITED_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		break;

	case 3: /* VHQ_MEDIUM_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_CHECKPREDICTION_RD;
		break;

	case 4: /* VHQ_WIDE_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_CHECKPREDICTION_RD;
		xvid_enc_frame.motion |= XVID_ME_EXTSEARCH_RD;
		break;

	default :
		break;
	}
    
	if (ARG_QMATRIX) {
		/* We don't use special matrices */
		xvid_enc_frame.quant_intra_matrix = qmatrix_intra;
		xvid_enc_frame.quant_inter_matrix = qmatrix_inter;
	}
	else {
		/* We don't use special matrices */
		xvid_enc_frame.quant_intra_matrix = NULL;
		xvid_enc_frame.quant_inter_matrix = NULL;
	}

	/* Encode the frame */
	ret = xvid_encore(enc_handle, XVID_ENC_ENCODE, &xvid_enc_frame,
					  &xvid_enc_stats);

	*key = (xvid_enc_frame.out_flags & XVID_KEYFRAME);
	*stats_type = xvid_enc_stats.type;
	*stats_quant = xvid_enc_stats.quant;
	*stats_length = xvid_enc_stats.length;
	sse[0] = xvid_enc_stats.sse_y;
	sse[1] = xvid_enc_stats.sse_u;
	sse[2] = xvid_enc_stats.sse_v;

	return (ret);
}