// ScreenEncoderDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Mmsystem.h>
#include "./voip/ThreadWork.h"

#include "virtualdub/h/vd2/system/cpuaccel.h"
#include "virtualdub/h/vd2/Kasumi/pixmap.h"
#include "virtualdub/h/vd2/Kasumi/pixmaputils.h"
#include "virtualdub/h/vd2/Kasumi/resample.h"

#include "ScreenEncoder.h"
#include "ScreenEncoderDlg.h"
#include ".\screenencoderdlg.h"
#include "Avi_Lib/avilib.h"
#include "ijl.h"

#pragma comment(lib,"ijl15l.lib")


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
//#pragma comment(lib,"skinmagic.lib")
#pragma comment(lib,"SkinMagicLibMT7.lib")
#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,".\\lib\\Kasumi.lib")
#pragma comment(lib,".\\lib\\system.lib")

static unsigned long time_old;
/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderDlg dialog

HINSTANCE g_hInst = NULL;
const TCHAR g_szAppTitle[] = _T("ScreenEncoder");
CTaksiConfig g_Config;

unsigned long iTotalFrameCount;
unsigned long iTotalFrameErrorCount;
CString strTempDir;

WorkThread m_WorkThread;

HWND hTargetWnd;

char * ARG_PASS =  "video_pass.txt"; 

IVDPixmapResampler *resampler;

enum _tagCODEC
{
	ID_XVID = 0,
	ID_JEPG,
	ID_BMP_TEMP
};

char CodecName[][10]=
{
	{"DIVX"},
	{"JPEG"},
	{"DIVX"},
};

CScreenEncoderDlg * CScreenEncoderDlg::pMainClass;


BYTE *m_lpBuffer;
JPEG_CORE_PROPERTIES m_Jpeg,m_DJpeg;	// 압축용



HBITMAP CreateEmptyBitmap (WORD dx,WORD dy)
{
	HDC h=GetDC(NULL);
	HBITMAP b=CreateCompatibleBitmap(h,dx,dy);
	ReleaseDC (NULL,h);

	return (b);
}

void SetBMIHeader (BITMAPINFO *b,short dx,short dy)
{
	b->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
	b->bmiHeader.biWidth=dx;
	b->bmiHeader.biHeight=-dy;
	b->bmiHeader.biPlanes=1;
	b->bmiHeader.biBitCount=24;
	b->bmiHeader.biCompression=BI_RGB;
	b->bmiHeader.biSizeImage=0;
	b->bmiHeader.biXPelsPerMeter=1;
	b->bmiHeader.biYPelsPerMeter=1;
	b->bmiHeader.biClrUsed=0;
	b->bmiHeader.biClrImportant=0;
}

void SaveWorkingBitmap (tWorkBMP *w,HBITMAP b)
{
	BITMAPINFO s;
	HDC h=GetDC(NULL);
	SetBMIHeader (&s,w->x,w->y);
	SetDIBits (h,b,0,w->y,w->b,&s,DIB_RGB_COLORS);
	ReleaseDC (NULL,h);
}

void CreateWorkingBitmap (WORD dx,WORD dy,int iDepth,tWorkBMP *w)
{
	w->x=dx;
	w->y=dy;
	w->l=(dx)*iDepth;//&0xfffc;
	w->iDepth = iDepth;
	w->b=Alloc(w->l*dy,BYTE);
}

void ShrinkWorkingBitmap (tWorkBMP *a,tWorkBMP *b,WORD bx,WORD by)
{
	BYTE *uy=a->b,*ux,i;
	WORD x,y,nx,ny=0;
	int iDepth = a->iDepth ;
	DWORD df=a->iDepth*bx,nf=df*by,j;
	float k,qx[2],qy[2],q[4],*f=Alloc(nf,float);

	CreateWorkingBitmap (bx,by,a->iDepth,b);

	jFor (nf) f[j]=0;
	j=0;

	For (y,a->y) {
	ux=uy;
	uy+=a->l;
	nx=0;
	ny+=by;

	if (ny>a->y) {
		qy[0]=1-(qy[1]=(ny-a->y)/(float)by);
		For (x,a->x) {
		    nx+=bx;
		    if (nx>a->x) {
				qx[0]=1-(qx[1]=(nx-a->x)/(float)bx);
			     iFor (4) q[i]=qx[i&1]*qy[i>>1];
			     iFor (iDepth) {
					f[j]+=(*ux)*q[0];
					f[j+iDepth]+=(*ux)*q[1];
					f[j+df]+=(*ux)*q[2];
					f[(j++)+df+iDepth]+=(*(ux++))*q[3];
				}
			}
			else iFor (iDepth) {
				f[j+i]+=(*ux)*qy[0];
				f[j+df+i]+=(*(ux++))*qy[1];
			}
			if (nx>=a->x) nx-=a->x;
			if (!nx) j+=iDepth;
		}
	}
	else {
		For (x,a->x) {
				nx+=bx;
				if (nx>a->x) {
					qx[0]=1-(qx[1]=(nx-a->x)/(float)bx);
					iFor (iDepth) {
						f[j]+=(*ux)*qx[0];
						f[(j++)+iDepth]+=(*(ux++))*qx[1];
					}
				}
				else{
					iFor (iDepth) f[j+i]+=*(ux++);
				}
				
				if (nx>=a->x) nx-=a->x;
				if (!nx) j+=iDepth;
			}
			if (ny<a->y) j-=df;
		}
		if (ny>=a->y) ny-=a->y;
	}

	nf=0;
	k=bx*by/(float)(a->x*a->y);
	uy=b->b;

	For (y,by) {
		jFor (df) uy[j]=(BYTE)f[nf++]*k+.5;
		uy+=b->l;
	}
	free (f);
}


void WINAPI CScreenEncoderDlg::DrawImageData1(void * Data,unsigned long DataLen)
{

	int a=0;
}

bool DeleteFolder(const CString &strFolder)
{
    SHFILEOPSTRUCT FileOp = {0};
    char szTemp[MAX_PATH];

    strcpy_s(szTemp, strFolder);
    szTemp[strFolder.GetLength() + 1] = NULL;    // NULL문자가 두개 들어가야 한다.

    FileOp.hwnd = NULL;
    FileOp.wFunc = FO_DELETE;
    FileOp.pFrom = NULL;
    FileOp.pTo = NULL;
    FileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI;  // 확인메시지가 안뜨도록 설정
    FileOp.fAnyOperationsAborted = false;
    FileOp.hNameMappings = NULL;
    FileOp.lpszProgressTitle = NULL;

    FileOp.pFrom = szTemp;
    SHFileOperation(&FileOp);
    return true;
}


UINT CScreenEncoderDlg::EncodingThreadProc1(LPVOID lParam)
{

	return 0;
}
UINT CScreenEncoderDlg::EncodingThreadProc(LPVOID lParam)
{
	char szMP3FileName[256];
	double FrameRate;
	BYTE  * Buffer;//[1600*1200*4];
	iTotalFrameErrorCount  = 0;
	int iMovieCount=0;

	Buffer =  new BYTE [GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4];

	CScreenEncoderDlg * pParentClass;
	pParentClass=(CScreenEncoderDlg *)lParam;
	register unsigned long time_old,time_start;
	long time_frame = 1000/pParentClass->m_SCConfig.iFrameRate; 
	FrameRate = 1000.0/((double)time_frame) ;

	pParentClass->bFrameLocked = true;
	pParentClass->pFrameBuffer = new BYTE[GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4];
	pParentClass->bFrameLocked = false;


	BITMAPINFO bi;

	ZeroMemory(&bi,sizeof(bi));
	bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth=pParentClass->m_SCConfig.iWidth;
	bi.bmiHeader.biHeight=pParentClass->m_SCConfig.iHeight;
	bi.bmiHeader.biPlanes=1;
	bi.bmiHeader.biBitCount=pParentClass->m_SCConfig.biBitCount*8;
	bi.bmiHeader.biCompression=BI_RGB;
	bi.bmiHeader.biSizeImage=4*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

	pParentClass->OnBnClickedButton3();
	SYSTEMTIME time;
	::GetLocalTime(&time);
	memset(pParentClass->m_SCConfig.OutFilePath,0x00,sizeof(pParentClass->m_SCConfig.OutFilePath));
	sprintf_s(pParentClass->m_SCConfig.OutFilePath,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초_%d.avi",pParentClass->m_SCConfig.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,iMovieCount);
	sprintf_s(szMP3FileName,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초.mp3",pParentClass->m_SCConfig.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);

	pParentClass->m_SCM.Init(hTargetWnd,pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight);
    
	switch(pParentClass->iCodec)
	{
	case ID_BMP_TEMP:
		{
			pParentClass->m_h263Encoder.Init(pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,pParentClass->m_SCConfig.iKeyFrame);
			pParentClass->aviFile=AVI_open_output_file(pParentClass->m_SCConfig.OutFilePath);
			AVI_set_video(pParentClass->aviFile,pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,FrameRate,CodecName[pParentClass->iCodec]);
			AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);
			AVI_set_Max_Len(80000000);
		}
		break;
	case ID_XVID:
		pParentClass->m_h263Encoder.Init(pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,pParentClass->m_SCConfig.iKeyFrame);
		pParentClass->aviFile=AVI_open_output_file(pParentClass->m_SCConfig.OutFilePath);
		AVI_set_video(pParentClass->aviFile,pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,pParentClass->m_SCConfig.iFrameRate,CodecName[pParentClass->iCodec]);
		AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);
		AVI_set_Max_Len(2000000000);
		break;
	case ID_JEPG:
		break;
	}


	time_old=::timeGetTime();
	time_start = time_old;

	CFile file;
	char FileName[255];
	strTempDir = pParentClass->m_SCConfig.OutDirPath;
	strTempDir +="\\Temp";
	memset(FileName,0x00,255);
	//sprintf(FileName,"%s.tmp",pParentClass->m_SCConfig.OutFilePath);
	sprintf_s(FileName,"\\ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초.avi.tmp",time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
	strTempDir += FileName;
	file.Open(strTempDir,CFile::modeCreate|CFile::modeReadWrite);

	while(pParentClass->bStart)
	{
		unsigned long now;
		

		now = ::timeGetTime();
		if(!pParentClass->bPause)
		if(now -time_old >= (unsigned long)time_frame)
		{
			time_old=now;
			if(!pParentClass->bFrameLocked)
			{
				sg_Shared.iFrameRate =  pParentClass->m_SCConfig.iFrameRate;
				sg_Shared.iKeyFrame =  pParentClass->m_SCConfig.iKeyFrame;
				//sg_Shared.SetRecord(true);
			}else {

				SIZE size;
				size.cx = pParentClass->m_SCConfig.iWidth;
				size.cy = pParentClass->m_SCConfig.iHeight;

				//sg_Shared.SetRecord(1);

				iTotalFrameCount ++;

				tWorkBMP a;
				a.iDepth = 4;
				a.x = pParentClass->nWidth;
				a.y = pParentClass->nHeight;
				a.l = pParentClass->nWidth*a.iDepth;

				memcpy(Buffer,pParentClass->pFrameBuffer,a.l*a.y);

				a.b = Buffer;

				pParentClass->bFrameLocked = false;

				
			
				for(int k = 0;k<a.y;k++)
				{
					memcpy(pParentClass->pFrameBuffer1+(a.y-k)*a.l,a.b+a.l*k,a.l);
				}

//				ShrinkWorkingBitmap (&a, &b,pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight);
//				free (b.b);

				long time_now = time_old;
				long time_gap = time_now - time_start;
				int iTotalFrameDelayedCount;
				
				switch(pParentClass->iCodec)
				{
				case ID_BMP_TEMP:				
					{
						int i;
						int iSize;

						if(time_gap/(time_frame) > (long)(iTotalFrameCount))
						{
							iTotalFrameDelayedCount  = time_gap/time_frame - iTotalFrameCount;
							iTotalFrameErrorCount += iTotalFrameDelayedCount;
						} else if(time_gap/time_frame < (long)iTotalFrameCount){
						
						}
						iSize = a.x*a.y*a.iDepth;

						file.Write(&iSize,sizeof(int));
						file.Write(a.b,iSize);

						iSize = 0;

						for(i=0;i<(long)iTotalFrameErrorCount;i++ )
						{
							file.Write(&iSize,sizeof(int));
							//file.Write(a.b,iSize);
						}
						iTotalFrameCount += iTotalFrameErrorCount;
						iTotalFrameErrorCount = 0;

						if(pParentClass->bPreview)
						{
							RECT rt;
							pParentClass->m_View1.GetWindowRect(&rt);

							bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
							bi.bmiHeader.biWidth=rt.right - rt.left;
							bi.bmiHeader.biHeight=rt.bottom - rt.top;
							bi.bmiHeader.biPlanes=1;
							bi.bmiHeader.biBitCount=pParentClass->m_SCConfig.biBitCount*8;
							bi.bmiHeader.biCompression=BI_RGB;
							bi.bmiHeader.biSizeImage=pParentClass->m_SCConfig.biBitCount*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

							IVDPixmapResampler::FilterMode fmode = IVDPixmapResampler::kFilterPoint;
							VDPixmapBuffer src(pParentClass->m_SCConfig.iWidth, pParentClass->m_SCConfig.iHeight, nsVDPixmap::kPixFormat_XRGB8888);
							VDPixmapBuffer dst(bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, nsVDPixmap::kPixFormat_XRGB8888);
							memcpy(src.data,pParentClass->pFrameBuffer1,pParentClass->m_SCConfig.iWidth*pParentClass->m_SCConfig.iHeight*4);
							VDPixmapResample(dst,src,fmode);
					
							pParentClass->m_h263Encoder.DrawImage(pParentClass->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_Preview.GetSafeHwnd(),hTargetWnd,dst.data,&bi);

						}

						
						break;
					}
				case ID_XVID:

					int i;

					if(time_gap/(time_frame) > ((long)iTotalFrameCount))
					{
						iTotalFrameDelayedCount  = time_gap/time_frame - iTotalFrameCount;
						iTotalFrameErrorCount += iTotalFrameDelayedCount;
					} else if(time_gap/time_frame < (long)iTotalFrameCount){

					}
	
					pParentClass->m_h263Encoder.Compress(a.b,true);

					if(pParentClass->bPreview)
					{
						RECT rt;
						pParentClass->m_View1.GetWindowRect(&rt);

						bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
						bi.bmiHeader.biWidth=rt.right - rt.left;
						bi.bmiHeader.biHeight=rt.bottom - rt.top;
						bi.bmiHeader.biPlanes=1;
						bi.bmiHeader.biBitCount=pParentClass->m_SCConfig.biBitCount*8;
						bi.bmiHeader.biCompression=BI_RGB;
						bi.bmiHeader.biSizeImage=pParentClass->m_SCConfig.biBitCount*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

						IVDPixmapResampler::FilterMode fmode = IVDPixmapResampler::kFilterPoint;
						VDPixmapBuffer src(pParentClass->m_SCConfig.iWidth, pParentClass->m_SCConfig.iHeight, nsVDPixmap::kPixFormat_XRGB8888);
						VDPixmapBuffer dst(bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, nsVDPixmap::kPixFormat_XRGB8888);
						memcpy(src.data,pParentClass->pFrameBuffer1,pParentClass->m_SCConfig.iWidth*pParentClass->m_SCConfig.iHeight*4);
		

						VDPixmapResample(dst,src,fmode);
						pParentClass->m_h263Encoder.DrawImage(pParentClass->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_Preview.GetSafeHwnd(),hTargetWnd,dst.data,&bi);
						//pParentClass->m_h263Encoder.DrawImage(pParentClass->m_View1.GetSafeHwnd(),hTargetWnd,dst.data,&bi);
				
						//pParentClass->m_h263Encoder.DrawImage(pParentClass->m_View1.GetSafeHwnd(),hTargetWnd,dst.data,&bi);
					}

					EnterCriticalSection(&pParentClass->cs);
					
					bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
					bi.bmiHeader.biWidth=pParentClass->m_SCConfig.iWidth;
					bi.bmiHeader.biHeight=pParentClass->m_SCConfig.iHeight;
					bi.bmiHeader.biPlanes=1;
					bi.bmiHeader.biBitCount=24;
					bi.bmiHeader.biCompression=BI_RGB;
					bi.bmiHeader.biSizeImage=pParentClass->m_SCConfig.biBitCount*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;
					pParentClass->SendStreamData(bi.bmiHeader,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());

					if( AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount()))
					{
						AVI_close(pParentClass->aviFile);

						iMovieCount++;
						memset(pParentClass->m_SCConfig.OutFilePath,0x00,sizeof(pParentClass->m_SCConfig.OutFilePath));
						sprintf_s(pParentClass->m_SCConfig.OutFilePath,"%s/ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초_%d.avi",pParentClass->m_SCConfig.OutDirPath,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,iMovieCount);
						pParentClass->aviFile=AVI_open_output_file(pParentClass->m_SCConfig.OutFilePath);
						AVI_set_video(pParentClass->aviFile,pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,pParentClass->m_SCConfig.iFrameRate,CodecName[pParentClass->iCodec]);
						AVI_set_audio(pParentClass->aviFile,2,NUM_SAMPLES,16,WAVE_FORMAT_PCM);

						AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());
					}
					
					for(i=0;i<(long)iTotalFrameErrorCount;i++ )
					{
						pParentClass->m_h263Encoder.Compress(NULL,true);
						AVI_write_frame(pParentClass->aviFile,(char *)0,0);	
					}
					LeaveCriticalSection(&pParentClass->cs);
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

	file.Close();

	pParentClass->m_PauseButton.EnableWindow(true);
	pParentClass->m_PauseButton.SetWindowText("Cancel");

	//sg_Shared.SetRecord(0);
	switch(pParentClass->iCodec)
	{
	case ID_BMP_TEMP:				
		{
			CFile file;
			char FileName[255];
			char buffer1[25];
			int iSize;
			tWorkBMP a;

			strTempDir = pParentClass->m_SCConfig.OutDirPath;
			strTempDir +="\\Temp";
			memset(FileName,0x00,255);
			sprintf_s(FileName,"\\ScreenRecord_%2.0d월%2.0d일%2.0d시%2.0d분%2.0d초.avi.tmp",time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
			strTempDir += FileName;
			
			file.Open(strTempDir,CFile::modeRead);
			unsigned long temlen = (unsigned long)file.GetLength()/iTotalFrameCount;

			for(unsigned int i =0 ; i< iTotalFrameCount ;i++)
			{


				if(pParentClass->bPause) 
				{
					break;
				}
				file.Read(&iSize,sizeof(int));
				if(iSize)
				{

					memset(Buffer,0x00,iSize);
					file.Read(Buffer,iSize);

					memset(buffer1,0x00,25);
					sprintf_s(buffer1,"%d%%",i*100/iTotalFrameCount);
					pParentClass->m_RecordButton.SetWindowText(buffer1);
					pParentClass->m_RecordButton.EnableWindow(false);
					
					a.l = (pParentClass->nWidth)*4;
					a.iDepth = 4;
					a.x = pParentClass->nWidth;
					a.y = pParentClass->nHeight;
					a.b = (BYTE *)Buffer;
					pParentClass->m_h263Encoder.Compress(a.b,true);
					
					if(pParentClass->bPreview)
					{
						for(int k = 0;k<a.y;k++)	memcpy(pParentClass->pFrameBuffer1+(a.y-k)*a.l,a.b+a.l*k,a.l);
						RECT rt;
						pParentClass->m_View1.GetWindowRect(&rt);

						bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
						bi.bmiHeader.biWidth=rt.right - rt.left;
						bi.bmiHeader.biHeight=rt.bottom - rt.top;
						bi.bmiHeader.biPlanes=1;
						bi.bmiHeader.biBitCount=pParentClass->m_SCConfig.biBitCount*8;
						bi.bmiHeader.biCompression=BI_RGB;
						bi.bmiHeader.biSizeImage=pParentClass->m_SCConfig.biBitCount*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

						IVDPixmapResampler::FilterMode fmode = IVDPixmapResampler::kFilterPoint;
						VDPixmapBuffer src(pParentClass->m_SCConfig.iWidth, pParentClass->m_SCConfig.iHeight, nsVDPixmap::kPixFormat_XRGB8888);
						VDPixmapBuffer dst(bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, nsVDPixmap::kPixFormat_XRGB8888);
						memcpy(src.data,pParentClass->pFrameBuffer1,pParentClass->m_SCConfig.iWidth*pParentClass->m_SCConfig.iHeight*4);
		

						VDPixmapResample(dst,src,fmode);
				
						pParentClass->m_h263Encoder.DrawImage(pParentClass->m_View1.GetSafeHwnd(),hTargetWnd,dst.data,&bi);

					
					}
					//AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());	

				} else {
					/*
					sprintf(buffer1,"%d%%",i*100/iTotalFrameCount);
					pParentClass->m_RecordButton.SetWindowText(buffer1);
					pParentClass->m_RecordButton.EnableWindow(false);

					
					a.l = (pParentClass->nWidth)*4;
					a.iDepth = 4;
					a.x = pParentClass->nWidth;
					a.y = pParentClass->nHeight;
					a.b = (BYTE *)Buffer;
					pParentClass->m_h263Encoder.Compress(a.b,true);
					*/
					AVI_write_frame(pParentClass->aviFile,(char *)0,0);	
				}
				
			}
			/*
			file.Close();
			pParentClass->m_h263Encoder.enc_stop();

			file.Open(strTempDir,CFile::modeRead);
			pParentClass->m_h263Encoder.Init(pParentClass->m_SCConfig.iWidth,pParentClass->m_SCConfig.iHeight,pParentClass->m_SCConfig.iKeyFrame,NULL,ARG_PASS);
			for(int i =0 ; i< iTotalFrameCount ;i++)
			{
				if(pParentClass->bPause) 
				{
					break;
				}
				file.Read(&iSize,sizeof(int));
				if(iSize)
				{
					memset(Buffer,0x00,iSize);
					file.Read(Buffer,iSize);

					memset(buffer1,0x00,25);
					sprintf(buffer1,"%d%%",i*100/iTotalFrameCount);
					pParentClass->m_RecordButton.SetWindowText(buffer1);
					pParentClass->m_RecordButton.EnableWindow(false);

					
					a.l = (pParentClass->nWidth)*4;
					a.iDepth = 4;
					a.x = pParentClass->nWidth;
					a.y = pParentClass->nHeight;
					a.b = (BYTE *)Buffer;

					pParentClass->m_h263Encoder.Compress(a.b,true);
					

					AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());			
				} else {
					sprintf(buffer1,"%d%%",i*100/iTotalFrameCount);
					pParentClass->m_RecordButton.SetWindowText(buffer1);
					pParentClass->m_RecordButton.EnableWindow(false);

					
					a.l = (pParentClass->nWidth)*4;
					a.iDepth = 4;
					a.x = pParentClass->nWidth;
					a.y = pParentClass->nHeight;
					a.b = (BYTE *)Buffer;
					pParentClass->m_h263Encoder.Compress(a.b,true);
					AVI_write_frame(pParentClass->aviFile,(char *)pParentClass->m_h263Encoder.GetData(),pParentClass->m_h263Encoder.GetCount());			
				}
				
				
			}
			*/
			pParentClass->bPause = false;

			file.Close();
			::DeleteFile(strTempDir);
			AVI_close(pParentClass->aviFile);

			strTempDir = pParentClass->m_SCConfig.OutDirPath;
			strTempDir +="\\Temp";
			DeleteFolder(strTempDir);
			CreateDirectory("Temp",NULL);
		}
		break;
	case ID_XVID:
		AVI_close(pParentClass->aviFile);
		break;
	case ID_JEPG:
		ijlFree(&m_Jpeg);  
		AVI_close(pParentClass->aviFile);
		break;
	}

	if(pParentClass->bPreview)
	{
		sg_Shared.SetRecord(true);
	} else {
		sg_Shared.SetRecord(false);
	}

//	pParentClass->m_MP3_File.Close();
	pParentClass->m_h263Encoder.enc_stop();
	pParentClass->bFrameLocked = true;
	delete [] pParentClass->pFrameBuffer ;
	delete []m_lpBuffer;

	pParentClass->m_FrameRate.EnableWindow(1);
	pParentClass->m_KeyFrame.EnableWindow(1);
	pParentClass->m_Width.EnableWindow(1);
	pParentClass->m_Height.EnableWindow(1);
	pParentClass->m_ScreenSize.EnableWindow(1);

	pParentClass->m_RecordButton.EnableWindow(true);
	pParentClass->m_RecordButton.SetWindowText("Record");
	pParentClass->m_PauseButton.SetWindowText("Pause");
	pParentClass->m_PauseButton.EnableWindow(false);
	pParentClass->m_Close.EnableWindow(true);
	pParentClass->m_RecordMethod.EnableWindow(true);
	sg_Shared.iIndicateType = 1;

	delete [] Buffer ;

	return 0;
}


void CALLBACK CScreenEncoderDlg::waveInProc(
  HWAVEIN hwi,       
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2     
)
{
	CScreenEncoderDlg * pWorkProcess;
	pWorkProcess =(CScreenEncoderDlg *) dwInstance;
	
	// return ;

	switch(uMsg)
	{
	case WIM_DATA:
		{
			WAVEHDR * wh = (WAVEHDR *)dwParam1;
			
			unsigned long num_samples = wh->dwBytesRecorded / 4;

			//wh->
			if (pWorkProcess->bStart)
			{

				//int mp3buffer_size = (unsigned long)(1.25 * (double)num_samples) + 7200;		
				//unsigned char * mp3buffer = new unsigned char[mp3buffer_size];
				if(pWorkProcess->bSoundRecord)
				{
					if(pWorkProcess->aviFile)
					{
						if(pWorkProcess->iCodec == ID_BMP_TEMP)
						{
							//short level = pWorkProcess->Encode_Buffers(dwParam1,mp3buffer,&mp3buffer_size);
							//short  sSize=mp3buffer_size;
							AVI_write_audio(pWorkProcess->aviFile,wh->lpData,wh->dwBytesRecorded);
						} else {
							
						EnterCriticalSection(&pWorkProcess->cs);
						AVI_write_audio(pWorkProcess->aviFile,wh->lpData,wh->dwBytesRecorded);
						LeaveCriticalSection(&pWorkProcess->cs);
						

						}
					}
				}
				//delete [] mp3buffer;

			}
			if(pWorkProcess->m_hWaveIn)
			{
				waveInAddBuffer(pWorkProcess->m_hWaveIn,wh,sizeof(WAVEHDR));
			}
		}

		break;
	default:

		break;
	}
}


CScreenEncoderDlg::CScreenEncoderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CScreenEncoderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CScreenEncoderDlg)
	//}}AFX_DATA_INIT
	bShiftDown = false;;
	bCtrlDown= false;;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CScreenEncoderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CScreenEncoderDlg)
	DDX_Control(pDX, IDC_PAUSE, m_PauseButton);
	DDX_Control(pDX, IDC_FRAMERATE, m_FrameRate);
	DDX_Control(pDX, IDC_KEYFRAME, m_KeyFrame);
	DDX_Control(pDX, IDC_CROSS1, m_Cross);
	DDX_Control(pDX, IDC_RECORD, m_RecordButton);
	DDX_Control(pDX, IDC_WIDTH, m_Width);
	DDX_Control(pDX, IDC_HEIGHT, m_Height);
	DDX_Control(pDX, IDC_SCREENSIZE, m_ScreenSize);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDOK, m_Close);
	DDX_Control(pDX, IDC_RECORD_METHOD, m_RecordMethod);
	DDX_Control(pDX, IDC_CURDIR, m_CurDir);
	DDX_Control(pDX, IDC_WINDOWNAME, m_CurWindowName);
	DDX_Control(pDX, IDC_SELWIN, m_SelWinButton);
	DDX_Control(pDX, IDC_VIEW1, m_View1);
	DDX_Control(pDX, IDC_CHECK1, m_Preview);
	DDX_Control(pDX, IDC_WIDE, m_Wide);
	DDX_Control(pDX, IDC_SOUND_RECORD, m_SoundRecord);
	DDX_Control(pDX, IDC_bitrate, M_BitRate);
}

BEGIN_MESSAGE_MAP(CScreenEncoderDlg, CDialog)
	//{{AFX_MSG_MAP(CScreenEncoderDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_RECORD, OnRecord)
	ON_BN_CLICKED(IDC_SELWIN, OnBnClickedSelwin)
//	ON_WM_TIMER()
	ON_CBN_EDITCHANGE(IDC_SCREENSIZE, OnEditchangeScreensize)
	ON_CBN_SELCHANGE(IDC_SCREENSIZE, OnSelchangeScreensize)
	ON_EN_CHANGE(IDC_WIDTH, OnChangeWidth)
	ON_EN_CHANGE(IDC_HEIGHT, OnChangeHeight)
	ON_EN_CHANGE(IDC_KEYFRAME, OnChangeKeyframe)
	ON_EN_CHANGE(IDC_FRAMERATE, OnChangeFramerate)
	ON_BN_CLICKED(IDC_PAUSE, OnPause)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_WM_SHOWWINDOW()
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_CBN_SELCHANGE(IDC_RECORD_METHOD, &CScreenEncoderDlg::OnCbnSelchangeRecordMethod)
	ON_EN_CHANGE(IDC_CURDIR, &CScreenEncoderDlg::OnEnChangeCurdir)
	ON_EN_CHANGE(IDC_WINDOWNAME, &CScreenEncoderDlg::OnEnChangeWindowname)
	
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDOK, &CScreenEncoderDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON3, &CScreenEncoderDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_CHECK1, &CScreenEncoderDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_WIDE, &CScreenEncoderDlg::OnBnClickedWide)
	ON_WM_KEYUP()
	ON_WM_KEYDOWN()
	ON_BN_CLICKED(IDC_SOUND_RECORD, &CScreenEncoderDlg::OnBnClickedSoundRecord)
	ON_EN_CHANGE(IDC_bitrate, &CScreenEncoderDlg::OnEnChangebitrate)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, &CScreenEncoderDlg::OnDeltaposSpin1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_START, &CScreenEncoderDlg::OnBnClickedStart)
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CScreenEncoderDlg message handlers

BOOL CScreenEncoderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

//	LoadSkinFile("LightBlue.smf"); 
	SetSingleDialogSkin( this->m_hWnd ,"Dialog");

	//_CrtSetBreakAlloc(133);
	this->m_MainCtrlPanel.hWndMaster = this->GetSafeHwnd();
	//this->m_Display.hWndMaster = this->GetSafeHwnd();

	::SetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE,
	::GetWindowLong(this->GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_CONTROLPARENT);

	RECT rt;
	this->GetWindowRect(&rt);
	this->SetWindowPos(NULL,0,0,rt.right-rt.left- 260,rt.bottom-rt.top,SWP_NOZORDER|SWP_NOMOVE);
	//this->SetWindowPos(NULL,0,0,rt.right-rt.left- 260,rt.bottom-rt.top,SWP_NOZORDER|SWP_NOMOVE);
	bPreview = false;

	InitializeCriticalSection(&cs);

	bSoundRecord =true;
	this->m_SoundRecord.SetCheck(bSoundRecord);


	this->m_MainCtrlPanel.Create(m_MainCtrlPanel.IDD);
	this->m_MainCtrlPanel.ShowWindow(SW_HIDE);


	m_hMp3Enc =NULL;
	m_hMp3Enc=::LoadLibrary(_T("libMp3Enc.dll"));

	if(m_hMp3Enc)
	{
		InitEnc = (int(*)(int,int ,int ))GetProcAddress(m_hMp3Enc,"InitEnc");
		Encode_Buffers = (short(*)(DWORD ,unsigned char * ,int * ))GetProcAddress(m_hMp3Enc,"Encode_Buffers");
		Close = (void(*)(void))GetProcAddress(m_hMp3Enc,"Close");
	}

	init(this->m_View1.GetSafeHwnd());
	Connect("165.246.154.146",6899);

	ICINFO info;
	g_Config.m_bDebugLog = 0;
	memset(g_Config.m_szFileNamePostfix,0x00,14);
	g_Config.m_fFrameRateTarget = 20.0;
	g_Config.m_bVideoHalfSize = 0;
//	g_Config.m_iAudioDevice =2;
	g_Config.m_wHotKey[0] = 1136;
	g_Config.m_wHotKey[1] = 1137;
	g_Config.m_wHotKey[2] = 1138;
	g_Config.m_wHotKey[3] = 117;
	g_Config.m_wHotKey[4] = 118;
	g_Config.m_wHotKey[5] = 119;
	g_Config.m_wHotKey[6] = 120;
	g_Config.m_wHotKey[7] = 632;
	g_Config.m_bUseDirectInput = 0;
	g_Config.m_bUseKeyboard = true;
	g_Config.m_bUseOverheadCompensation = false;
	g_Config.m_bGDIFrame = true;
	g_Config.m_bGDIDesktop = false;
	g_Config.m_abUseAPI[0] = true; //DX9
	g_Config.m_abUseAPI[1] = false;
	g_Config.m_abUseAPI[2] = false; //GDI
	g_Config.m_bShowIndicator = false;
	g_Config.m_bVideoCodecMsg = false;
	sg_Shared.bReset = false;

	g_Config.m_VideoCodec.GetCodecInfo(info);

	sg_Config.CopyConfig( g_Config );


#ifdef _DEBUG
//	CTaksiShared* pDll = &sg_Shared;
#endif

	
#ifdef USE_DX
	Test_DirectX8(this->GetSafeHwnd());		// Get method offsets of IDirect3DDevice8 vtable
	Test_DirectX9(this->GetSafeHwnd());		// Get method offsets of IDirect3DDevice9 vtable
#endif

	// Set HWND for reference by the DLL
	if ( ! sg_Shared.InitMasterWnd(this->GetSafeHwnd()))
	{
		return -1;
	}

	
	sg_Shared.HookCBT_Uninstall();
	sg_Shared.SetUserCallback(&DrawImageData1);
		
	//::FindWindow("GxWindowClassD3d","월드 오브 워크래프트")

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	iCodec =0;
	pMainClass=this;

	

	AspectRatio = ((double)GetSystemMetrics(SM_CXSCREEN))/((double)GetSystemMetrics(SM_CYSCREEN));
	double fRet = fabs(AspectRatio - 4.0/3.0 );

	/*
	if( fRet  > 0.1)
	{
		bWide = true;
		this->m_Wide.SetCheck(1);
	} else {
		bWide = false;
		this->m_Wide.SetCheck(0);
	}
	*/

	OnBnClickedWide();

	bStart=false;
	bPause=false;
	hTargetWnd  = NULL;

	memset(m_SCConfig.OutDirPath,0x00,255);
	::GetCurrentDirectoryA(255,m_SCConfig.OutDirPath);
	this->m_CurDir.SetWindowTextA(m_SCConfig.OutDirPath);

	memset(sg_Shared.OutDirPath,0x00,255);
	::GetCurrentDirectoryA(255,sg_Shared.OutDirPath);
	this->m_CurDir.SetWindowTextA(m_SCConfig.OutDirPath);


	SIZE size;

	m_SCConfig.iWidth;
	m_SCConfig.iHeight = (int)((double)m_SCConfig.iWidth/AspectRatio);

	size.cx = m_SCConfig.iWidth;
	size.cy = m_SCConfig.iHeight;
	sg_Shared.SetDestSize(size);

	sg_Shared.m_hPreviewWnd = m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_Preview.GetSafeHwnd();

	iBitRate = 5000;
	this->m_h263Encoder.SetBItrate(iBitRate);
	this->M_BitRate.SetWindowTextA("5000");

	m_RecordMethod.SetWindowTextA("H.263 XVID");
	m_RecordMethod.AddString("H.263 XVID");
	//m_RecordMethod.AddString("JPEG");
	m_RecordMethod.AddString("BMP=>XVID");

	this->m_PauseButton.EnableWindow(false);

	//this->m_ScreenSize.SetCurSel(4);
	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_ScreenSizeSel.SetCurSel(4);
	
	this->m_FrameRate.SetWindowText("25");
	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_FrameRate.SetWindowText("25");

	this->m_KeyFrame.SetWindowText("5");

	this->SetWindowPos(&wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);

	SoundInit();

//	pFrameBuffer1  = new BYTE[GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4];
	pFrameBuffer1  = new BYTE[GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4];
	return TRUE;  // return TRUE  unless you set the focus to a control
}
void (CScreenEncoderDlg::init)(HWND hWnd)
{
	DWORD dwThreadId;

	m_WorkThread.m_WorkProc.hWndMain = hWnd;
	m_WorkThread.m_WorkProc.Init();

	CloseHandle(BEGINTHREADEX(NULL,0,m_WorkThread.THConnectServer,&m_WorkThread,0,&dwThreadId));
}
void (CScreenEncoderDlg::Connect)(char * AddrTo,int PortNum)
{
	m_WorkThread.m_WorkProc.ConnectServer(AddrTo,PortNum);
}
int (CScreenEncoderDlg::StartStreaming)()
{
	m_WorkThread.m_WorkProc.StartStreaming();
	return 0;
}
int (CScreenEncoderDlg::SendStreamData)(BITMAPINFOHEADER bi,char * buffer,int len)
{
	m_WorkThread.m_WorkProc.SceneUpdate(bi,buffer,len);
	return 0;
}
void CScreenEncoderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

HCURSOR CScreenEncoderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CScreenEncoderDlg::OnRecord() 
{
	if(!bStart)
	{
		m_RecordMethod.EnableWindow(0);
		//this->ShowWindow(SW_MINIMIZE);
		bStart=true;
		m_RecordButton.SetWindowText("Stop");
		aviFile = NULL;
		
//		::AfxBeginThread(EncodingThreadProc,this,THREAD_PRIORITY_TIME_CRITICAL);

		this->m_PauseButton.EnableWindow(true);
		this->m_Close.EnableWindow(false);

		this->M_BitRate.EnableWindow(0);
		m_FrameRate.EnableWindow(0);
		m_KeyFrame.EnableWindow(0);
		m_Width.EnableWindow(0);
		m_Height.EnableWindow(0);
		m_ScreenSize.EnableWindow(0);
		m_SoundRecord.EnableWindow(0);

		sg_Shared.iFrameRate = m_SCConfig.iFrameRate;
		sg_Shared.iKeyFrame =  m_SCConfig.iKeyFrame;

		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_FrameSelSpin.EnableWindow(0);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_BitrateSelSpin.EnableWindow(0);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_ScreenSizeSel.EnableWindow(0);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.M_BitRate.EnableWindow(0);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_FrameRate.EnableWindow(0);

		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.SetRecord(bStart);
		sg_Shared.SetRecord(true);
		iTotalFrameCount = 0;
		iWaveCount = 0;
	} else {

		bStart=false;
		sg_Shared.SetRecord(false);
		
		
		if(m_SCConfig.bInit)
		{
			m_SCConfig.bInit = false;
		}

		this->M_BitRate.EnableWindow(1);
		m_RecordMethod.EnableWindow(1);
		m_SoundRecord.EnableWindow(1);
		m_RecordButton.SetWindowText("Record");
		this->m_PauseButton.EnableWindow(false);
		this->m_Close.EnableWindow(true);

		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_FrameSelSpin.EnableWindow(1);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_BitrateSelSpin.EnableWindow(1);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_ScreenSizeSel.EnableWindow(1);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.M_BitRate.EnableWindow(1);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_FrameRate.EnableWindow(1);
		this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.SetRecord(bStart);

		m_FrameRate.EnableWindow(1);
		m_KeyFrame.EnableWindow(1);
		m_Width.EnableWindow(1);
		m_Height.EnableWindow(1);
		m_ScreenSize.EnableWindow(1);

		m_RecordButton.EnableWindow(true);
		m_RecordButton.SetWindowText("Record");
		m_PauseButton.SetWindowText("Pause");
		m_PauseButton.EnableWindow(false);
		m_Close.EnableWindow(true);
		m_RecordMethod.EnableWindow(true);
		sg_Shared.iIndicateType = 1;

	}
}

//void CScreenEncoderDlg::OnTimer(UINT nIDEvent) 
//{
//	CDialog::OnTimer(nIDEvent);
//}

void CScreenEncoderDlg::OnEditchangeScreensize() 
{	
}

void CScreenEncoderDlg::OnSelchangeScreensize() 
{
	char buffer[255];
	int  iCurSel;
	iCurSel=this->m_ScreenSize.GetCurSel();

	{
		AspectRatio = ((double)GetSystemMetrics(SM_CXSCREEN))/((double)GetSystemMetrics(SM_CYSCREEN));
	}

	switch(iCurSel)
	{
	case 0:
		m_SCConfig.iWidth=160;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 1:
		m_SCConfig.iWidth=320;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 2:
		m_SCConfig.iWidth=480;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 3:
		m_SCConfig.iWidth=640;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 4:
		m_SCConfig.iWidth=800;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 5:
		m_SCConfig.iWidth=1024;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 6:
		m_SCConfig.iWidth=1280;
		m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	case 7:
		if(hTargetWnd != NULL)
		{
			RECT rt;
			::GetWindowRect(hTargetWnd,&rt);
			m_SCConfig.iWidth	= rt.right - rt.left;
			m_SCConfig.iHeight	= rt.bottom - rt.top;

		} else {
			m_SCConfig.iWidth=GetSystemMetrics(SM_CXSCREEN);
			//if(m_SCConfig.iWidth >= 1600)
			{
			//	m_SCConfig.iWidth = 1600;
			}
			m_SCConfig.iHeight=m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		}
		break;
	case 8:
		memset(buffer,0x00,sizeof(buffer));
		sprintf_s(buffer,"%d",GetSystemMetrics(SM_CXSCREEN)/2);
		m_Width.SetWindowText(buffer);
		m_SCConfig.iWidth=GetSystemMetrics(SM_CXSCREEN)/2;

		memset(buffer,0x00,sizeof(buffer));
		sprintf_s(buffer,"%d",m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio));
		m_Height.SetWindowText(buffer);
		m_SCConfig.iHeight=m_SCConfig.iHeight=(int)(((double)m_SCConfig.iWidth)/AspectRatio);
		break;
	}
	if(iCurSel==8)
	{
		//this->m_Wide.ShowWindow(SW_HIDE);
		this->m_Cross.ShowWindow(SW_SHOW);
		this->m_Height.ShowWindow(SW_SHOW);
		this->m_Width.ShowWindow(SW_SHOW);
	} else {
		//this->m_Wide.ShowWindow(SW_SHOW);
		this->m_Cross.ShowWindow(SW_HIDE);
		this->m_Height.ShowWindow(SW_HIDE)
			;
		this->m_Width.ShowWindow(SW_HIDE);
	}
	SIZE size;
	size.cx = m_SCConfig.iWidth;
	size.cy = m_SCConfig.iHeight;
	sg_Shared.SetDestSize(size);
}

void CScreenEncoderDlg::OnChangeWidth() 
{
	char buffer[255];
	int  iTemp;

	memset(buffer,0x00,sizeof(buffer));
	this->m_Width.GetWindowText(buffer,sizeof(buffer));
	sscanf_s(buffer,"%d",&iTemp);


	m_SCConfig.iWidth=iTemp;
	//if(m_SCConfig.iWidth > 1600)
	//{
	//	m_SCConfig.iWidth = 1600;
	//	memset(buffer,0x00,sizeof(buffer));
	//	sprintf(buffer,"%d",1600);
	//	this->m_Width.SetWindowTextA(buffer);
	//} else
	if(m_SCConfig.iWidth < 1)
	{
		m_SCConfig.iWidth = 1;
		memset(buffer,0x00,sizeof(buffer));
		sprintf_s(buffer,"%d",1);
		this->m_Width.SetWindowTextA(buffer);	
	}

	SIZE size;
	size.cx = m_SCConfig.iWidth;
	size.cy = m_SCConfig.iHeight;
	sg_Shared.SetDestSize(size);
}

void CScreenEncoderDlg::OnChangeHeight() 
{
	char buffer[255];
	int  iTemp;

	memset(buffer,0x00,sizeof(buffer));
	this->m_Height.GetWindowText(buffer,sizeof(buffer));
	sscanf_s(buffer,"%d",&iTemp);
	m_SCConfig.iHeight=iTemp;

	//if(m_SCConfig.iHeight > 1200)
	//{
	//	m_SCConfig.iHeight = 1200;
	//	memset(buffer,0x00,sizeof(buffer));
	//	sprintf(buffer,"%d",1200);
	//	this->m_Height.SetWindowTextA(buffer);
	//} else 
	if(m_SCConfig.iHeight < 1)
	{
		m_SCConfig.iHeight = 1;
		memset(buffer,0x00,sizeof(buffer));
		sprintf_s(buffer,"%d",1);
		this->m_Height.SetWindowTextA(buffer);	
	}
	SIZE size;
	size.cx = m_SCConfig.iWidth;
	size.cy = m_SCConfig.iHeight;
	sg_Shared.SetDestSize(size);
}

void CScreenEncoderDlg::OnChangeKeyframe() 
{
	char buffer[255];
	int  iTemp;
	memset(buffer,0x00,sizeof(buffer));
	this->m_KeyFrame.GetWindowText(buffer,sizeof(buffer));
	sscanf_s(buffer,"%d",&iTemp);
	m_SCConfig.iKeyFrame=iTemp;
}

void CScreenEncoderDlg::OnChangeFramerate() 
{
	char buffer[255];
	int  iTemp;
	memset(buffer,0x00,sizeof(buffer));
	this->m_FrameRate.GetWindowText(buffer,sizeof(buffer));
	sscanf_s(buffer,"%d",&iTemp);
	m_SCConfig.iFrameRate=iTemp;
}

BOOL CScreenEncoderDlg::DestroyWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
//	delete m_pEncodingThread;

	if(bStart) return 0;
	return CDialog::DestroyWindow();
}

void CScreenEncoderDlg::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CScreenEncoderDlg::OnPause() 
{
	if(bStart)
	{
		if(bPause)
		{
			this->m_PauseButton.SetWindowText("일시 정지");
			this->m_RecordButton.EnableWindow(true);
		} else {

			this->m_PauseButton.SetWindowText("다시 시작");
			this->m_RecordButton.EnableWindow(false);
		}
		bPause=!bPause;
	} else {
		bPause = false;		
	}
	
	// TODO: Add your control notification handler code here
}

void CScreenEncoderDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CScreenEncoderDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	if(bStart) return;
	//sg_Shared.m_bMasterExiting = true;
	sg_Shared.DestroyShared();
	CDialog::OnClose();
}

void CScreenEncoderDlg::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialog::OnActivate(nState, pWndOther, bMinimized);

	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
}

void CScreenEncoderDlg::OnCbnSelchangeRecordMethod()
{
	switch(this->m_RecordMethod.GetCurSel())
	{
	case 0:
		iCodec = ID_XVID;
		break;
	case 1:
		iCodec = ID_BMP_TEMP;
		break;
	case 2:
		
		break;
	}
	// TODO: Add your control notification handler code here
}

void CScreenEncoderDlg::OnEnChangeCurdir()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CScreenEncoderDlg::OnEnChangeWindowname()
{
	CString szWindowName;
	this->m_CurWindowName.GetWindowTextA(szWindowName);

	if(this->bPreview)
	{
	} else {
		//this->m_Preview.SetCheck(1);
		//OnBnClickedCheck1();
	}
	/*
	if(szWindowName.GetLength() != 0)
	{
		hTargetWnd = ::FindWindow(NULL,szWindowName);
		if(hTargetWnd  != NULL )
		{
			RECT rt;
			::GetWindowRect(hTargetWnd,&rt);
			iWidth	= rt.right - rt.left;
			iHeight = rt.bottom - rt.top;
			if((iWidth < 160) || (iHeight < 120))
			{

			} else if((iWidth < 320) || (iHeight < 240)) {
				m_SCConfig.iWidth=160;
				m_SCConfig.iHeight=120;
				this->m_ScreenSize.SetCurSel(0);

			} else if((iWidth < 480) || (iHeight < 340)) {
				m_SCConfig.iWidth=320;
				m_SCConfig.iHeight=240;
				this->m_ScreenSize.SetCurSel(1);

			} else if((iWidth < 640) || (iHeight < 480)) {
				m_SCConfig.iWidth=480;
				m_SCConfig.iHeight=320;
				this->m_ScreenSize.SetCurSel(2);

			} else if((iWidth < 800) || (iHeight < 600)) {
				m_SCConfig.iWidth=640;
				m_SCConfig.iHeight=480;
				this->m_ScreenSize.SetCurSel(3);

			} else if((iWidth < 1024) || (iHeight < 768)) {
				m_SCConfig.iWidth=800;
				m_SCConfig.iHeight=600;
				this->m_ScreenSize.SetCurSel(4);

			} else if((iWidth < 1280) || (iHeight < 1024)) {
				m_SCConfig.iWidth=1024;
				m_SCConfig.iHeight=768;
				this->m_ScreenSize.SetCurSel(5);
			} else if((iWidth < 1600) || (iHeight < 1200)) {
				m_SCConfig.iWidth=1280;
				m_SCConfig.iHeight=1024;
				this->m_ScreenSize.SetCurSel(6);
			}
		}
	}else {
		hTargetWnd = NULL;
		m_SCConfig.iWidth=GetSystemMetrics(SM_CXSCREEN);
		m_SCConfig.iHeight=GetSystemMetrics(SM_CYSCREEN);
		this->m_ScreenSize.SetCurSel(7);

	}
	*/
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialog::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}

void CScreenEncoderDlg::OnBnClickedSelwin()
{
	// TODO: Add your control notification handler code here
	this->m_CurWindowName.SetWindowTextA("");
	if(!bStart)
	{
		sg_Shared.bReset = false;
		this->m_SelWinButton.EnableWindow(0);
		if(sg_Shared.IsHookCBT())
		{
			sg_Shared.HookCBT_Uninstall();
		}
		this->m_CurWindowName.SetWindowTextA("대상 윈도우를 선택 하세요~");
		Sleep(100);
		sg_Shared.HookCBT_Install();
	}
}

/*

TAKSI_INDICATE_Ready = 0,
	TAKSI_INDICATE_Hooked,	// CBT is hooked. (looking for new app to load)
	TAKSI_INDICATE_Recording,	// actively recording right now
	TAKSI_INDICATE_RecordPaused,

*/

BOOL CScreenEncoderDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	static SIZE SrcSize,DesSize;


	switch(pCopyDataStruct->dwData)
	{
	case 0:
		{
			sg_Shared.iIndicateType = 0;

			SIZE * pSize = (SIZE*)pCopyDataStruct->lpData;
			nWidth =  pSize ->cx;
			nHeight = pSize ->cy;
			if(bStart)
			{
				if(!m_SCConfig.bInit)
				{
					
					m_SCConfig.bInit = true;
				}
			} else {
				
			}
		}
		break;
	case 1:
		{
			s_bbFormat = *(int *)pCopyDataStruct->lpData;
			switch(s_bbFormat)
			{
			case 20:
				{
					this->m_SCConfig.biBitCount = 3;
				}
				break;
			case 22:
				{
					this->m_SCConfig.biBitCount = 4;
				}
				break;
			}
		}
		break;
	case 2:
		{
			if(bStart)
			{
				if(m_SCConfig.bInit)
				{
					DWORD lSize = pCopyDataStruct->cbData; 
					if(!this->bFrameLocked)
					{
						memcpy(this->pFrameBuffer,pCopyDataStruct->lpData,lSize);
						this->bFrameLocked = true;
					}
					if(this->bPause)
					{
						sg_Shared.iIndicateType = 3;
					}else {
						sg_Shared.iIndicateType = 2;
					}
				}
			}else {

				sg_Shared.iIndicateType = 1;

				BITMAPINFO bi;

				ZeroMemory(&bi,sizeof(bi));
			

				RECT rt;
				

				if(bPreview)
				{
					if(m_SCConfig.bInit)
					{
						DWORD lSize = pCopyDataStruct->cbData; 
						BYTE *Buffer =  new BYTE [GetSystemMetrics(SM_CXSCREEN)*GetSystemMetrics(SM_CYSCREEN)*4*4];

						if(!this->bFrameLocked)
						{
							memcpy(this->pFrameBuffer,pCopyDataStruct->lpData,lSize);
							this->bFrameLocked = true;
						}

						tWorkBMP a;
						a.iDepth = 4;
						a.x = nWidth;
						a.y = nHeight;
						a.l = nWidth*a.iDepth;

						memcpy(Buffer,pFrameBuffer,a.l*a.y);

						a.b = Buffer;

						bFrameLocked = false;

						
					
						for(int k = 0;k<a.y;k++)
						{
							memcpy(pFrameBuffer1+(a.y-k)*a.l,a.b+a.l*k,a.l);
						}
						delete [] Buffer;

						RECT rt;
						m_View1.GetWindowRect(&rt);

						bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
						bi.bmiHeader.biWidth=rt.right - rt.left;
						bi.bmiHeader.biHeight=rt.bottom - rt.top;
						bi.bmiHeader.biPlanes=1;
						bi.bmiHeader.biBitCount=m_SCConfig.biBitCount*8;
						bi.bmiHeader.biCompression=BI_RGB;
						bi.bmiHeader.biSizeImage=m_SCConfig.biBitCount*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

						IVDPixmapResampler::FilterMode fmode = IVDPixmapResampler::kFilterPoint;
						VDPixmapBuffer src(m_SCConfig.iWidth, m_SCConfig.iHeight, nsVDPixmap::kPixFormat_XRGB8888);
						VDPixmapBuffer dst(bi.bmiHeader.biWidth, bi.bmiHeader.biHeight, nsVDPixmap::kPixFormat_XRGB8888);
						memcpy(src.data,pFrameBuffer,m_SCConfig.iWidth*m_SCConfig.iHeight*4);
		

						VDPixmapResample(dst,src,fmode);
						m_h263Encoder.DrawImage(m_MainCtrlPanel.m_MainTab.m_RecordDlg.m_Preview.GetSafeHwnd(),hTargetWnd,pFrameBuffer,&bi);
					//	m_h263Encoder.DrawImage(m_View1.GetSafeHwnd(),hTargetWnd,dst.data,&bi);

					}

					
			
					//pParentClass->m_h263Encoder.DrawImage(pParentClass->m_View1.GetSafeHwnd(),hTargetWnd,dst.data,&bi);
				}


			}
		}
		break;
	case 3:
		{
		/*	
			char szTitle[255];
			char szTitleOld[255];
			HWND TemphWnd;
			DWORD lSize = pCopyDataStruct->cbData; 
			memcpy(&TemphWnd,pCopyDataStruct->lpData,lSize);
			::GetWindowText(TemphWnd,m_szTargetWinName,255);
			this->m_CurWindowName.GetWindowTextA(szTitleOld,255);
			hTargetWnd = TemphWnd;
			if(strcmp(szTitleOld,m_szTargetWinName))
			{
				this->m_CurWindowName.SetWindowTextA(m_szTargetWinName);
				this->m_SelWinButton.EnableWindow(1);
			}else {
				
			}
			if(IsWindow(hTargetWnd))
			{
				TemphWnd = this->m_Display.GetParent()->GetSafeHwnd();
				if(TemphWnd  !=  hTargetWnd)
				{
					::SetParent(this->m_Display.GetSafeHwnd(),hTargetWnd);
					this->m_Display.ShowWindow(SW_Skih2974	1kihHOW);
				}
			}
			*/
		
			
			
		}
	//	break;
	case 4:
		{
			char szTitleOld[255];
			HWND TemphWnd;
			DWORD lSize = pCopyDataStruct->cbData; 
			memcpy(&TemphWnd,pCopyDataStruct->lpData,lSize);
			::GetWindowText(TemphWnd,m_szTargetWinName,255);
			this->m_CurWindowName.GetWindowTextA(szTitleOld,255);
			hTargetWnd = TemphWnd;
			if(strcmp(szTitleOld,m_szTargetWinName))
			{
				this->m_CurWindowName.SetWindowTextA(m_szTargetWinName);
				this->m_SelWinButton.EnableWindow(1);
			}else {
				
			}
			TemphWnd = ::FindWindowA(NULL,m_szTargetWinName);
			int a=0;
			
			if(IsWindow(hTargetWnd))
			{
				HWND TemphWnd = m_MainCtrlPanel.GetParent()->GetSafeHwnd();
				if(TemphWnd  !=  hTargetWnd)
				{
					m_MainCtrlPanel.UnsetParent();
					m_MainCtrlPanel.SetParentWithAlpha(hTargetWnd);
				}
			}
		}

		break;
	case 5:
		{
			
			m_MainCtrlPanel.UnsetParent();
//			m_Display.UnsetParent();
			SetTimer(TID_RECONNECT,3000,NULL);
		}
		break;
	
	}
	
	// TODO: Add your message handler code here and/or call default
	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

void CScreenEncoderDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	delete [] pFrameBuffer1;

	sg_Shared.HookCBT_Uninstall();
	//sg_Shared.DestroyShared();


	if (m_hWaveIn != 0)
	{
		waveInStop(m_hWaveIn);
		//waveInReset(m_hWaveIn);

		waveInClose(m_hWaveIn);

		m_hWaveIn = 0;
	}

	std::list<WAVEHDR*>::iterator i;
	for (i = m_headers.begin();
		 i != m_headers.end();
		 ++i)
	{
	     delete *i;
	}
	m_headers.clear();

	std::list<char*>::iterator j;
	for (j = m_buffers.begin();
		 j != m_buffers.end();
		 ++j)
	{
	     delete [] *j;
	}

	m_buffers.clear();

	if(m_hMp3Enc)
	{
		::FreeLibrary(m_hMp3Enc);
	}

	if(m_hStreamDll)
	{
		::FreeLibrary(m_hStreamDll);
	}

	DeleteCriticalSection(&cs);
	sg_Shared.DestroyShared();
	CDialog::OnOK();
}

void CScreenEncoderDlg::SoundInit()
{

	BOOL result = FALSE;
	UINT numDevs = waveInGetNumDevs();
	UINT m_inputDeviceID = 0;
	
	WAVEINCAPS wic;
	if (waveInGetDevCaps(m_inputDeviceID,&wic,sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR)
	{

	}

	ZeroMemory(&m_wfe,sizeof(WAVEFORMATEX));
	m_wfe.wFormatTag =  WAVE_FORMAT_PCM;
	m_wfe.nChannels = CHANEL;
	m_wfe.nSamplesPerSec = NUM_SAMPLES;
	m_wfe.wBitsPerSample = BIT;
	m_wfe.nBlockAlign = m_wfe.nChannels * (m_wfe.wBitsPerSample / 8);
	m_wfe.nAvgBytesPerSec = m_wfe.nSamplesPerSec * m_wfe.nBlockAlign;

	if (waveInOpen(&m_hWaveIn,m_inputDeviceID,&m_wfe,(DWORD)(VOID*)waveInProc,(DWORD)(void *)this,CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
	}
	else
	{
		const long numBuffers = 30;
		const long bufferSizeSeconds = 1;
		const long bufferSize = (long)((double)m_wfe.nSamplesPerSec / 4.0) * (m_wfe.wBitsPerSample / 8) * m_wfe.nChannels;

		result = TRUE;

		for (long x = 0; x < numBuffers; x++)
		{
			char * bufData = new char[bufferSize];
			if (bufData != 0)
			{
				m_buffers.push_back(bufData);
				ZeroMemory(bufData,bufferSize);

				WAVEHDR * bufHdr = new WAVEHDR;
				if (bufHdr != 0)
				{
					m_headers.push_back(bufHdr);
					ZeroMemory(bufHdr,sizeof(WAVEHDR));
					bufHdr->lpData = bufData;
					bufHdr->dwBufferLength = bufferSize;

					if (waveInPrepareHeader(m_hWaveIn,bufHdr,sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
					{
						result = FALSE;
					}
					else
					{
						if (waveInAddBuffer(m_hWaveIn,bufHdr,sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
						{
							result = FALSE;
						}
					}
				}
			}
		}

		if (!result)
		{
		}
		else
		{
			if (waveInStart(m_hWaveIn) != MMSYSERR_NOERROR)
			{
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}
		}
	}
	
	if(m_hMp3Enc)
	{
		InitEnc(m_wfe.nSamplesPerSec,128,4);
	}
}

void CScreenEncoderDlg::OnBnClickedButton3()
{
	strTempDir = m_SCConfig.OutDirPath;
	strTempDir +="\\Temp";
	DeleteFolder(strTempDir);
	CreateDirectory("Temp",NULL);
}

void CScreenEncoderDlg::OnCancel()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialog::OnCancel();
}

void CScreenEncoderDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialog::OnOK();
}

void CScreenEncoderDlg::OnBnClickedCheck1()
{
	if(this->m_Preview.GetCheck())
	{
		if(!bStart)
		{
			sg_Shared.SetRecord(true);
		}
		//bPreview = true;
		RECT rt;
		this->GetWindowRect(&rt);
		this->SetWindowPos(NULL,0,0,rt.right-rt.left+260,rt.bottom-rt.top,SWP_NOZORDER|SWP_NOMOVE);

	} else {
		if(!bStart)
		{
			sg_Shared.SetRecord(false);
		}
		RECT rt;
		this->GetWindowRect(&rt);
		this->SetWindowPos(NULL,0,0,rt.right-rt.left- 260,rt.bottom-rt.top,SWP_NOZORDER|SWP_NOMOVE);
		//bPreview = false;
	}
}

void CScreenEncoderDlg::OnBnClickedWide()
{
	char szTemp[255];
	int iCurSel = this->m_ScreenSize.GetCurSel();
	for (int i = this->m_ScreenSize.GetCount()-1; i >= 0; i--)
	{
	   this->m_ScreenSize.DeleteString( i );
	}

	//if(this->m_Wide.GetCheck())
	{
		this->bWide =(bool) this->m_Wide.GetCheck();

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",160,(int)((double)160.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",320,(int)((double)320.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",480,(int)((double)480.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",640,(int)((double)640.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",800,(int)((double)800.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",1024,(int)((double)1024.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",1280,(int)((double)1280.0/AspectRatio));
		this->m_ScreenSize.AddString(szTemp);

		this->m_ScreenSize.AddString("현재 해상도");
		this->m_ScreenSize.AddString("사용자 정의");

	}
 /* else {
		this->bWide = this->m_Wide.GetCheck();
		
		
		this->m_ScreenSize.AddString("160 X 120");
		this->m_ScreenSize.AddString("320 X 240");
		this->m_ScreenSize.AddString("480 X 360");
		this->m_ScreenSize.AddString("640 X 480");
		this->m_ScreenSize.AddString("800 X 600");
		this->m_ScreenSize.AddString("1024 X 768");
		this->m_ScreenSize.AddString("1280 X 1024");
		this->m_ScreenSize.AddString("현재 해상도");
		this->m_ScreenSize.AddString("사용자 정의");
	}
	*/
	
	this->m_ScreenSize.SetCurSel(iCurSel);
	this->OnSelchangeScreensize();

}

void CScreenEncoderDlg::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	switch(nChar)
	{
	case 16: //shift
		this->bShiftDown = false;
		break;
	case 17: //ctrl
		this->bCtrlDown = false;
		break;
	case 229:
		{

			
			int a=0;
		}
		break;
	}

	CDialog::OnKeyUp(nChar, nRepCnt, nFlags);
}

void CScreenEncoderDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default
	switch(nChar)
	{
	case VK_F7:
		{
			if(bCtrlDown)
			{
			}
			if(bShiftDown)
			{
			}
		}
		break;
	case VK_F8:
		{
			if(bCtrlDown)
			{
				this->m_MainCtrlPanel.m_MainTab.m_WebCtrlDlg.m_Display.SetTransparent(true);
			}
			if(bShiftDown)
			{
				this->m_MainCtrlPanel.m_MainTab.m_WebCtrlDlg.m_Display.SetTransparent(false);
			}
		}
		break;
	case VK_F12:
		{
			if(bCtrlDown)
			{
				/*
				if(::GetWindowLong(this->m_Display.GetSafeHwnd(), GWL_EXSTYLE) & WS_EX_TRANSPARENT)
				{
					::SetWindowLong(this->m_Display.GetSafeHwnd(), GWL_EXSTYLE,
					::GetWindowLong(this->m_Display.GetSafeHwnd(), GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
				} else {
					::SetWindowLong(this->m_Display.GetSafeHwnd(), GWL_EXSTYLE,
					::GetWindowLong(this->m_Display.GetSafeHwnd(), GWL_EXSTYLE) | WS_EX_TRANSPARENT);
				}

				this->m_Display.ShowWindow(SW_HIDE);
				::SetParent(this->m_Display.GetSafeHwnd(),NULL);
				*/
			}

			if(bShiftDown)
			{
				this->ShowWindow(SW_MINIMIZE);
			}
		}
		break;
	case 16: //shift
		this->bShiftDown = true;
		break;
	case 17: //ctrl
		this->bCtrlDown = true;
		break;
	case VK_F9:
		{
			if(bCtrlDown)
			{
				m_MainCtrlPanel.ShowWindow(SW_HIDE);
//				m_Display.UnsetParent();
			} else {
				m_MainCtrlPanel.UnsetParent();
				
//				m_Display.ShowWindow(SW_HIDE);
			}
		}
		break;
	case VK_F10:
		{
			if(bCtrlDown)
			{
				OnRecord();							
			}
		}
		break;
	case VK_F11:
		{
			if(bCtrlDown)
			{
				OnPause();							
			}
		}
		break;

	}

	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CScreenEncoderDlg::OnBnClickedSoundRecord()
{
	// TODO: Add your control notification handler code here
	bSoundRecord = !bSoundRecord;

	this->m_SoundRecord.SetCheck(bSoundRecord);
}

void CScreenEncoderDlg::OnEnChangebitrate()
{
	char buffer[255];
	int  iTemp;

	memset(buffer,0x00,sizeof(buffer));
	this->M_BitRate.GetWindowText(buffer,sizeof(buffer));
	sscanf_s(buffer,"%d",&iTemp);

	iBitRate =iTemp;
	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.iBitRate = iBitRate ;
	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.M_BitRate.SetWindowTextA(buffer);

	this->m_h263Encoder.SetBItrate(iBitRate );
}

void CScreenEncoderDlg::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	char buffer[255];

	iBitRate -= pNMUpDown->iDelta*1000;

	memset(buffer,0x00,sizeof(buffer));
	sprintf_s(buffer,"%d",iBitRate);
	this->M_BitRate.SetWindowTextA(buffer);

	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.iBitRate = iBitRate ;
	this->m_MainCtrlPanel.m_MainTab.m_RecordDlg.M_BitRate.SetWindowTextA(buffer);

	this->m_h263Encoder.SetBItrate(iBitRate );
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

void CScreenEncoderDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	switch(nIDEvent)
	{
	case TID_RECONNECT:
		//this->m_SelWinButton.EnableWindow(0);
		hTargetWnd = ::FindWindowA(NULL,m_szTargetWinName);

		if(::IsWindow(hTargetWnd))
		{

			sg_Shared.ReConnect(hTargetWnd);
/*
			if(sg_Shared.IsHookCBT())
			{
				sg_Shared.HookCBT_Uninstall();
			}

			Sleep(100);
			sg_Shared.HookCBT_Install();

	
			::SendMessage(hTargetWnd ,WM_ACTIVATE,WA_INACTIVE,NULL);
			Sleep(100);
			::SendMessage(hTargetWnd ,WM_ACTIVATE,WA_ACTIVE,NULL);
			::SendMessage(hTargetWnd ,WM_SETFOCUS,NULL,NULL);
			*/
			
			KillTimer(TID_RECONNECT);
		}
		break;

	
	}
	CDialog::OnTimer(nIDEvent);
}

BOOL CScreenEncoderDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.


	switch(wParam)
	{
	case ID_PREVIEW:
		if(lParam)
		{
			this->bPreview = true;
			if(!bStart)
			{
				//sg_Shared.SetRecord(true);
			}
		} else {
			this->bPreview = false;
			if(!bStart)
			{
				//sg_Shared.SetRecord(false);
			}
		}
		return 0;
		break;
	case ID_UPDATE_BITRATE:
		{
			char buffer[255];

			iBitRate = lParam;

			memset(buffer,0x00,sizeof(buffer));
			sprintf_s(buffer,"%d",iBitRate);
			this->M_BitRate.SetWindowTextA(buffer);
			this->m_h263Encoder.SetBItrate(iBitRate );
		
			return 0;
		}
		break;

	case ID_UPDATE_FRAME_RATE:
		{
			char buffer[255];

			m_SCConfig.iFrameRate = lParam;

			memset(buffer,0x00,sizeof(buffer));
			sprintf_s(buffer,"%d",m_SCConfig.iFrameRate);
			this->m_FrameRate.SetWindowTextA(buffer);
		
			return 0;
		}
		break;

	case IDC_UPDATE_SCREEN_SIZE:
		{
			this->m_ScreenSize.SetCurSel(lParam);
			this->OnSelchangeScreensize();
			return 0;		
		}
		break;
	case WCM_UPDATE_CURPOS:
		{

			//::SendMessage(this->m_Display.GetSafeHwnd(), WM_COMMAND,ID_KILLFOCUS,0);
		//	this->m_Display.m_Go.SetFocus();
		//	this->m_Display.m_Go.SetActiveWindow();

			
			CPoint pt;
			CRect rt;

			::GetCursorPos(&pt);
			
		//	this->m_Display.ScreenToClient(&pt);
		//	this->m_Display.GetWindowRect(&rt);

			this->m_MainCtrlPanel.ScreenToClient(&pt);
			this->m_MainCtrlPanel.GetWindowRect(&rt);

			if(rt.PtInRect(pt))
			{

			} else {
				this->m_MainCtrlPanel.KillFocus();
			}
			
			//if((CRect(rt).Width()<pt.x) &)

		
		}
		return 1;
	default:
		break;
	
	}
	return CDialog::OnCommand(wParam, lParam);
}

void CScreenEncoderDlg::OnBnClickedStart()
{

	StartStreaming();
}

void CScreenEncoderDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x,point.y));
	CDialog::OnLButtonDown(nFlags, point);
}
