#pragma once
#include "Sock_Dgram.h"
#include "WinSocket.h"
#include "Mmsystem.h"
#include "lame.h"
#include "Waveout.h"
#include <list>
#include "MP3Play.h"
//#include "Com.h"
#include "CommThread.h"
#include "./CamManager.h"
#include "CamDecoder.h"
#include "Ini.h"
#include "ijl.h"
#include "cvidc.h"
#include "H263Dec.h"


#define _CRT_SECURE_NO_DEPRECATE
#define MAXPLAYER 4
#define HOBBY_MODE 1 //취미생활
#define RS232SET				0



typedef struct _tagWorkprocessFlags
{
	CRITICAL_SECTION  cs;

	char * TestFileName;
	FILE * pTempFIle,*pTempFIle1;
	

	bool			bInternalCodec;
	bool			bJpeg;
	bool			bStart;
	bool			bDone;
	int				bUseCurrentMac;
	unsigned int	iWorkStatus;
	unsigned long	LastUpdateTime;
	unsigned long	LastAliveCheckTime;
	unsigned long	LastExtaraDataCheckTime_Manager;
	unsigned long	LastEmergencyCheckTime_Manager;
	unsigned long	LastExtaraDataCheckTime;
	unsigned long	LastEmergencyCheckTime;
	unsigned long	StreamServerStartTime;
	unsigned long	StreamServercCheckTime;
	
	bool				bTargetListUpdated;
	PerSocketContext	TargetList[32];
	int		iTargetNum;

#if HOBBY_MODE
	int  LowCount;
	bool bLow;
	int	 bPhoneMode;
	int  bConferenceMode;
	char TargetName[64];
	char szCurrentMacAddress[256];
	char ServerIP[256];
	char GroupName[256];
	int iPortNum;

	unsigned char Mp3Buffer[1024*32];
	unsigned long iEncodedLen;

#endif

	int bIniLoad;
	int bRelay;
	int bEmergency;
	int bBroadcast;
	int bMulticast;
	int	bAmpMode;
	int iSkipCount;

	unsigned long iLastSendTime;
	bool bServerReady;
	bool bSendAck;
	unsigned int iCurrentPlayLevel;

	ConfigData m_ConfigData;
	ClientInfoHead ClinetInfo;
	StreamDataHead m_StreamDataHead;

	long ScheduledBufferCount;

	//CIni ini;
	int bGroupBroadCast;

}WorkprocessFlags;

class WorkProcess :public WorkprocessFlags
{
public:
	CCamDecoder CDM;
	CH263Dec	m_h263Decoder;
	HWND hWndMain,View1,View2;
	CMP3Play  m_Mp3Play[MAXPLAYER];
	
	int (*InitEnc)(int nSamplesPerSec,int Bitrate,int Quality);
	short (*Encode_Buffers)(DWORD dwParam1,unsigned char * buffer,int * iBufferSize);
	void (*Close)(void);

	WorkProcess(void);
	~WorkProcess(void);
	
	bool m_isRecording;
	lame_global_flags * m_lgf;
	HWAVEIN m_hWaveIn;
	WAVEFORMATEX m_wfe;
	
	CSock_Dgram m_Sock;//m_Sock_Audio,m_Sock_Video;
	CSock_Dgram m_MulticastSock;
	
	JPEG_CORE_PROPERTIES m_Jpeg;

	void PacketParser(Packet * pPacket,bool bTcp = false);
	void StatusOut(int iDest, char *strStat);
	


public:
	HINSTANCE hVideoEncDec;

	void Init();
	void StartStreaming(ClientInfoHead * pClinetInfo = NULL);
	void Reset();
	
	void Run(bool bUDP = false);

	void ConnectServer(char * Address,unsigned int iPortNum);
	void CloseServer();
	void SendMediaData( char * pBuffer,unsigned long iLen,bool bControl = false);

	void (*EncInit)(int CSP,unsigned int Width, unsigned int Height,int KeyFrame,int BItrate);
	void (*EncCompress)(void *pData,bool bInter);
	int  (*GetEncData)(void *pData,int iLen);
	void (*EncStop)();
/*
	void (*DecInit)(unsigned int iWidth, unsigned int iHeight);
	void (*DecDecompress)(unsigned char * pData,int iSize);
	void (*DrawImage)(HWND hWnd,BITMAPINFOHEADER bi);
	void (*DecStop)();
*/
	void (DecInit)(unsigned int iWidth, unsigned int iHeight);
	void (DecDecompress)(unsigned char * pData,int iSize);
	void (DrawImage)(HWND hWnd,BITMAPINFOHEADER bi);
	void (DecStop)();


	void SceneUpdate(BITMAPINFOHEADER  bi,void * lpData,int iLen);

private:

	MicVolume m_curVolume;
	bool m_bComOpen;

	DWORD m_dwMicIndex;
	DWORD m_dwMixIndex;
	DWORD m_dwLineIndex;
	bool m_isMicSel;
	int m_nSSLogin;

	CWaveout m_WavOut;


	CWinSocket	m_WinSock;
	void Destroy();

	HINSTANCE m_hMp3Enc;

	unsigned char MacAddress[6];

	int FindPlayer(char * UserName);

	
	void ConnectStreamServer(char * Address,unsigned int iRecvPortNum,unsigned int iSendPortNum);
	void CloseStreamServer();

	void UpdateTime();
	void UpdateStreamServer();
	void MICVolume(short vol);
	short GetMICVolume();
	void SetInputControl(DWORD dwSelLine);
	BOOL GetSoundInputInfo();
	void SetLineMicOn();

public:
	bool SendStreamData(void);
	WSADATA WSAData;                    // Contains details of the 
										// Winsock implementation
};
