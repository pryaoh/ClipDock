// CamDecoder.cpp: implementation of the CCamDecoder class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CamDecoder.h"
#include <Wingdi.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#pragma comment(lib,"vfw32")
#pragma comment(lib,"winmm")

CCamDecoder::CCamDecoder()
{
	bInit=false;
	m_DIBBuffer=new BYTE[640*480*4];
	memset(bShow,0x01,sizeof(bShow));
	//hBmpCutton=(HBITMAP)LoadImage(NULL,"images/Back/cutten.bmp",IMAGE_BITMAP,0,0,LR_DEFAULTSIZE|LR_LOADFROMFILE|LR_SHARED); //Exception 惯积

	IJLERR enumJPEG_Err;

	enumJPEG_Err = ijlInit(&m_DJpeg);
	if(IJL_OK != enumJPEG_Err)
	{
		ijlFree(&m_DJpeg);  
		return ;
	}


}

CCamDecoder::~CCamDecoder()
{
    delete[] m_DIBBuffer;
}

void CCamDecoder::JpegDCompress(unsigned int w,unsigned int h,unsigned int JPGSizeBytes,BYTE * m_tmpJPG)
{
	memset(m_DIBBuffer,0x00,640*480*4);
    m_DJpeg.DIBWidth = w  ;
    m_DJpeg.DIBHeight = h ;
    m_DJpeg.DIBBytes = m_DIBBuffer ;

    m_DJpeg.DIBChannels = 3;
	m_DJpeg.DIBColor = IJL_BGR;
    m_DJpeg.DIBPadBytes =IJL_DIB_PAD_BYTES( m_DJpeg.DIBWidth, 3 );

    m_DJpeg.JPGBytes = m_tmpJPG;                            // 傈价等 JPG Data
    m_DJpeg.JPGWidth = w ;
    m_DJpeg.JPGHeight = h;
    m_DJpeg.JPGFile = NULL;

    m_DJpeg.JPGSizeBytes= JPGSizeBytes;      // 傈价等 JPG Data 农扁 
    m_DJpeg.JPGChannels = 3;
    m_DJpeg.JPGColor    = IJL_YCBCR;

    m_DJpeg.JPGSubsampling = IJL_411;        // 4:1:1 subsampling.
    m_DJpeg.jquality       = 70;
 
    if( ijlRead(&m_DJpeg, IJL_JBUFF_READWHOLEIMAGE) != IJL_OK)
    {
        //AfxMessageBox( " Decoding  Fail  ");
		int a=0;
    }
}


void CCamDecoder::DrawImage1(int i,BITMAPINFOHEADER *pBmpHeader,int windth,int height,int Size, void *Data)
{
	static CRect	rect;
	BITMAPINFO bmi;

	if(hdib[i]!=NULL)
	{

		m_hdc[i]=::GetDC(m_hwnd[i]);

		if(!bShow[i]) 
		{
			::GetWindowRect(m_hwnd[i],&rect);
			HBITMAP OldBitmap;
			HDC hMemDC;
			hMemDC=::CreateCompatibleDC(m_hdc[i]);
			//OldBitmap=(HBITMAP)SelectObject(hMemDC,hBmpCutton);
			StretchBlt(m_hdc[i],0,0,rect.Width(),rect.Height(),hMemDC,0,0,320,240,SRCCOPY);
			//SelectObject(hMemDC,OldBitmap);
			::ReleaseDC(m_hwnd[i],hMemDC);
			return;
		}

		if(m_hdc[i]==NULL)
			return;
		//if(!m_bInitDib)
		{
			::DrawDibBegin(hdib[i],
				m_hdc[i],
				-1,						// don't stretch
				-1,						// don't stretch
				pBmpHeader,
				Rect[i].Width(),         // width of image
				Rect[i].Height(),        // height of image
				DDF_SAME_DRAW				
				);
			m_bInitDib=true;
		}

		::GetWindowRect(m_hwnd[i],&Rect[i]);

		//*/

		//JpegDCompress(windth,height,Size,(unsigned  char *)Data);


		::DrawDibDraw(hdib[i],
			m_hdc[i],
			0,		// dest : left pos
			0,		// dest : top pos
			Rect[i].Width(),					 // don't zoom x
			Rect[i].Height(),					 // don't zoom y
			pBmpHeader,					// bmp header info
			Data,					 // bmp data
			0,					 // src :left
			0,					 // src :top
			pBmpHeader->biWidth,				 // src : width
			pBmpHeader->biHeight,				 // src : height
			DDF_SAME_DRAW			 // use prev params....
			);
		::DrawDibEnd(hdib[i]);

		::ReleaseDC(m_hwnd[i],m_hdc[i]);
	}

}

void CCamDecoder::DrawImage(int i,BITMAPINFOHEADER *pBmpHeader,int windth,int height,int Size, void *Data)
{
	static CRect	rect;
	BITMAPINFO bmi;

	if(hdib[i]!=NULL)
	{
	
		m_hdc[i]=::GetDC(m_hwnd[i]);

		if(!bShow[i]) 
		{
			::GetWindowRect(m_hwnd[i],&rect);
			HBITMAP OldBitmap;
			HDC hMemDC;
			hMemDC=::CreateCompatibleDC(m_hdc[i]);
			//OldBitmap=(HBITMAP)SelectObject(hMemDC,hBmpCutton);
			StretchBlt(m_hdc[i],0,0,rect.Width(),rect.Height(),hMemDC,0,0,320,240,SRCCOPY);
			//SelectObject(hMemDC,OldBitmap);
			::ReleaseDC(m_hwnd[i],hMemDC);
			return;
		}

		if(m_hdc[i]==NULL)
			return;
		//if(!m_bInitDib)
		{
			::DrawDibBegin(hdib[i],
						   m_hdc[i],
						   -1,						// don't stretch
						   -1,						// don't stretch
						   pBmpHeader,
						   Rect[i].Width(),         // width of image
						   Rect[i].Height(),        // height of image
						   DDF_SAME_DRAW				
						   );
			m_bInitDib=true;
		}

		::GetWindowRect(m_hwnd[i],&Rect[i]);

	   //*/

		JpegDCompress(windth,height,Size,(unsigned  char *)Data);

/*

	if (
		// Check if CHECKJPEGFORMAT exists:

		(ExtEscape(hdc, QUERYESCSUPPORT,
				   sizeof(ul), &ul, 0, 0) > 0) &&

		// Check if CHECKJPEGFORMAT executed without error:

		(ExtEscape(hdc, CHECKJPEGFORMAT,
				   nJpgImageSize, pvJpgImage, sizeof(ul), &ul) > 0)
	   )

	{
				memset(&bmi, 0, sizeof(bmi));
				bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth       = pBmpHeader->biWidth;
				bmi.bmiHeader.biHeight      = -pBmpHeader->biHeight; // top-down image
				bmi.bmiHeader.biPlanes      = 1;
				bmi.bmiHeader.biBitCount    = 0;
				bmi.bmiHeader.biCompression = BI_JPG;
				bmi.bmiHeader.biSizeImage   = Size;

			//
			// Do the StretchDIBits.
			//

			 StretchDIBits(m_hdc[i],
								 // destination rectangle
							 0, 0, Rect[i].Width(), Rect[i].Height(),
							 // source rectangle
							 0, 0, pBmpHeader->biWidth, pBmpHeader->biHeight,
							 Data,
							 &bmi,
							 DIB_RGB_COLORS,
							 SRCCOPY);
	}

*/
		/*

		bmi.bmiHeader=*pBmpHeader;
		bmi.bmiColors[0].rgbBlue=0;
		bmi.bmiColors[0].rgbGreen=0;
		bmi.bmiColors[0].rgbRed=0;
		bmi.bmiColors[0].rgbReserved=0;

		StretchDIBits( m_hdc[i],
                       0,
                       0,
                       Rect[i].Width(),
                       Rect[i].Height(),
                       0,
                       0,
                       pBmpHeader->biWidth,
                       pBmpHeader->biHeight,
                       m_DIBBuffer,
                       &bmi,
                       DIB_RGB_COLORS,
                       SRCCOPY );
//*/

		TRY
		{
			::DrawDibDraw(hdib[i],
				m_hdc[i],
				0,		// dest : left pos
				0,		// dest : top pos
				Rect[i].Width(),					 // don't zoom x
				Rect[i].Height(),					 // don't zoom y
				pBmpHeader,					// bmp header info
				m_DIBBuffer,					 // bmp data
				0,					 // src :left
				0,					 // src :top
				pBmpHeader->biWidth,				 // src : width
				pBmpHeader->biHeight,				 // src : height
				DDF_SAME_DRAW			 // use prev params....
				);

		}CATCH_ALL(e)
		{
			e->Delete();
		}END_CATCH_ALL
		
		::DrawDibEnd(hdib[i]);

		::ReleaseDC(m_hwnd[i],m_hdc[i]);
	}
}

void CCamDecoder::Init(HWND m_hWnd,int v)
{
	m_hwnd[v]=m_hWnd;

	::GetWindowRect(m_hwnd[v],&Rect[v]);
	memset(m_DIBBuffer,0,640*480*4);

	hdib[v]=::DrawDibOpen();	
	 //DrawDibStart(hdib[v],100);
	 //m_bInitDib=false;
}

void CCamDecoder::Close()
{

}


void CCamDecoder::Delete(int v)
{
	/*
	if(m_bInitDib)
	{
		::DrawDibEnd(hdib[v]);
	}

	DrawDibStop(hdib[v]);
	*/
	::DrawDibClose(hdib[v]);
	

	::ReleaseDC(m_hwnd[v],m_hdc[v]);
}

void CCamDecoder::SetView(int i, bool bTrue)
{
	bShow[i]=bTrue;
}
