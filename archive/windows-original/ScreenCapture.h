#pragma once
      
#include "imageprocessors.h"
#include "taksi/dll/Taksi_Global.h"



//#define USE_TRAYICON	// Tray icon or not ?

extern bool Test_DirectX8(HWND hWnd );
extern bool Test_DirectX9(HWND hWnd );

extern int DlgHelp( HWND hWnd, const TCHAR* pszMsg, UINT uType = MB_OK );
extern const TCHAR* CheckIntResource( const TCHAR* pszText, TCHAR* pszTmp );
//extern void CheckVideoCodec( HWND hWnd, const ICINFO& info );

extern HINSTANCE g_hInst;	// for the EXE
extern CTaksiConfig g_Config;
extern const TCHAR g_szAppTitle[];

/*
HBITMAP ShrinkBitmap (HBITMAP a,WORD bx,WORD by)
// creates and returns new bitmap with dimensions of
// [bx,by] by shrinking bitmap a both [bx,by] must be less or equal
// than the dims of a, unless the result is nonsense
{
 tWorkBMP in,out;
 HBITMAP b=CreateEmptyBitmap(bx,by);
 OpenBitmapForWork (a,&in);
 ShrinkWorkingBitmap (&in,&out,bx,by);
 free (in.b);
 SaveWorkingBitmap (&out,b);
 free (out.b);
 return (b);
}

*/
class CScreenCapture
{
public:
	
	/************************************************************************/
	/* 스크린 캡쳐를 위한 변수 선언                                         */
	/************************************************************************/
	HDC		hDesktopCompatibleDC;
	HDC		hDesktopDC;
	HWND		hDesktopWnd;

 
	int		ImageTotalLength,iImagePos;
	unsigned char *pBits;
	unsigned char *ScreenImage;
	HBITMAP	hBmpFileBitmap;
	BITMAPINFO	bi;

	int	 nWidth;
	int	 nHeight;
	int	 iWidth;
	int	 iHeight;
	int	 Index,iCount;
	HINSTANCE handle;

	typedef BOOL (WINAPI *tPrintWindow)( HWND, HDC,UINT);
	
	tPrintWindow pPrintWindow;

	/************************************************************************/
	/*스크린 캡쳐를 위한 변수선언 끝                                        */
	/************************************************************************/

public:

	CScreenCapture()
	{
		/************************************************************************/
		/* 스크린 캡쳐 변수 초기화                                              */
		/************************************************************************/

		hDesktopCompatibleDC=NULL;
		hDesktopDC=NULL;
		hDesktopWnd=NULL;

		/************************************************************************/
		/* 스크린 캡쳐 초기화                                                   */
		/************************************************************************/

		ScreenImage=new unsigned char[1600*1200*4];

		handle = ::LoadLibrary("User32.dll");
		/************************************************************************/
		/*스크린 캡쳐 초기화 끝                                                 */
		/************************************************************************/
	}

	~CScreenCapture(void)
	{
		if(hDesktopWnd)
		{
			::ReleaseDC(hDesktopWnd,hDesktopDC);
			DeleteDC(hDesktopCompatibleDC);
			DeleteObject(hBmpFileBitmap);
		}
		::FreeLibrary(handle);
		delete[] ScreenImage;
	}

	void Init(HWND hWnd,unsigned int nWidth1,unsigned int nHeight1)
	{
		//tPrintWindow pPrintWindow = 0;



		if(hDesktopCompatibleDC != NULL)
		{
			DeleteDC(hDesktopCompatibleDC);
			hDesktopCompatibleDC = NULL;
		}


		if(hBmpFileBitmap != NULL)
		{
			DeleteObject(hBmpFileBitmap);
			hBmpFileBitmap = NULL;
		}

		if(hWnd == NULL)
		{
			hDesktopWnd=::GetDesktopWindow();
			nWidth=GetSystemMetrics(SM_CXSCREEN);
			nHeight=GetSystemMetrics(SM_CYSCREEN);
		} else {
			RECT rt;
			hDesktopWnd=hWnd;
			::GetWindowRect(hWnd,&rt);
			nWidth=rt.right - rt.left;
			nHeight=rt.bottom - rt.top;
		}

		hDesktopDC=::GetDC(hDesktopWnd);
		hDesktopCompatibleDC=CreateCompatibleDC(hDesktopDC);

		iWidth=nWidth1;
		iHeight=nHeight1;
		ZeroMemory(&bi,sizeof(bi));
		bi.bmiHeader.biSize=sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth=nWidth;
		bi.bmiHeader.biHeight=nHeight;
		bi.bmiHeader.biPlanes=1;
		bi.bmiHeader.biBitCount=24;
		bi.bmiHeader.biCompression=BI_RGB;
		bi.bmiHeader.biSizeImage=3*bi.bmiHeader.biHeight*bi.bmiHeader.biWidth;

		ImageTotalLength = bi.bmiHeader.biSizeImage;

		hBmpFileBitmap=CreateDIBSection(hDesktopCompatibleDC,&bi,DIB_RGB_COLORS,(void**)&pBits,NULL,0);
		SelectObject(hDesktopCompatibleDC,hBmpFileBitmap);
	}


	/*
void ImageResize(CString str_file, Size new_size)
   {
      Image src_image = Image.FromFile(str_file);
      Bitmap bitmap = new Bitmap(new_size.Width, new_size.Height, 
         src_image.PixelFormat);
      Graphics new_g = Graphics.FromImage(bitmap);
      new_g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.HighQuality;
      new_g.InterpolationMode =
         System.Drawing.Drawing2D.InterpolationMode.HighQualityBicubic;
      new_g.DrawImage(src_image, 0, 0, bitmap.Width, bitmap.Height);
      src_image.Dispose();
      bitmap.Save("ir-" + str_file, System.Drawing.Imaging.ImageFormat.Jpeg);
      bitmap.Dispose();
      new_g.Dispose();
   }
*/


	unsigned char * Capture()
	{
		unsigned char * TempBuffer;
		

		RECT rt;
		::GetWindowRect(hDesktopWnd,&rt);
		nWidth=rt.right - rt.left;
		nHeight=(int)((double)nWidth * 0.75);//rt.bottom - rt.top;

		/*
		nWidth=GetSystemMetrics(SM_CXSCREEN);
		nHeight=GetSystemMetrics(SM_CYSCREEN);
		*/
	
		CSize	SrcSize,DesSize;

		//pPrintWindow(hDesktopWnd, hDesktopCompatibleDC,0 );

		
		if(0 == BitBlt(hDesktopCompatibleDC,0,0,nWidth,nHeight,hDesktopDC,0,0,SRCCOPY))
		{
			return 0;
		
		}
		

		
		/*
		Bitmap src_bitmap(hBmpFileBitmap,NULL);
		//IStream 
		//Image src_image
		Bitmap * m_pBitmap=new Bitmap(100, 100, src_bitmap.GetPixelFormat());
		Graphics * m_New_g = Graphics::FromImage(m_pBitmap);
		m_New_g->SetSmoothingMode(SmoothingMode::SmoothingModeHighSpeed);
		m_New_g->SetInterpolationMode( InterpolationModeBicubic);
		m_New_g->DrawImage(
		*/
		

		//Bitmap m_bitmap(hBmpFileBitmap,NULL);
		//m_bitmap.GetThumbnailImage(
		//	;
       
		SrcSize.cx=nWidth;
		SrcSize.cy=nHeight;

		DesSize.cx=iWidth;
		DesSize.cy=iHeight;


		CImageResizer ImageResizer((double)iHeight/(double)nHeight);
		CImageFlipper ImageFlipper(false,true); 
		ImageResizer.m_dFactor=(double)iWidth/(double)nWidth;


		//*
		TempBuffer=new unsigned char [iWidth*iHeight*4];
		if(ImageResizer.m_dFactor>=1.0)
		{
			ImageResizer.Enlarge((RGBX*)pBits,SrcSize,(RGBX*)TempBuffer,DesSize);
		} else {
			ImageResizer.Shrink((RGBX*)pBits,SrcSize,(RGBX*)TempBuffer,DesSize);
		}

		ImageFlipper.ProcessPixels((RGBX*)TempBuffer,DesSize,(RGBX*)ScreenImage,DesSize);

		delete[] TempBuffer;
		
		return ScreenImage;
	}
};

