// CamManager.cpp: implementation of the CCamManager class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "CamTest.h"
#include "CamManager.h"
#include ".\cammanager.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib,"ijl15l.lib")

// Global varialbes...
int count=0;

// Callback function gets invoked during compression...
// It returns the compressed data byte by byte...


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

BYTE * CCamManager::m_lpBuffer;
pUserSocketCallBack	CCamManager::pUSCB;
int	CCamManager::JpegSize;
BITMAPINFOHEADER  CCamManager::BitMapHeader;

int iCount=0;




CCamManager::CCamManager()
{
	bInit=false;
	isVideo=false;
	isStarted=false;
	isCaptureStarted=false;
	FrameRate=0;
	phCamMutex=NULL;
	sFrame=15;
	sQulity=60;
	//m_lpBuffer= new BYTE[500000];

	hInst=LoadLibraryA("CamLib.dll");
	
	//CAMLIB_API short InitDevice(short sCamFrame,short sCamQulity) 

	CamStart = (void (*)())GetProcAddress(hInst,"Start");
	CamStop = (void (*)())GetProcAddress(hInst,"Stop");
	CamClose= (void (*)())GetProcAddress(hInst,"Close");
	CamInitDevice=(short(*)(short sCamFrame,short sCamQulity))GetProcAddress(hInst,"InitDevice");
	GetDeviceName=(void(*)(short,char *))GetProcAddress(hInst,"GetDeviceName");
	SetChannel=(bool(*)(int Index))GetProcAddress(hInst,"SetChannel");
	GetChannel=(int(*)())GetProcAddress(hInst,"GetChannel");
	CamCreateCamObj=(bool (*)(HWND hWnd,short DeviceNum, short width, short height,void * pUserCallBack,void * v))GetProcAddress(hInst,"CreateCamObj");
	InitCam=(void (*)())GetProcAddress(hInst,"Init");
	Destroy= (void (*)())GetProcAddress(hInst,"Destroy");

	InitCam();

}



CCamManager::~CCamManager()
{
	Destroy();
	FreeLibrary(hInst);
}

bool CCamManager::Init(HWND m_hWnd)
{
	return true;
}

void CCamManager::Close()
{
	CamClose();
	bInit=false;
}

void CCamManager::Stop()
{
	if(isStarted)
	{
		CamStop();
		isStarted=false;
	}
}

void CCamManager::Start()
{
	if(!bInit) return;

	//if(isStarted)
	//return;

	CamStart();

	isStarted=TRUE;
}
void WINAPI CCamManager::UserSocketCallback()
{
}

void CCamManager::InitCamDevice(HWND m_hWnd,int CamNum)
{
	if(iCamNum>0)
	{
		CamCreateCamObj(m_hWnd,1,320,240,&DrawImageData1,pUSCB);
		bool bTest = SetChannel(11);
		bInit=true;
	} else {
		bInit=false;
	}
}


void WINAPI CCamManager::DrawImageData1(BITMAPINFOHEADER  bi,void * Data,unsigned long DataLen)
{
	//JpegSize=DataLen;
	//BitMapHeader=bi;
	//m_lpBuffer = new BYTE [DataLen] ;
	//memset(m_lpBuffer,0,DataLen);
	//memcpy(m_lpBuffer,Data,DataLen);
	pUSCB(bi,Data,DataLen);
	//delete [] m_lpBuffer;
	
	return ;
}

bool CCamManager::InitDevice()
{
	if(CamInitDevice(sFrame,sQulity)>0)
	{
		return true;
	}	
	return false;
}

bool CCamManager::InitCam1(void)
{
	JpegSize=0;

	iCamNum=CamInitDevice(sFrame,sQulity);

	if(iCamNum>0) return true;

	return false;
}

bool CCamManager::IsCam(void)
{
	HINSTANCE tmpInstance;
	
	tmpInstance=LoadLibrary("CamLib.dll");

	CamStart = (void (*)())GetProcAddress(tmpInstance,"Start");
	CamStop = (void (*)())GetProcAddress(tmpInstance,"Stop");
	CamClose= (void (*)())GetProcAddress(tmpInstance,"Close");
	CamInitDevice=(short(*)(short,short))GetProcAddress(tmpInstance,"InitDevice");
	CamCreateCamObj=(bool (*)(HWND hWnd,short DeviceNum, short width, short height,void * pUserCallBack,void * v))GetProcAddress(tmpInstance,"CreateCamObj");
	SetCallBack=(void (*)(void * v))GetProcAddress(tmpInstance,"SetCallBack");
	GetDeviceName=(void(*)(short,char *))GetProcAddress(tmpInstance,"GetDeviceName");
	iCamNum=CamInitDevice(sFrame,sQulity);
	CamClose();

	FreeLibrary(tmpInstance);
	bInit=false;

	if(iCamNum>0) return true;

	return false;
}

void CCamManager::GetCamDeviceName(short m_DevicdNum,char * DeviceName) 
{
	GetDeviceName(m_DevicdNum,DeviceName);
}