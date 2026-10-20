// H263Enc.h: interface for the CH263Enc class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_H263ENC_H__A5B74FA4_A7DA_4B0D_AE96_C955CDD46721__INCLUDED_)
#define AFX_H263ENC_H__A5B74FA4_A7DA_4B0D_AE96_C955CDD46721__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "xvid/xvid.h"

class CH263Enc  
{
private:  //h263
	static  unsigned char  cdata[];
	static int count;
	static int iBufferSize;

	FILE *out_file ;
//	unsigned int *yuv;
public:
	/* Internal structures (handles) for encoding and decoding */
	void *enc_handle ;
	int m4v_size;
	int key;
	int stats_type;
	int stats_quant;
	int stats_length;

	int enc_init(int use_assembler,int keyframe,char * PASS1,char * PASS2);
	int enc_stop();
	int enc_main(unsigned char *image,
					unsigned char *bitstream,
					int *key,
					int *stats_type,
					int *stats_quant,
					int *stats_length,
					int stats[3]);
	void SetBItrate(int iBit);


public:
	unsigned int m_QValue;
	unsigned int m_uiFormat;
	unsigned int m_uiWidth,m_uiHeight;
private:

	static void OwnWriteFunction(int byte);
public:
	void DrawImage(HWND hWnd,HWND hTargetWnd,void * rgbdata,BITMAPINFO * pInfo);
	unsigned int GetCount();
	unsigned char * GetData();
	void Destroy();
	void Compress(void * pData,bool bInter);
	void Init(unsigned int uiWidth,unsigned int uiHeight,int keyframe,char * PASS1 =NULL,char * PASS2=NULL);
	CH263Enc();
	virtual ~CH263Enc();

};

#endif // !defined(AFX_H263ENC_H__A5B74FA4_A7DA_4B0D_AE96_C955CDD46721__INCLUDED_)
