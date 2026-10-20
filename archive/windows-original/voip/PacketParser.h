#pragma once
#include "DataStruct.h"
typedef struct _tagPaserData
{
	unsigned char * pData;
	int iPos;
	int TotalLen;
} _ParserData;

/*
typedef struct _tagSendData
{
	char * pData;
	int iPos;
	int TotalLen;
} _SendData;
*/

class CPacket
{
public:
	CPacket(void);
	~CPacket(void);
	_ParserData	SendData;
	_ParserData ParserData;
public:
	int GetCode();
	int GetInt();
	short GetShort();
	unsigned char GetChar();
	void GetString(char * v,int len);

	void SetSendData(void * v);
	void SetCode(int v);
	void SetInt(int v);
	void SetShort(short v);
	void SetChar(unsigned char v);
	void SetString(char * v,int len);
public:
	void SetCRC(unsigned char * v,int len);
public:
	BYTE GetCRC(unsigned char * v, int len);
};
