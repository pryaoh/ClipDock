// H263Dec.h: interface for the CH263Dec class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_H263DEC_H__EFED7FCF_C40B_4A50_8661_AAB83C95A7B3__INCLUDED_)
#define AFX_H263DEC_H__EFED7FCF_C40B_4A50_8661_AAB83C95A7B3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "xvid/xvid.h"

class CH263Dec  
{
private:
	static unsigned char rgbdata[];
	int buffersize;
public:
	void *dec_handle ;
	int dec_init(int use_assembler, int debug_level);
	int dec_main(unsigned char *istream, unsigned char *ostream, int istream_size, xvid_dec_stats_t *xvid_dec_stats);
public:
	void Destroy();
	void DrawImage(HWND hWnd,BITMAPINFO * pInfo);
	int Decompress(unsigned char* pData, int size);
	void Init(unsigned int uiWidth,unsigned int uiHeight);
	CH263Dec();
	virtual ~CH263Dec();

};

#endif // !defined(AFX_H263DEC_H__EFED7FCF_C40B_4A50_8661_AAB83C95A7B3__INCLUDED_)
