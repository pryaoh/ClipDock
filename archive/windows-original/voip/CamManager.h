// CamManager.h: interface for the CCamManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAMMANAGER_H__C201324A_6581_44F3_AD8B_7619FB0BC507__INCLUDED_)
#define AFX_CAMMANAGER_H__C201324A_6581_44F3_AD8B_7619FB0BC507__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
//#include "VFWLIB/VideoCapture.h"
#include "ijl.h"
#include "global.h"

class CCamManager  //:public CWnd
{
public:
	HINSTANCE hInst;
	int iCamNum;
	short sFrame, sQulity;
	bool	bLock;
	bool	bInit;
	bool	bStart;
	unsigned int TempSize;
	

	bool isCaptureStarted;
	bool isStarted;
	bool isVideo;

	//	VideoCapture *vidcap;
	
	double FrameRate;

	//LPBYTE lpBits;              // memory pointer 
	int local_wnd_x , local_wnd_y;
	unsigned int *yuv;

public:
	HWND m_hWnd1;
	HANDLE *phCamMutex;

	static pUserSocketCallBack	pUSCB;
	static BYTE * m_lpBuffer;
	static int	JpegSize;
	static BITMAPINFOHEADER  BitMapHeader;
	//BYTE * m_DIBBuffer;
	

	JPEG_CORE_PROPERTIES m_Jpeg,m_DJpeg;	// ¾ÐÃà¿ë
private:
	void (*SetCallBack)(void * v);

public:
	bool InitDevice();
	static void WINAPI UserSocketCallback();
	static void WINAPI DrawImageData1(BITMAPINFOHEADER  BitMapHeader,void * Data,unsigned long DataLen);
	void InitCamDevice(HWND m_hWnd,int CamNum);

	void (*CamStop)();
	void (*CamStart)();
	void (*CamClose)();
	short (*CamInitDevice)(short sFrame,short sQulity);
	void (*InitCam)();
	void (*Destroy)();
	bool (*SetChannel)(int Index);
	int  (*GetChannel)();
	void (*GetDeviceName)(short m_DevicdNum,char * DeviceName) ;

	bool (*(CamCreateCamObj))(HWND hWnd,short DeviceNum, short width, short height,void * pUserCallBack,void * v);

	void GetCamDeviceName(short m_DevicdNum,char * DeviceName) ;
	void Stop();
	void Start();
	void Close();
	bool Init(HWND m_hWnd);
	
	CCamManager();
	virtual ~CCamManager();

	bool InitCam1(void);
	bool IsCam(void);
};




#endif // !defined(AFX_CAMMANAGER_H__C201324A_6581_44F3_AD8B_7619FB0BC507__INCLUDED_)
