// CamLib.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include "CamLib.h"
#include "stdafx.h"

#include "camdevice.h"
#include "viewer.h"
#include "SampleGrabberCB.h"

#include "camdevicemanager.h"
#include "viewermanager.h"

typedef void CALLBACK UserDrawImageCallBack(BITMAPINFOHEADER  BitMapHeader,void * Data,unsigned long  DataLen);
typedef UserDrawImageCallBack * pUserDrawImageCallBack;

typedef void CALLBACK UserSocketCallBack();
typedef UserSocketCallBack * pUserSocketCallBack;

#pragma comment(lib,"ijl15l.lib")

HWND m_hWnd1;

pUserSocketCallBack	pUSCB;
pUserDrawImageCallBack pUSGCB;

BYTE * m_lpBuffer=NULL;
BYTE * m_DIBBuffer=NULL;

bool	bLock;
bool	bInit;
bool	bStart;
HDC		m_hdc[5];
CRect	Rect[5];
HDRAWDIB hdib[5];
unsigned int TempSize;
int	JpegSize;


BOOL m_SN_Chk;	//시리어 체크가 돼어있나

HWND m_Device_Wnd;	//캠화면이 보여질 핸들

//--- 영상장비관련
int		m_DeviceNum;	//캠장치 번호
long sec_byte;	//초당 몇 바이트가 전송되나

BITMAPINFOHEADER  m_BitMapHeader;	//현재 캠디바이스에서 나오는 영상의 비트맵 해더
CCamDevice*		m_CamDevice;	//현제 사용하는 디바이스(캠)
CViewer*		m_Viewer;		//캠을 보여줄 뷰어객체(컨트롤)

BYTE * m_VideoBuffer=NULL; //원본 비디오 버퍼
long	m_Video_Len;	//원본 비디오 버퍼 크기

bool isCaptureStarted;
bool isStarted;
bool isVideo;

double FrameRate;

int local_wnd_x , local_wnd_y;
unsigned int *yuv;

JPEG_CORE_PROPERTIES m_Jpeg,m_DJpeg;	// 압축용

#define WM_SAMPLEGRABBER_CB		WM_APP + 2

int iImageCount;

DWORD etime,etime_old;
DWORD FrameTime;
short iFrame;
short iQulity;

bool bCompNow;
bool bUpdated;
DWORD dwTimerID;
HANDLE  Thread_handle;
DWORD Thread_ID;

DWORD WINAPI _stdcall Compress_Process(LPVOID data)
{

	while(1)
	{
		//iFrame=5;
		FrameTime=(DWORD)(1000/iFrame);

		etime_old=::timeGetTime()-etime;

		etime=::timeGetTime();
		if(etime_old < FrameTime)
		{
			etime=FrameTime-etime_old;
			Sleep(etime);
		}

		if(bUpdated)
		{
			int ret=0;
			// Setup DIB
			m_Jpeg.DIBWidth         = m_BitMapHeader.biWidth;
			m_Jpeg.DIBHeight        = m_BitMapHeader.biHeight;
			m_Jpeg.DIBBytes         = m_VideoBuffer;
			
			// Setup JPEG
			m_Jpeg.JPGFile          = NULL;
			m_Jpeg.JPGBytes         = m_lpBuffer;
			m_Jpeg.JPGSizeBytes     = m_BitMapHeader.biWidth * m_BitMapHeader.biHeight *3;
			m_Jpeg.JPGWidth         = m_BitMapHeader.biWidth;
			m_Jpeg.JPGHeight        = m_BitMapHeader.biHeight;
			
			// Set the JPEG image core-structure
			m_Jpeg.DIBChannels    = 3;
			m_Jpeg.DIBColor       =  IJL_BGR;
			m_Jpeg.DIBPadBytes    = IJL_DIB_PAD_BYTES(m_Jpeg.DIBWidth, 3);
			m_Jpeg.JPGColor       = IJL_YCBCR;
			m_Jpeg.JPGChannels    = 3;
			m_Jpeg.JPGSubsampling = IJL_411;// 4:1:1 subsampling

			//--컬리티 추가한거.
			m_Jpeg.jquality=iQulity; // Select "good" image quality. 35~70 사이 설정
			
			if (ijlWrite(&m_Jpeg, IJL_JBUFF_WRITEWHOLEIMAGE) != IJL_OK)
			{
				TRACE("Can't write JPEG image\n");
				AfxThrowUserException();
				return DWORD(29);
			}
			ret=m_Jpeg.JPGSizeBytes;

			pUSGCB(m_BitMapHeader,m_lpBuffer,m_Jpeg.JPGSizeBytes);
			//pUSCB();
			iImageCount=0;
			bUpdated=false;
		}
	}
	//*/
	return DWORD(29);
}

CAMLIB_API void Start()
{
	if(isStarted)
	return;

	m_Viewer->Play();
	bUpdated=false;

	isStarted=TRUE;
	Thread_handle=CreateThread(NULL,				//default security
							0,					//default stack
							Compress_Process,			//use this thread function
							NULL,				//user data sent to thread
							0,					//creation flags, 0=start now
							&Thread_ID);
	
	bCompNow=false;
}

void WINAPI DrawImageData1(void * Data,unsigned long DataLen)
{
	BYTE *pBuffer = (BYTE *)Data;
	unsigned long lBufferLen = (unsigned long)DataLen;

	bUpdated=false;
	memcpy(m_VideoBuffer,pBuffer,lBufferLen);
	m_Video_Len=lBufferLen;
	bUpdated=true;

	return ;		
}

CAMLIB_API void Stop()
{
	if(!isStarted)
		return;

	m_Viewer->Stop();
	::TerminateThread(Thread_handle,NULL);

	isStarted=false;
}

CAMLIB_API void Close()
{
	if(isStarted)
	{
		Sleep(2000);
		isStarted=false;
	}

	if(!bInit)
		return;

	CViewerManager::releaseInstance();
	CCamDeviceManager::releaseInstance();
	
	bInit=false;
}

CAMLIB_API void Init() 
{
	m_VideoBuffer=new BYTE[230401]; //원본 비디오 버퍼
	m_DIBBuffer=new BYTE[230401];
	m_lpBuffer= new BYTE[230401];

}

CAMLIB_API void Destroy() 
{
	if(m_VideoBuffer!=NULL) 
	{
		delete[] m_VideoBuffer;
	}

	if(m_lpBuffer!=NULL) 
	{
		delete[] m_lpBuffer;
	}

	if(m_DIBBuffer!=NULL) 
	{
		delete[] m_DIBBuffer;
	}
}


CAMLIB_API short InitDevice(short sCamFrame,short sCamQulity) 
{
	// 캠장치를 초기화 한다.
	//--- 영상장비를 초기화 해서 객체로 가지고 있는다.
	
	#ifdef _DEBUG
		//return 0;
	#endif
	

	iFrame=sCamFrame;
	iQulity=sCamQulity;
	m_DeviceNum=0;
	sec_byte=0;

	m_Video_Len=0;
	memset(m_VideoBuffer,0,230401);

	if (CCamDeviceManager::getInstance()->Init()==FALSE)	//현재 CAM있을경우 장치들을 모두 객체화 시킨다.
	{
	//	AfxMessageBox("화상 장치가 존재하지 않습니다",MB_OK);
		return 0;
	}

	if(bInit) return 0;

	if(bInit)
	{
		bInit=false;
	}

	CRect rect,brect;
	local_wnd_x=0;//m_pVideo.TopLeft().x+(m_pVideo.Width()-IMAGE_WIDTH)/2;
	local_wnd_y=0;//m_pVideo.TopLeft().y+(m_pVideo.Height()-IMAGE_HEIGHT)/2;

	IJLERR enumJPEG_Err;

	enumJPEG_Err = ijlInit(&m_Jpeg);
	if(IJL_OK != enumJPEG_Err)
	{
		ijlFree(&m_Jpeg);  
		return 0;
	}
	
	if(ijlInit(&m_DJpeg) != IJL_OK )
	{
		ijlFree(&m_DJpeg);
		return 0;
	} 
	memset(m_lpBuffer,0,230401);
	memset(m_DIBBuffer,0,230401);

	pUSCB=NULL;

	bInit=true;

	return (short)CCamDeviceManager::getInstance()->getDeviceCount();
}

CAMLIB_API void GetDeviceName(short m_DevicdNum,char * DeviceName) 
{
	// 캠장치를 초기화 한다.
	//--- 영상장비를 초기화 해서 객체로 가지고 있는다.	
	memcpy(DeviceName,CCamDeviceManager::getInstance()->GetDeviceName(m_DevicdNum).GetBuffer(0),
		CCamDeviceManager::getInstance()->GetDeviceName(m_DevicdNum).GetLength());

	return ;
}

CAMLIB_API BOOL CreateCamObj(HWND hWnd,short DeviceNum, short width, short height,void * pUserCallBack,void * v) 
{
	// -- 특정 영상장비를 생성해서 초기화 한다.
	m_DeviceNum=(int)DeviceNum;

	m_Viewer=CViewerManager::getInstance()->CreateViewer(); //-- 장치의 화상을 나타낼 뷰를 생성한다.
	
	//---맴버변수에 캠디바이스 포인터를 넘겨준다.
	m_CamDevice=CCamDeviceManager::getInstance()->FindDeviceForNum(m_DeviceNum);
	//--샘플갭버에 이밴트를 받을 핸들을 설정한다.
	::SetWindowLong(hWnd,GWL_USERDATA,(long)OnCaptureFrame);

	m_CamDevice->SetCallBack(&DrawImageData1);
	
	pUSGCB=(pUserDrawImageCallBack)pUserCallBack;
	
	m_CamDevice->setSGHwnd(hWnd);
	
	if (m_CamDevice->DeviceON(width,height)==FALSE)
	{	
		//AfxMessageBox("연결실패",MB_OK);
		return FALSE;
	}
	m_BitMapHeader=m_CamDevice->getBitmapHeader();
	
	m_CamDevice->setViewerObj((HWND)hWnd ,m_Viewer);

	pUSCB=(pUserSocketCallBack)v;
	//if( InitIjl()==FALSE) return FALSE;

	return TRUE;
}

CAMLIB_API void SetCallBack(void * v)
{
        
}

