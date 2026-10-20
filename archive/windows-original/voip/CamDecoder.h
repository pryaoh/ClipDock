// CamDecoder.h: interface for the CCamDecoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAMDECODER_H__DC9B5353_8D72_4431_A854_77312B4EC75E__INCLUDED_)
#define AFX_CAMDECODER_H__DC9B5353_8D72_4431_A854_77312B4EC75E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "VFWLIB/VideoCapture.h"
#include "ijl.h"



class CCamDecoder  
{
public:
	HBITMAP hBmpCutton;
	bool	bShow[5];
	bool	bInit;
	HDC		m_hdc[5];
	HWND	m_hwnd[5];
	CRect	Rect[5];
	HDRAWDIB hdib[5];
	BYTE * m_DIBBuffer;
	JPEG_CORE_PROPERTIES m_DJpeg;	// ¾ÐÃà¿ë
	bool	m_bInitDib;

public:
	void SetView(int i,bool bTrue);
	void Delete(int v);
	void Close();
	void Init(HWND m_hWnd,int v);
	CCamDecoder();
	virtual ~CCamDecoder();
	void JpegDCompress(unsigned int w,unsigned int h,unsigned int JPGSizeBytes,BYTE * m_tmpJPG);
	void DrawImage(int i,BITMAPINFOHEADER *pBmpHeader,int windth,int height,int Size, void *Data);
	void DrawImage1(int i,BITMAPINFOHEADER *pBmpHeader,int windth,int height,int Size, void *Data);
};

#endif // !defined(AFX_CAMDECODER_H__DC9B5353_8D72_4431_A854_77312B4EC75E__INCLUDED_)
