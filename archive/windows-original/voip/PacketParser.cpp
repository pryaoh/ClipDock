#include "StdAfx.h"
#include "PacketParser.h"


CPacket::CPacket(void)
{
}

CPacket::~CPacket(void)
{
}



int CPacket::GetCode()
{ 
	
	char buffer[3];
	int iCode;

	if(ParserData.pData[0] != 0x02 ) 
	{
		return ID_ERR_STX_FIELD_FAIL;
	}
	ParserData.pData++;
	
	if(ParserData.TotalLen>=3)
	{
		memset(buffer,0x00,sizeof(buffer));
		memcpy(buffer,ParserData.pData ,2);
		ParserData.pData += 2;
	} else {
		return ID_ERR_STX_FIELD_FAIL;
	}
	sscanf(buffer,"%d",&iCode);

	return iCode;
}


int CPacket::GetInt()
{
	int ret;

	memcpy(&ret,ParserData.pData,4);
	ParserData.pData += 4;
	return ret;

}
short CPacket::GetShort()
{
	short ret;

	memcpy(&ret,ParserData.pData,2);
	ParserData.pData += 2;

	return ret;

}
unsigned char CPacket::GetChar()
{
	unsigned char ret;

	memcpy(&ret,ParserData.pData,1);
	ParserData.pData += 1;

	return ret;
}

void CPacket::GetString(char * v,int len)
{
	memcpy(v,ParserData.pData,len);
	ParserData.pData += len;
}


void CPacket::SetSendData(void * v)
{
	SendData.iPos = 0;
	SendData.pData = (unsigned char *)v;
	SendData.TotalLen = 0;
}

void CPacket::SetCode(int v)
{
	char buffer[3];
	sprintf(buffer,"%d",v);

	memcpy(&SendData.pData[SendData.iPos],buffer,2);

	SendData.iPos +=2;
	this->SendData.TotalLen +=2 ;
}

void CPacket::SetInt(int v)
{
	memcpy(&SendData.pData[SendData.iPos],&v,4);

	SendData.iPos +=4;
	this->SendData.TotalLen +=4 ;
}

void CPacket::SetShort(short v)
{
	memcpy(&SendData.pData[SendData.iPos],&v,2);

	SendData.iPos +=2;
	this->SendData.TotalLen +=2 ;
}

void CPacket::SetChar(unsigned char  v)
{
	memcpy(&SendData.pData[SendData.iPos],&v,1);
	SendData.iPos +=1;
	this->SendData.TotalLen +=1 ;
}

void CPacket::SetString(char * v,int len)
{
	memcpy(&SendData.pData[SendData.iPos],v,len);
	SendData.iPos +=len;
	this->SendData.TotalLen +=len ;
}

void CPacket::SetCRC(unsigned  char * v,int len)
{
   byte CRC = 0;
   for(int i=0; i<len; i++)
   {     
    CRC ^= v[i];   
   }
   CRC |= 0x20;

	memcpy(&SendData.pData[SendData.iPos],&CRC,1);
	SendData.iPos +=1;
	this->SendData.TotalLen +=1 ;
}

BYTE CPacket::GetCRC(unsigned char * v, int len)
{
   byte CRC = 0;
   for(int i=0; i<len; i++)
   {     
    CRC ^= v[i];   
   }
   CRC |= 0x20;

   return CRC;
}
