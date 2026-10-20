////////////////////////////////////////////////////////////////////////////
//
//
//    Project     : VideoNet version 1.1.
//    Description : Peer to Peer Video Conferencing over the LAN.
//	  Author      :	Nagareshwar Y Talekar ( nsry2002@yahoo.co.in)
//    Date        : 15-6-2004.
//
//
//    File description : 
//    Name    :  VideoCapture.cpp
//    Details :  Captures the frames from webcam.
//
//
/////////////////////////////////////////////////////////////////////////////

#include "../Stdafx.h"
#include "VideoCapture.h"
#include "../resource.h"
#include "../CamManager.h"
#include <iostream>

#include <afxmt.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define CAM_TIMER	23123
#pragma comment(lib,"vfw32")
#pragma comment(lib,"winmm")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


VideoCapture::VideoCapture()
{
	m_capwnd=NULL;
	isOver=FALSE;
}





VideoCapture::~VideoCapture()
{
}





void VideoCapture::SetDialog(void *v)
{
	Parent=v;
}


BOOL VideoCapture::Initialize()
{
char devname[100],devversion[100];
int index=0;

m_capwnd = capCreateCaptureWindow("Capture",WS_POPUP,0,0,1,1,0,0);


	if(m_capwnd==NULL)
	{
		return FALSE;
	}
	//connect callback functions
	capSetUserData(m_capwnd,this);
	//Change destroy functions also........

//	CAPDRIVERCAPS ps;
	
	//capSetCallbackOnFrame(m_capwnd,OnCaptureVideo);
	capSetCallbackOnVideoStream(m_capwnd,OnCaptureVideo);
//	capDriverGetCaps(m_capwnd,&ps,sizeof(ps));

	capGetDriverDescription(index,devname,100,devversion,100);
	
	// Connect to webcam driver
	for(int i=0;i<3;i++)
	{
		index=i;
		if(capDriverConnect(m_capwnd,index)) break;
	}


/*
	if( ! capDriverConnect(m_capwnd,index) )
	{
		// Device may be open already or it may not have been
		// closed properly last time.
		AfxMessageBox("Unable to open Video Capture Device");
		m_capwnd=NULL;
		return FALSE;
	}
	*/
	
	// Set the capture parameters

	if(SetCapturePara()==FALSE)
	{
	capDriverDisconnect(m_capwnd);
	return FALSE;
	}

return TRUE;
}


/**
*   Start capturing frames from webcam
*
*/

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	capGrabFrameNoStop(hwnd);
}

BOOL VideoCapture::StartCapture()
{
	// Start live capturing ...
	 /*
	if(capPreview(m_capwnd,true)==FALSE)
	{
		return FALSE;
	}

	TimerID=::SetTimer(m_capwnd,CAM_TIMER,115,(TIMERPROC)TimerProc);
	
return TRUE;
*/

	// Start live capturing ...
	if(capCaptureSequenceNoFile(m_capwnd)==FALSE)
	{
		return FALSE;
	}
	return TRUE;



}


/**
*   Stop the capturing process
*
*/

BOOL VideoCapture::StopCapture()
{

	capCaptureStop(m_capwnd);
	//capCaptureAbort(m_capwnd);
	Sleep(1000);	
	return TRUE;


	/*
	KillTimer(m_capwnd,TimerID);
	if(capPreview(m_capwnd,false)==FALSE)
	{
		return FALSE;
	}
	*/


//	capCaptureStop(m_capwnd);
	
//	capCaptureAbort(m_capwnd);
//	Sleep(1000);	
//return TRUE;
}






/**
*   Stop the catpure process and disconnect the driver
*
*/
void VideoCapture::Destroy()
{

	if(m_capwnd==NULL) return;
    
    // Stop the capturing process    
	capCaptureAbort(m_capwnd);
          
    // Disable the callback function..
	capSetCallbackOnVideoStream(m_capwnd, NULL);
     
	Sleep(300);		// This delay is important...
			
	// Finally disconnect the driver
    capDriverDisconnect(m_capwnd);
	
	DestroyWindow(m_capwnd);
}




/**
*
*     Set various capture parameters...
*
*/

BOOL  VideoCapture::SetCapturePara()
{
	CAPTUREPARMS CapParms={0};	

	capCaptureGetSetup(m_capwnd,&CapParms,sizeof(CapParms));

	CapParms.fAbortLeftMouse = false;
	CapParms.fAbortRightMouse = FALSE;
	CapParms.fYield = TRUE;
	CapParms.fCaptureAudio = FALSE;
	CapParms.dwRequestMicroSecPerFrame=66667;
	CapParms.wPercentDropForError =10;

	if(capCaptureSetSetup(m_capwnd,&CapParms,sizeof(CapParms))==FALSE)
	{
		return FALSE;
	}

	// Set Video Format 

	capGetVideoFormat(m_capwnd,&m_bmpinfo,sizeof(m_bmpinfo));

	m_bmpinfo.bmiHeader.biWidth=IMAGE_WIDTH;
	m_bmpinfo.bmiHeader.biHeight=IMAGE_HEIGHT;

	BOOL ret=capSetVideoFormat(m_capwnd,&m_bmpinfo,sizeof(m_bmpinfo));

	if(ret==TRUE)
	return TRUE;
	else return false;

}

/**
*
*  Allocate Memory for DIB image buffer
* 
*/

int VideoCapture::AllocateMemory(PBITMAPINFO &bmpinfo)
{
int size1,size2,size;
BITMAPINFO tbmp;
char  str[200];

	capGetVideoFormat(m_capwnd,&tbmp,sizeof(tbmp));

	size1 = getFormatSize ( tbmp );
	size2 = getImageSize ( tbmp );
	size = size1 + size2;


	sprintf(str,"\n Formatsize = %d imagesize = %d , fun_size = %d ",
			 size1,size2, capGetVideoFormatSize(m_capwnd));

	bmpinfo=(BITMAPINFO *) new BYTE[size];

	
	if(bmpinfo==NULL)
	{
	AfxMessageBox("Unable to allocate memory");
    return -1;
	}

	memset(bmpinfo,0,sizeof(*bmpinfo));

	capGetVideoFormat(m_capwnd,bmpinfo,sizeof(*bmpinfo));
	return size1;
}

/**
*
*   Calculates the Format Size for DIB image
*
*/

int VideoCapture::getFormatSize(BITMAPINFO bmp)
{
int size;
	size=(bmp.bmiHeader.biSize!=0)?bmp.bmiHeader.biSize :sizeof(BITMAPINFOHEADER);

	//return (size+ bmp.bmiHeader.biClrUsed *sizeof(RGBQUAD));

	return size; //RGBQUAD is absent for 24 bit bmp image.
}






/**
*	Calculates the Size of Image
*
*/

int VideoCapture::getImageSize(BITMAPINFO bmp)
{
int size;
BITMAPINFOHEADER head=bmp.bmiHeader;
	if( head.biSizeImage==0 )
	{
		size=( head.biWidth * head.biHeight * head.biBitCount)/8;
	}
	else
	size = head.biSizeImage;

return size;

}



/**
*    Invoked when the video frame is captured by the driver
*
*
*/

LRESULT CALLBACK OnCaptureVideo(HWND mwnd,LPVIDEOHDR lphdr)
{

	/*
	VideoCapture *vidcap=(VideoCapture *)capGetUserData(mwnd);

	if(vidcap!=NULL )

	((CCamManager*)(vidcap->Parent))->DrawImage(lphdr);
	vidcap=NULL;
	*/

 
return TRUE;

}



