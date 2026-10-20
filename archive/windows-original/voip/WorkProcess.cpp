#include "StdAfx.h"
#include "WorkProcess.h"
#include "Mmsystem.h"

#include <rpc.h>
#include <rpcdce.h>

#pragma comment(lib,"Winmm.lib")
#pragma comment(lib,"rpcrt4.lib")

#define NUM_SAMPLES				16000
#define BIT						16
#define CHANEL					2
#define BROADCAST				true

ClientListItem ClientList[50];
unsigned long iClientList;
WorkProcess * pThis;
#pragma warning(disable:4996)
unsigned long CurrentTime ;
unsigned long iAliveCount;
unsigned long AliveCheckClientTime;
#pragma comment(lib,"ijl15l.lib")
#pragma comment(lib,"xvid/libxvidcore.lib")

LONG CALLBACK compressStatusProc(LPARAM lParam,     UINT message, LONG l) 
{     
	char	pStart[] = "Compressing";
	switch (message)     
	{
	case ICSTATUS_START: 
	//	AfxGetMainWnd()->SendMessage(MSG_STARTPROGRESS,(WPARAM)pStart);
		break;
	
	case ICSTATUS_STATUS: 
	//	AfxGetMainWnd()->SendMessage(MSG_SETPROGRESS,l);
		break; 
    case ICSTATUS_END:          
		//AfxGetMainWnd()->SendMessage(MSG_SETPROGRESS,100);
		break; 
	case ICSTATUS_YIELD:
		break; 
    }
	return 0; 
} 


bool MacCompare(unsigned char Mac1[6],unsigned char Mac2[6])
{

	for(int i=0;i<6;i++)
	{
		if(Mac1[i] != Mac2[i])
		{
			return false;
		}
	}
	return true;
}


void  GetFilePath(char * fn,char * path)
{
	static char * pCur;
	static unsigned int size=0;
	static unsigned int tsize;
	pCur=fn;
	while(1)
	{
		pCur=strchr(pCur,'\\');
		if (pCur==NULL)
		{
			if (size==0) return;
			memset(path,0x00,size*sizeof(TCHAR)+1);
			memcpy(path,fn,size*sizeof(TCHAR));
			return;
		}
		else
		{
			pCur++;
			size=(unsigned int)(pCur-fn);
		}
	}
};
WorkProcess::WorkProcess(void)
{

//	_CrtSetBreakAlloc(641);
	
//	iSkipCount = 50;


	bInternalCodec = true;
	bJpeg = false;
	hWndMain = NULL;
	m_hWaveIn = NULL;
	bDone = false;
	bRelay=false;
	bMulticast = false;
	bEmergency = false;
	bConferenceMode = false;
	bGroupBroadCast = 0;

	iEncodedLen = 0;

	this->bBroadcast = false;
	
//	m_Sock_Audio.m_pWorkProcess = this;
//	m_Sock_Video.m_pWorkProcess = this;
	m_Sock.m_pWorkProcess = this;
	m_MulticastSock.m_pWorkProcess = this;

	m_hMp3Enc=::LoadLibraryA(("libMp3Enc.dll"));

	InitEnc = (int(*)(int,int ,int ))GetProcAddress(m_hMp3Enc,"InitEnc");
	Encode_Buffers = (short(*)(DWORD ,unsigned char * ,int * ))GetProcAddress(m_hMp3Enc,"Encode_Buffers");
	Close = (void(*)(void))GetProcAddress(m_hMp3Enc,"Close");
	
	InitializeCriticalSection ( &cs );

	LastEmergencyCheckTime = timeGetTime();
	LastExtaraDataCheckTime = timeGetTime();
	LastEmergencyCheckTime_Manager = timeGetTime();
	LastExtaraDataCheckTime_Manager = timeGetTime();
	iLastSendTime = timeGetTime();
	StreamServercCheckTime = timeGetTime();
	memset(&m_ConfigData,0x00,sizeof(m_ConfigData));
	iWorkStatus = CLIENT_HALT;


	char TempString[100];
	char modename[1024];

	memset(modename,0x00,sizeof(modename));
	
	::GetCurrentDirectoryA(1024,modename);
/*
	TCHAR FilePath[1024];
	memset(FilePath,0x00,sizeof(FilePath));
	::GetModuleFileName(NULL,modename,1024);
	GetFilePath(modename,FilePath);
*/
	strcat(modename,("\\Config.ini"));
	char TempString1[256];

	//GetPrivateProfileInt(
	GetPrivateProfileStringA(("SETTING"),("MACADDRESS"),"",TempString1,256,modename);

	memset(TempString,0x00,sizeof(TempString));
	strcpy(TempString,TempString1);
	//wcstombs(TempString,TempString1,sizeof(TempString));

	sscanf(TempString,("%x-%x-%x-%x-%x-%x"),&MacAddress[0],&MacAddress[1],&MacAddress[2],&MacAddress[3],&MacAddress[4],&MacAddress[5]);
	memcpy(ClinetInfo.MacAddress,MacAddress,6);
	memcpy(m_curVolume.MacAddress,MacAddress,6);


	bPhoneMode		= GetPrivateProfileIntA(("CALL_MODE"),("CALL_MODE_ON"),0,modename);
	bConferenceMode = GetPrivateProfileIntA(("CALL_MODE"),("CONFERENCE_MODE_ON"),0,modename);
	
	memset(TargetName,0x00,sizeof(TargetName));
	GetPrivateProfileStringA(("CALL_MODE"),("TARGET_NAME"),"",TempString1,256,modename);
	//wcstombs(TargetName,TempString1,sizeof(TargetName));
	strcpy(TargetName,TempString1);

	GetPrivateProfileStringA(("CALL_MODE"),("USER_NAME"),"",TempString1,256,modename);
	//wcstombs(ClinetInfo.UserName,TempString1,sizeof(ClinetInfo.UserName));
	strcpy(ClinetInfo.UserName,TempString1);

	bAmpMode = GetPrivateProfileIntA(("TEST"),("AMPMODE"),0,modename);

	memset(szCurrentMacAddress,0x00,sizeof(szCurrentMacAddress));

	GetPrivateProfileStringA(("SETTING"),("SERVER_IP"),"",TempString1,256,modename);
	memset(ServerIP,0x00,sizeof(ServerIP));
	//wcstombs(ServerIP,TempString1,sizeof(ServerIP));
	strcpy(ServerIP,TempString1);

	iPortNum	= GetPrivateProfileIntA(("SETTING"),("PORT_NUM"),0,modename);
	bBroadcast	= GetPrivateProfileIntA(("TEST"),("BROADCAST"),0,modename);
	bMulticast	= GetPrivateProfileIntA(("TEST"),("MULTICAST"),0,modename);
	bMulticast	= GetPrivateProfileIntA(("TEST"),("RELAYSERVER"),0,modename);

	m_ConfigData.GroupID = GetPrivateProfileIntA(("TEST"),("GROUP_ID"),0,modename);
	m_ConfigData.Chanel_1 = m_ConfigData.GroupID;
	bUseCurrentMac = GetPrivateProfileIntA(("SETTING"),("USE_CURRENT_MAC"),1,modename);

	if(bUseCurrentMac)
	{
		UUID uuid;
		//UuidCreateSequential( &uuid );    // Ask OS to create UUID
		UuidCreate( &uuid );    // Ask OS to create UUID
		for (int i=2; i<8; i++)  // Bytes 2 through 7 inclusive are  address
			MacAddress[i - 2] = uuid.Data4[i];
	}
	memcpy(ClinetInfo.MacAddress,MacAddress,6);
	memcpy(m_curVolume.MacAddress,MacAddress,6);

	m_WavOut.Set_WIN_Params(NUM_SAMPLES,16,2);
	this->LowCount = 0;
	iClientList = 0;

	iWorkStatus = CLIENT_HALT;


	for(int i=0;i<MAXPLAYER ;i++)
	{
		m_Mp3Play[i].Start((int)((double)NUM_SAMPLES),BIT,CHANEL);
	}

	pThis = this;
	Reset();

	
#if RS232SET
	//com,bps,data,parity,stop
	if(m_ComuPort.OpenPort(_T("COM1:"),9600,8,0,0)){//CreateCommInfo()){
		m_bComOpen = true;
	}
	else{
		m_bComOpen = false;
	}

#endif


}

WorkProcess::~WorkProcess(void)
{

	Destroy();
	DeleteCriticalSection ( &cs );

	char modename[1024];
	memset(modename,0x00,sizeof(modename));
	::GetCurrentDirectoryA(1024,modename);
	strcat(modename,("\\Config.ini"));

	FreeLibrary(hVideoEncDec);
	//ini.Write(modename);


#if RS232SET
	if(m_bComOpen == true){
		m_bComOpen = false;
		m_ComuPort.ClosePort();
	}
#endif
}

void WorkProcess::Destroy()
{
	bDone = true;

	if (m_hWaveIn != 0)
	{
		waveInStop(m_hWaveIn);
		waveInReset(m_hWaveIn);
		waveInClose(m_hWaveIn);
		m_hWaveIn = 0;
	}

	CloseServer();

	WSACleanup(); //kimm 07.03/

	

	Sleep(1000);

	for(int k=0;k<MAXPLAYER ;k++)
	{
		m_Mp3Play[k].bDone = true;
	}

	
	
///*
//	if(NULL != m_fs)
//		fclose(m_fs);
//*/
}

void WorkProcess::DecInit(unsigned int iWidth, unsigned int iHeight)
{
	m_h263Decoder.Init(iWidth,iHeight);
	
};
void WorkProcess::DecDecompress(unsigned char * pData,int iSize)
{
	

	m_h263Decoder.Decompress(pData,iSize);

	
};

void WorkProcess::DrawImage(HWND hWnd,BITMAPINFOHEADER bi)
{
	BITMAPINFO bmi;
	memset(&bmi,0x00,sizeof(BITMAPINFO));
	bmi.bmiHeader = bi;
	m_h263Decoder.DrawImage(hWnd,&bmi);
}
void WorkProcess::DecStop()
{
	m_h263Decoder.Destroy();
};

void WorkProcess::StatusOut(int iDest, char *strStat)
{
	if(hWndMain != NULL)
	{
		HDC hdc;
		hdc=GetDC(hWndMain);

		TextOutA(hdc,10,15*iDest,strStat,(int)strlen(strStat));
	//		TextOut(hdc,10,15,strStat,wcslen(strStat));

		ReleaseDC(hWndMain, hdc);
		UpdateWindow(hWndMain);
	}
}

void WorkProcess::Run(bool bUDP )
{
	bStart = true;

	//iWorkStatus = CLIENT_WAIT;
	
	
	while(!bDone)
	{
		CurrentTime = timeGetTime() ;
		switch(this->iWorkStatus)
		{
		case CLIENT_DESTROY:
			{
				this->Destroy();				
				exit(0);
			}
		case CLIENT_WAIT:
			{
				Sleep(100);
				//CloseStreamServer();
			}
			break;

		case CLIENT_HALT:
			{
				//StatusOut(FALSE, ("Status:CLIENT_HALT"));

				iWorkStatus = CLIENT_WAIT;
			}
			break;
	
		case CLIENT_DSOCK_INIT:
			{
				LastAliveCheckTime = CurrentTime;

				
				ConnectStreamServer((char *)this->m_ConfigData.StreamServerAddress,m_ConfigData.SendPortNum,m_ConfigData.SendPortNum);
				memset(Mp3Buffer,0x00,sizeof(Mp3Buffer));
				iEncodedLen = 0;
				AliveCheckClientTime = CurrentTime;

			}
			break;
		case CLIENT_STREAM_SERVER_LOGIN:
			{
				iEncodedLen = 0;
				ScheduledBufferCount =0;
				if( 5000 <=  (timeGetTime() - LastUpdateTime))
				{
					iWorkStatus = CLIENT_DSOCK_INIT;
					LastUpdateTime = timeGetTime() ;
				}
			}
			break;

		case CLIENT_STREAM_SERVER_LOGIN_OK:
			{
				StatusOut(0, ("Status:CLIENT_STREAM_SERVER_LOGIN_OK"));

				UpdateStreamServer();

				if( iAliveCount  > 3000 )
				{
					StatusOut(0, ("Status:CLIENT_STREAM_SERVER_LOGIN_OK => CLIENT_DSOCK_INIT"));
					CloseStreamServer();
					iWorkStatus = CLIENT_DSOCK_INIT;
				}

			}
			break;

		default:
			break;
		}
			m_Sock.UpdatePacket();
		Sleep(1);	
	}
}

void WorkProcess::UpdateStreamServer()
{
	unsigned int i;
	unsigned short level;
	level = 0;

	CurrentTime= timeGetTime();
	if( (10000 <=  (CurrentTime - LastUpdateTime)) )
	{
		Packet packet;
		packet.id(ALIVE_CHECK);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		this->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());

		packet.clear();
		packet.id(GROUP_LIST_UPDATE_REQ);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
		iAliveCount++;
		LastUpdateTime = CurrentTime;
	}


	if( (1000 <=  (CurrentTime - AliveCheckClientTime)) )
	{
		Packet packet;
		packet.id(ALIVE_CHECK_CLIENT_REQ);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		this->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());

		for( i=0;i<iClientList;i++ )
		{
			if(!MacCompare(ClientList[i].SocketContext.ClientData.MacAddress,ClinetInfo.MacAddress))
			{
				m_Sock.SendTo(&ClientList[i].SocketContext.socket_control,(char *)packet.getPacketBuffer(),packet.getPacketSize());
				//Sleep(10);
				//m_Sock_Audio.SendTo(&ClientList[i].SocketContext.socket_audio,(char *)packet.getPacketBuffer(),packet.getPacketSize());
				//Sleep(10);
				//m_Sock_Video.SendTo(&ClientList[i].SocketContext.socket_video,(char *)packet.getPacketBuffer(),packet.getPacketSize());
			}
			
		}
		AliveCheckClientTime = CurrentTime;
	}
}

void CALLBACK waveInProc(
  HWAVEIN hwi,       
  UINT uMsg,         
  DWORD dwInstance,  
  DWORD dwParam1,    
  DWORD dwParam2     
)
{
	WorkProcess * pWorkProcess;
	pWorkProcess =(WorkProcess *) dwInstance;
	
	if(pWorkProcess->bDone)
	{
		return ;
	}
/*
	pWorkProcess->bServerReady = true;
	switch(uMsg)
	{
	case WIM_DATA:
		{
			WAVEHDR * wh = (WAVEHDR *)dwParam1;
			unsigned long num_samples = wh->dwBytesRecorded / 8;

#if HOBBY_MODE
			if(pWorkProcess->bConferenceMode)
			{

				unsigned int i;
				int mp3buffer_size = (unsigned long)(1.25 * (double)num_samples) + 7200;		
				unsigned char * mp3buffer = new unsigned char[mp3buffer_size];

				unsigned int level = pWorkProcess->Encode_Buffers(dwParam1,mp3buffer,&mp3buffer_size);
				unsigned short sSize=mp3buffer_size;


				if(level <400)
				{
					pWorkProcess->LowCount ++;
				} else {
					pWorkProcess->LowCount = 0;
				}

				if( pWorkProcess->bServerReady)
				{
					pWorkProcess->iLastSendTime = timeGetTime();
					

					if(pWorkProcess->LowCount <10)
					{
						EnterCriticalSection (&pWorkProcess->cs );
						if(iClientList>1)
						{

							Packet packet;

							packet.id(RECV_STREAM_DATA_FROM_SERVER);
							pWorkProcess->m_StreamDataHead.ChanelID = pWorkProcess->ClinetInfo.Chanel_1;
							strcpy((char *)pWorkProcess->m_StreamDataHead.SourceName,pWorkProcess->szCurrentMacAddress);
							strcpy((char *)pWorkProcess->m_StreamDataHead.TargetName,pWorkProcess->TargetName);
							pWorkProcess->m_StreamDataHead.DataType = NORMAL_STREAM_DATA;
							pWorkProcess->m_StreamDataHead.lTimeStemp = timeGetTime();

							packet.writeData2(&pWorkProcess->m_StreamDataHead,sizeof(StreamDataHead));
							packet.writeData2(mp3buffer,sSize);
							packet.writeData2(&level,sizeof(level));

							SOCKADDR_IN * pTarget_sin = new SOCKADDR_IN[iClientList];
							SOCKADDR_IN * pTemp_sin  = pTarget_sin;
							for(i=0;i<iClientList;i++ )
							{
								if(!MacCompare(ClientList[i].SocketContext.ClientData.MacAddress,pWorkProcess->ClinetInfo.MacAddress))
								{
									//*(pTemp_sin++) = ClientList[i].SocketContext.socket;
									pWorkProcess->m_Sock.SendTo(&ClientList[i].SocketContext.socket_control,packet.getPacketBuffer(),packet.getPacketSize());
								}
							}
							//pWorkProcess->m_Sock.AddSendPacket(packet.getPacketBuffer(),packet.getPacketSize(),pTarget_sin,iClientList-1);
							delete [] pTarget_sin;
						}
						LeaveCriticalSection ( &pWorkProcess->cs );
					}

					pWorkProcess->bServerReady = false;
				}

				delete [] mp3buffer;
			} else if(pWorkProcess->bPhoneMode) { 
				int mp3buffer_size = (unsigned long)(1.25 * (double)num_samples) + 7200;		
				unsigned char * mp3buffer = new unsigned char[mp3buffer_size];

				unsigned short level = pWorkProcess->Encode_Buffers(dwParam1,mp3buffer,&mp3buffer_size);
				unsigned short  sSize=mp3buffer_size;

				Packet packet;
				packet.id(SEND_STREAM_DATA_BY_NAME);
				pWorkProcess->m_StreamDataHead.GroupID = pWorkProcess->ClinetInfo.GroupID;
				strcpy((char *)pWorkProcess->m_StreamDataHead.SourceName,pWorkProcess->ClinetInfo.UserName);
				strcpy((char *)pWorkProcess->m_StreamDataHead.TargetName,pWorkProcess->TargetName);
				pWorkProcess->m_StreamDataHead.DataType = NORMAL_STREAM_DATA;
				pWorkProcess->m_StreamDataHead.lTimeStemp = timeGetTime();
				packet.writeData2(&pWorkProcess->m_StreamDataHead,sizeof(StreamDataHead));
				packet.writeData2(mp3buffer,sSize);
				packet.writeData2(&level,sizeof(level));

				pWorkProcess->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
				delete [] mp3buffer;

			}else {
#endif
			if (pWorkProcess->bBroadcast || pWorkProcess-> bEmergency||pWorkProcess->bAmpMode ||pWorkProcess->bGroupBroadCast)
			{
				int mp3buffer_size = (unsigned long)(1.25 * (double)num_samples) + 7200;		
				unsigned char * mp3buffer = new unsigned char[mp3buffer_size];

				pWorkProcess->m_StreamDataHead.lTimeStemp = timeGetTime();
				unsigned short level = pWorkProcess->Encode_Buffers(dwParam1,mp3buffer,&mp3buffer_size);
				unsigned short sSize=mp3buffer_size;

				//////////////////////////////////////////////////////////////////////////
//				if( pWorkProcess->m_fs!= NULL )
//				{
//					fwrite(mp3buffer, 1, mp3buffer_size, pWorkProcess->m_fs );
//				}
				//////////////////////////////////////////////////////////////////////////
				if(pWorkProcess->bAmpMode) {
					//pWorkProcess->m_WavOut.WIN_Play_Samples(wh->lpData,wh->dwBytesRecorded);
					pWorkProcess->m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
				
				} else if(pWorkProcess->bBroadcast)
				{
					unsigned int i;

					if(level <300)
					{
						pWorkProcess->LowCount ++;
					} else {
						pWorkProcess->LowCount = 0;
					}

					Packet packet;
					//packet.id(SEND_STREAM_DATA_TO_SERVER);
					packet.id(RECV_STREAM_DATA_FROM_SERVER);
					pWorkProcess->m_StreamDataHead.DataType = NORMAL_STREAM_DATA;
					pWorkProcess->m_StreamDataHead.GroupID = 0; //do not use
					pWorkProcess->m_StreamDataHead.ChanelID = pWorkProcess->ClinetInfo.Chanel_1; //kimm.0628.set channelID
					strcpy((char *)pWorkProcess->m_StreamDataHead.SourceName,pWorkProcess->ClinetInfo.UserName);

					packet.writeData2(&pWorkProcess->m_StreamDataHead,sizeof(StreamDataHead));
					packet.writeData2(mp3buffer,sSize);
					packet.writeData2(&level,sizeof(level));
					//pWorkProcess->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());

					//EnterCriticalSection ( &pWorkProcess->cs );
					if(pWorkProcess->LowCount < 2)
					{
						for(i=0;i<iClientList;i++ )
						{
							pWorkProcess->m_Sock.SendTo(&ClientList[i].SocketContext.socket_control,(char *)packet.getPacketBuffer(),packet.getPacketSize());
						}
					}
					//LeaveCriticalSection ( &pWorkProcess->cs );

				} else if(pWorkProcess->bEmergency && (!pWorkProcess->bRelay)) {

					Packet packet;
					packet.id(SEND_STREAM_DATA_TO_SERVER);
					pWorkProcess->m_StreamDataHead.DataType = EMERGENCY_STREAM_DATA;
					pWorkProcess->m_StreamDataHead.GroupID = pWorkProcess->ClinetInfo.GroupID;
					pWorkProcess->m_StreamDataHead.ChanelID = pWorkProcess->ClinetInfo.Chanel_1; //kimm.0628.set channelID
					strcpy((char *)pWorkProcess->m_StreamDataHead.SourceName,pWorkProcess->ClinetInfo.UserName);

					packet.writeData2(&pWorkProcess->m_StreamDataHead,sizeof(StreamDataHead));
					packet.writeData2(mp3buffer,sSize);
					packet.writeData2(&level,sizeof(level));

					pWorkProcess->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
				}  else if(pWorkProcess->bGroupBroadCast && (!pWorkProcess->bRelay)) {

					Packet packet;
					packet.id(SEND_STREAM_DATA_TO_SERVER);
					pWorkProcess->m_StreamDataHead.DataType = EXTRA_STREAM_DATA;
					pWorkProcess->m_StreamDataHead.GroupID = pWorkProcess->ClinetInfo.GroupID;
					pWorkProcess->m_StreamDataHead.ChanelID = pWorkProcess->ClinetInfo.Chanel_1; //kimm.0628.set channelID
					strcpy((char *)pWorkProcess->m_StreamDataHead.SourceName,pWorkProcess->ClinetInfo.UserName);

					packet.writeData2(&pWorkProcess->m_StreamDataHead,sizeof(StreamDataHead));
					packet.writeData2(mp3buffer,sSize);
					packet.writeData2(&level,sizeof(level));

					pWorkProcess->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
				} 

				delete [] mp3buffer;

			}
#if HOBBY_MODE

			}
#endif
			waveInAddBuffer(pWorkProcess->m_hWaveIn,wh,sizeof(WAVEHDR));
		}

		break;
	default:
		break;
	}
	*/
}


void WorkProcess::Init()
{
	UpdateTime();

	ClinetInfo.ClientID = CLIENTID;
	ClinetInfo.ClinetType = SOURCE_CLIENT;
	ClinetInfo.Chanel_1 = m_ConfigData.Chanel_1;
	ClinetInfo.Chanel_2 = m_ConfigData.Chanel_2;
	ClinetInfo.GroupID = m_ConfigData.GroupID;
	
#ifdef WINVER
	/*
	ini.GetValue(_T("SETTING"),_T("USE_CURRENT_MAC"),bUseCurrentMac);
	if(bUseCurrentMac)
	{
		UUID uuid;
		//UuidCreateSequential( &uuid );    // Ask OS to create UUID
		UuidCreate( &uuid );    // Ask OS to create UUID
		for (int i=2; i<8; i++)  // Bytes 2 through 7 inclusive are  address
			MacAddress[i - 2] = uuid.Data4[i];
	}
	memcpy(DataHead.MacAddress,MacAddress,6);
	memcpy(m_curVolume.MacAddress,MacAddress,6);
	*/
#endif

	hVideoEncDec=LoadLibraryA("VideoEncDec.dll");
	
	//CAMLIB_API short InitDevice(short sCamFrame,short sCamQulity) 

	EncInit = (void (*)(int CSP,unsigned int Width, unsigned int Height,int KeyFrame,int BItrate))GetProcAddress(hVideoEncDec,"EncInit");
	EncCompress = (void (*)(void *pData,bool bInter))GetProcAddress(hVideoEncDec,"EncCompress");
	GetEncData= (int (*)(void *pData,int iLen))GetProcAddress(hVideoEncDec,"GetEncData");
	EncStop=(void(*)())GetProcAddress(hVideoEncDec,"EncStop");
/*
	DecInit=(void(*)(unsigned int iWidth, unsigned int iHeight))GetProcAddress(hVideoEncDec,"DecInit");
	DecDecompress=(void (*)(unsigned char * pData,int iSize))GetProcAddress(hVideoEncDec,"DecDecompress");
	DrawImage=(void (*)(HWND hWnd,BITMAPINFOHEADER bi))GetProcAddress(hVideoEncDec,"DrawImage");
	DecStop= (void (*)())GetProcAddress(hVideoEncDec,"DecStop");
*/
	GetSoundInputInfo(); //kimm.06.20 //get sound mixer information for input switching 
	SetInputControl(m_dwMicIndex);


	if(IJL_OK != ijlInit(&m_Jpeg))
	{
		ijlFree(&m_Jpeg);  
	}



	this->m_WinSock.m_pWorkProcess = this;	

}

void WorkProcess::Reset()
{
//	SetLineMicOn();
	ScheduledBufferCount =0;
	m_nSSLogin = 0;
	m_hWaveIn = NULL;

	bDone  = true;
	BOOL result = FALSE;
	UINT numDevs = waveInGetNumDevs();
	UINT m_inputDeviceID = 0;
	
	WAVEINCAPS wic;
	if (waveInGetDevCaps(m_inputDeviceID,&wic,sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR)
	{
	}

	UINT num = mixerGetNumDevs();  

	ZeroMemory(&m_wfe,sizeof(WAVEFORMATEX));

	m_wfe.wFormatTag = WAVE_FORMAT_PCM;
	m_wfe.nChannels = CHANEL;
	m_wfe.nSamplesPerSec = NUM_SAMPLES;
	m_wfe.wBitsPerSample = BIT;
	//m_wfe.nBlockAlign = m_wfe.nChannels * (m_wfe.wBitsPerSample / 8);
	m_wfe.nBlockAlign = m_wfe.nChannels * (m_wfe.wBitsPerSample /4);
	m_wfe.nAvgBytesPerSec = m_wfe.nSamplesPerSec * m_wfe.nBlockAlign;
/*
	if (waveInOpen(&m_hWaveIn,m_inputDeviceID,&m_wfe,(DWORD)(VOID*)waveInProc,(DWORD)(void *)this,CALLBACK_FUNCTION) != MMSYSERR_NOERROR)
	{
	}
	else
	{
		const long numBuffers = 64;
		const long bufferSizeSeconds = 1;
		const long bufferSize = (long)((double)m_wfe.nSamplesPerSec/16.0) * (m_wfe.wBitsPerSample/8 ) * m_wfe.nChannels;

		result = TRUE;

		for (long x = 0; x < numBuffers; x++)
		{
			char * bufData = new char[bufferSize];
			if (bufData != 0)
			{
				ZeroMemory(bufData,bufferSize);

				WAVEHDR * bufHdr = new WAVEHDR;
				if (bufHdr != 0)
				{

					ZeroMemory(bufHdr,sizeof(WAVEHDR));
					bufHdr->lpData = bufData;
					bufHdr->dwBufferLength = bufferSize;

					if (waveInPrepareHeader(m_hWaveIn,bufHdr,sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
					{

						result = FALSE;
					}
					else
					{
						if (waveInAddBuffer(m_hWaveIn,bufHdr,sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
						{
							
							result = FALSE;

						}
					}
				}
			}
		}
		if (!result)
		{

		}
		else
		{
			if (waveInStart(m_hWaveIn) != MMSYSERR_NOERROR)
			{
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}
		}
	}
	*/
	InitEnc((int)((double)m_wfe.nSamplesPerSec),256,9);


	TCHAR szError[100];   
	if (WSAStartup (MAKEWORD(2,3), &WSAData) != 0) 
	{
		wsprintf (szError, TEXT("WSAStartup failed! Error: %d"), WSAGetLastError ());
		MessageBox (NULL, szError, TEXT("Error"), MB_OK);
	}

	bDone = false; 
}

void WorkProcess::UpdateTime()
{
	//LastUpdateTime =  ::timeGetTime();
}


void WorkProcess::ConnectServer(char * Address,unsigned int iPortNum)
{

	m_WinSock.ConnectServer(Address,iPortNum);

	Sleep(100);
//	m_Sock_Audio.hWndMain = this->hWndMain;
//	m_Sock_Video.hWndMain = this->hWndMain;
	m_Sock.hWndMain = this->hWndMain;
//	m_Sock_Video.Init();
//	m_Sock_Audio.Init();
	m_Sock.Init();
//	m_MediaSock.Init();
	
	if(bMulticast)
	{
		m_MulticastSock.bMultiCast = true;//(bool)bMulticast;
		m_MulticastSock.Init();
	}
//	m_Sock_Video.SetReceiver("127.0.0.1",0);
//	m_Sock_Audio.SetReceiver("127.0.0.1",0);
	m_Sock.SetReceiver("127.0.0.1",0);

	//*
	CDM.Init(this->View2,0);
	CDM.SetView(0,true);


	//*/
}

void WorkProcess::SceneUpdate(BITMAPINFOHEADER  bi,void * lpData,int iLen)
{
	static int TotalSize = 0;
	
	Packet packet;
	packet.id(MEDIA_STREAM_MPEG4_DATA);
	packet.writeData2(&bi,sizeof(BITMAPINFOHEADER));
	packet.writeData2(lpData,iLen);
	pThis->SendMediaData(packet.getPacketBuffer(),packet.getPacketSize());
	TotalSize+=iLen;


	char strOut[100];
	sprintf_s(strOut, ("Encoded Total Size:%d k"),TotalSize/1024);
	pThis->StatusOut(8,strOut);

}

void WorkProcess::StartStreaming(ClientInfoHead * pClientInfo)
{
	if(pClientInfo)
	{
		memcpy(&pClientInfo,pClientInfo,sizeof(ClientInfoHead));
	}

	Packet packet;
	packet.id(LOGIN_REQ);
	packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
	m_WinSock.SendPacket(&packet);
}

void WorkProcess::ConnectStreamServer(
									  char * Address,
									  unsigned int iRecvPortNum,
									  unsigned int iSendPortNum )
{

	memset(ClientList,0x00,sizeof(ClientList));


	//if(!m_Sock.Sock == INVALID_SOCKET)z
	{
	
	//	m_MediaSock.SetReceiver("127.0.0.1",iRecvPortNum+2);
//
//		m_Sock_Video.InitSockList();
//		m_Sock_Audio.InitSockList();

//		m_Sock_Video.SetSender(Address,iSendPortNum);
//		m_Sock_Audio.SetSender(Address,iSendPortNum);

		m_Sock.InitSockList();
		m_Sock.SetSender(Address,iSendPortNum);
	//	m_MediaSock.SetSender(Address,iSendPortNum+2);

		if(bMulticast)
		{
			m_MulticastSock.SetReceiver((char *)this->m_ConfigData.MultiCastGroupIP,
				this->m_ConfigData.MultiCastPortNum+this->m_ConfigData.GroupID);
	//		m_MulticastSock.SetReceiver((char *)this->m_ConfigData.MultiCastGroupIP,
	//			this->m_ConfigData.MultiCastPortNum /*+ this->m_ConfigData.Chanel_1 + 2500*/);
			m_MulticastSock.SetSender((char *)this->m_ConfigData.MultiCastGroupIP,
				this->m_ConfigData.MultiCastPortNum+this->m_ConfigData.GroupID);
	//		m_MulticastSock.SetSender((char *)this->m_ConfigData.MultiCastGroupIP,
	//			this->m_ConfigData.MultiCastPortNum /* + this->m_ConfigData.Chanel_1 + 2500*/);	


		}
	}

	if((!this->bRelay) && (bMulticast))
	{
		this->iWorkStatus = CLIENT_STREAM_SERVER_LOGIN_OK;
	} else {
		this->iWorkStatus = CLIENT_STREAM_SERVER_LOGIN;

		Packet packet;
		packet.id(CREATE_SESSION_REQ);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
/*
		Sleep(100);		
		packet.clear();
		packet.id(CREATE_AUDIO_SESSION_REQ);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		m_Sock_Audio.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());

		Sleep(100);
		packet.clear();
		packet.id(CREATE_VIDEO_SESSION_REQ);
		packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
		m_Sock_Video.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());
		*/

		
	}
}

void WorkProcess::CloseServer()
{
	m_Sock.Close();
//	this->m_MediaSock.Close();
	if(bMulticast)
	{
		m_MulticastSock.Close();
	}
	m_WinSock.CloseServer();
}

void WorkProcess::CloseStreamServer()
{
	m_Sock.Close();
	if(bMulticast)
	{
		m_MulticastSock.Close();
	}
}

bool WorkProcess::SendStreamData(void)
{
	return false;
}

int WorkProcess::FindPlayer(char * UserName)
{
	for(int i=0;i<MAXPLAYER ;i++)
	{
		if(!strcmp((char *)m_Mp3Play[i].UserName,(char *)UserName))
		{
			return i;
		}
	}

	for(int i=0;i<MAXPLAYER ;i++)
	{
		if( 1000 < timeGetTime()  - m_Mp3Play[i].lLastPlayTime)
		{
			memset(m_Mp3Play[i].UserName,0x00,sizeof(m_Mp3Play[i].UserName));

			strcpy((char *)m_Mp3Play[i].UserName,UserName);

			return i;
		}
	}
	return 0;
}


void WorkProcess::SendMediaData( char * pBuffer,unsigned long iLen,bool bControl )
{
	unsigned int i,iCount;
	iCount = 0;
	SOCKADDR_IN  pTarget_sin[50];// = new SOCKADDR_IN[iClientList];
	for( i=0;i<iClientList;i++ )
	{
		if(!MacCompare(ClientList[i].SocketContext.ClientData.MacAddress,ClinetInfo.MacAddress))
		//if(1)
		{
			pTarget_sin[iCount] = ClientList[i].SocketContext.socket_control;
			iCount ++;
		} 
		else {
			char strOut[100];
			sprintf_s(strOut, (" Port:%d      "),ntohs(ClientList[i].SocketContext.socket_control.sin_port));
			StatusOut(3,strOut);

		}
		
		
	}
	m_Sock.AddSendPacket(pBuffer,iLen,pTarget_sin,iCount,true);
	//delete [] pTarget_sin;
}


void  WorkProcess::PacketParser(Packet * pPacket,bool bTcp )
{
	unsigned short sSize;
	unsigned short ID = pPacket->id();

	if(!bTcp)
	{
		switch(ID)
		{
		case ALIVE_CHECK:
			{
				unsigned short sSize;
				unsigned long lTempTime;
				pPacket->readData(&sSize,2);
				pPacket->readData(&lTempTime,sSize);
				iAliveCount = 0;
			}
			break;

		case ALIVE_CHECK_CLIENT_REQ:
			{
				
				ClientListItem tempClient;
				pPacket->readData(&sSize,2);
				pPacket->readData(&tempClient,sSize);

				Packet retPacket;
				retPacket.id(ALIVE_CHECK_CLIENT_ACK);
				retPacket.writeData2(&ClinetInfo,sizeof(ClinetInfo));
				this->m_Sock.SendTo(&tempClient.SocketContext.socket_control,(char *)retPacket.getPacketBuffer(),retPacket.getPacketSize());
			}
			break;
		case ALIVE_CHECK_CLIENT_ACK:
			{
				unsigned int i;
				ClientInfoHead tempClientInfo;
				pPacket->readData(&sSize,2);
				pPacket->readData(&tempClientInfo,sSize);

				unsigned long TempTime = timeGetTime();
				EnterCriticalSection ( &cs );
				for( i=0;i<iClientList;i++ )
				{
					if(MacCompare(ClientList[i].SocketContext.ClientData.MacAddress,tempClientInfo.MacAddress))
					{
						if(1000 >= (TempTime - ClientList[i].LastCheckTime) )
						{
							ClientList[i].LastCheckTime = TempTime;						
						}
						break;
					}
				}
				LeaveCriticalSection ( &cs );
			}
			break;

		case CREATE_SESSION_ACK:
			{
				switch(iWorkStatus)
				{
				case CLIENT_STREAM_SERVER_LOGIN:
					unsigned long lTempTime;
					pPacket->readData(&sSize,2);
					pPacket->readData(&lTempTime,sSize);
					StreamServerStartTime = lTempTime;
					
					iWorkStatus = CLIENT_STREAM_SERVER_LOGIN_OK;
					bSendAck = false;

					Packet packet;
					packet.id(GROUP_LIST_UPDATE_REQ);
					packet.writeData2(&ClinetInfo,sizeof(ClinetInfo));
					this->m_Sock.SendTo((unsigned char *)packet.getPacketBuffer(),packet.getPacketSize());

					break;
				}
			}
			break;

		case GROUP_LIST_UPDATE_ACK:
			{
				unsigned int i;

				EnterCriticalSection ( &cs );

				Sleep(100);
				unsigned long tempTime = timeGetTime();
				pPacket->readData((void *)&sSize,sizeof(sSize));
				pPacket->readData((void *)&iClientList,sizeof(iClientList));
				for(i=0;i<iClientList;i++)
				{
					pPacket->readData((void *)&sSize,2);				
					pPacket->readData((void *)&ClientList[i].SocketContext,sSize);				
					ClientList[i].LastCheckTime = tempTime;
					
				}
				LeaveCriticalSection ( &cs );

			}
			break;

		case MEDIA_STREAM_DATA:
			{
				unsigned char StreamTemp[1024*10],StreamTemp1[1024*10];
				unsigned int	Count;

				pPacket->readData(&sSize,2);
				BYTE * pData = new BYTE [sSize];
				pPacket->readData(StreamTemp1,sSize);
	

				Count = 0;
				while(Count<sizeof(StreamTemp))
				{
					StreamTemp[Count]=Count;
					if(StreamTemp1[Count] != StreamTemp[Count])
					{
						int i=0;
					}
					Count++;
				}

				delete []pData;

			}
			break;
		case MEDIA_STREAM_JPEG_DATA:
			{
				unsigned short sSize;
				BITMAPINFOHEADER  bi;
				//pPacket->readData(&sSize,2);
				
				pPacket->readData2(&bi);
				
				pPacket->readData(&sSize,2);
				BYTE * pData = new BYTE [sSize];
				pPacket->readData(pData,sSize);
				CDM.DrawImage(0,&bi,bi.biWidth,bi.biHeight,sSize,pData);
				delete []pData;
			}
			break;
		case MEDIA_STREAM_MPEG4_DATA:
			{
				EnterCriticalSection (&cs );

	
				unsigned short sSize;
				BITMAPINFOHEADER  bi;
				pPacket->readData2(&bi);
				pPacket->readData(&sSize,2);
				//BYTE *pData = new BYTE [sSize];
				BYTE pData[1024*60];
				pPacket->readData(pData,sSize);

				this->DecDecompress((unsigned char *)pData,sSize);
				this->DrawImage(pThis->View2,bi);

				//delete []pData;

				LeaveCriticalSection(&cs );
				

				
			}
			break;

		case RECV_STREAM_DATA_FROM_SERVER:
			{
				unsigned short sSize;

				StreamDataHead TempConfigData;

				//StatusOut(2, ("RECV_STREAM_DATA_FROM_SERVER"));
				pPacket->readData(&sSize,2);
				pPacket->readData(&TempConfigData,sSize);
				pPacket->readData(&sSize,2);
				unsigned char * mp3buffer = new unsigned char[sSize];
				pPacket->readData(mp3buffer,sSize);
				
				if( (TempConfigData.DataType == M_EXTRA_STREAM_DATA)
					&& (iCurrentPlayLevel != EXTRA_STREAM_DATA) 
					&& (iCurrentPlayLevel != M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA)
					)
				{
					LastExtaraDataCheckTime_Manager = timeGetTime();
					iCurrentPlayLevel = M_EXTRA_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Extra\r\n"));
					//StatusOut(TRUE, ("Get M_EXTRA_STREAM_DATA Packet!"));
				}

				if( (TempConfigData.DataType == EXTRA_STREAM_DATA)
					&& (iCurrentPlayLevel != M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA)
					)
				{
					LastExtaraDataCheckTime = timeGetTime();
					iCurrentPlayLevel = EXTRA_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Emergency\r\n"));
					//StatusOut(TRUE, ("Get EXTRA_STREAM_DATA Packet!"));
				}

				if( (TempConfigData.DataType == M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA))
				{
					LastEmergencyCheckTime_Manager = timeGetTime();
					iCurrentPlayLevel = M_EMERGENCY_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Extra\r\n"));
					//StatusOut(TRUE, ("Get M_EMERGENCY_STREAM_DATA Packet!"));
				}

				if(TempConfigData.DataType == EMERGENCY_STREAM_DATA)
				{
					LastEmergencyCheckTime = timeGetTime();
					iCurrentPlayLevel = EMERGENCY_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Emergency\r\n"));
					//StatusOut(TRUE, ("Get Emergency Packet!"));
				}
#if HOBBY_MODE
				if(bConferenceMode)
				{

				int iSelect;
				if(!strcmp(szCurrentMacAddress,TempConfigData.SourceName))
				{


					iSelect = FindPlayer(TempConfigData.SourceName);
					m_Mp3Play[iSelect].AddBuffer((char *)mp3buffer,sSize);
					m_Mp3Play[iSelect].lLastTimeStemp	= TempConfigData.lTimeStemp;
					m_Mp3Play[iSelect].lLastPlayTime = timeGetTime();
					

				} else {
					iSelect = FindPlayer(TempConfigData.SourceName);
					{
						m_Mp3Play[iSelect].AddBuffer((char *)mp3buffer,sSize);
						m_Mp3Play[iSelect].lLastTimeStemp	= TempConfigData.lTimeStemp;
						m_Mp3Play[iSelect].lLastPlayTime = timeGetTime();
					}
				}

				} else {
#endif
					if(!bBroadcast && !bRelay) 
					{
						if(TempConfigData.DataType == iCurrentPlayLevel)
						{
							//int iSelect;
							//iSelect = FindPlayer(TempConfigData.SourceName);

							int iSelect = 0;
							//노멀 스트리밍이라면 
							if(iCurrentPlayLevel == NORMAL_STREAM_DATA)
							{
								if(TempConfigData.ChanelID == ClinetInfo.Chanel_1)
								{
									m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
								}else if(TempConfigData.ChanelID == ClinetInfo.Chanel_2)
								{
									m_Mp3Play[1].AddBuffer((char *)mp3buffer,sSize);
								}
							} else { //응급방송이나 기타방송은 무조건 0번(MIC)
								m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
							}
						}
					}

					if(bMulticast && bRelay)
					{
						if(TempConfigData.DataType == iCurrentPlayLevel)
						{
							Packet packet;
							packet.id(MULTICAST_STREAM_DATA);
							//sprintf( TempConfigData.SourceName, "user18");
							//sprintf( TempConfigData.TargetName, "user19");
							packet.writeData2(&TempConfigData,sizeof(StreamDataHead));
							packet.writeData2(mp3buffer,sSize);

							this->m_MulticastSock.SendTo(
								(unsigned char *)packet.getPacketBuffer(),
								packet.getPacketSize());
						}
					}
#if HOBBY_MODE
				}
#endif
				delete [] mp3buffer;
			}
			break;
		case SEND_STREAM_DATA_TO_SERVER_OK:
			{
				bServerReady  = true;
			}
			break;
		case RECV_STREAM_DATA_FROM_SERVER_OK:
			{
				int a=0;
			}
			break;

		case RECV_STREAM_DATA_BY_NAME:
			{
				unsigned short sSize;
				StreamDataHead TempConfigData;
				pPacket->readData(&sSize,2);
				pPacket->readData(&TempConfigData,sSize);
				pPacket->readData(&sSize,2);
				unsigned char * mp3buffer = new unsigned char[sSize];
				pPacket->readData(mp3buffer,sSize);
				m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
				delete [] mp3buffer;
			}
			break;

		case MULTICAST_STREAM_DATA:
			{
				//if(bRelay)
				//	return;

				unsigned short sSize;
				StreamDataHead TempConfigData;
				
				pPacket->readData(&sSize,2);
				pPacket->readData(&TempConfigData,sSize);

				pPacket->readData(&sSize,2);
				unsigned char * mp3buffer = new unsigned char[sSize];
				
				pPacket->readData(mp3buffer,sSize);
/*
				if((TempConfigData.DataType == EXTRA_STREAM_DATA) && (iCurrentPlayLevel != EMERGENCY_STREAM_DATA))
				{
					LastExtaraDataCheckTime = timeGetTime();
					iCurrentPlayLevel = EXTRA_STREAM_DATA;
				}

				if(TempConfigData.DataType == EMERGENCY_STREAM_DATA)
				{
					LastEmergencyCheckTime = timeGetTime();
					iCurrentPlayLevel = EMERGENCY_STREAM_DATA;
				}
*/
				if( (TempConfigData.DataType == M_EXTRA_STREAM_DATA)
					&& (iCurrentPlayLevel != EXTRA_STREAM_DATA) 
					&& (iCurrentPlayLevel != M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA)
					)
				{
					LastExtaraDataCheckTime_Manager = timeGetTime();
					iCurrentPlayLevel = M_EXTRA_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Extra\r\n"));
					//StatusOut(TRUE, ("Get M_EXTRA_STREAM_DATA Packet!"));
				}

				if( (TempConfigData.DataType == EXTRA_STREAM_DATA)
					&& (iCurrentPlayLevel != M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA)
					)
				{
					LastExtaraDataCheckTime = timeGetTime();
					iCurrentPlayLevel = EXTRA_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Emergency\r\n"));
					//StatusOut(TRUE, ("Get EXTRA_STREAM_DATA Packet!"));
				}

				if( (TempConfigData.DataType == M_EMERGENCY_STREAM_DATA) 
					&& (iCurrentPlayLevel != EMERGENCY_STREAM_DATA))
				{
					LastEmergencyCheckTime_Manager = timeGetTime();
					iCurrentPlayLevel = M_EMERGENCY_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Extra\r\n"));
					//StatusOut(TRUE, ("Get M_EMERGENCY_STREAM_DATA Packet!"));
				}

				if(TempConfigData.DataType == EMERGENCY_STREAM_DATA)
				{
					LastEmergencyCheckTime = timeGetTime();
					iCurrentPlayLevel = EMERGENCY_STREAM_DATA;
					//OutputDebugStr(_T("PlayLevel=Emergency\r\n"));
					//StatusOut(TRUE, ("Get Emergency Packet!"));
				}

				if((TempConfigData.ChanelID == this->m_ConfigData.Chanel_1) || (TempConfigData.ChanelID == this->m_ConfigData.Chanel_2))				
				if(!bRelay)
				{
					
					if(TempConfigData.DataType == iCurrentPlayLevel)
					{
						//int iSelect;
						//iSelect = FindPlayer(TempConfigData.SourceName);

						int iSelect = 0;
						//노멀 스트리밍이라면 
						if(iCurrentPlayLevel == NORMAL_STREAM_DATA)
						{
							if(TempConfigData.ChanelID == ClinetInfo.Chanel_1)
							{
								m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
							}else if(TempConfigData.ChanelID == ClinetInfo.Chanel_2)
							{
								m_Mp3Play[1].AddBuffer((char *)mp3buffer,sSize);
							}
						} else { //응급방송이나 기타방송은 무조건 0번(MIC)
							m_Mp3Play[0].AddBuffer((char *)mp3buffer,sSize);
						}

					}
				}
				delete [] mp3buffer;
			}
			break;
		}
	}else {
		switch(ID)
		{
		case LOGIN_ACK:
			{
				unsigned short sSize;
				pPacket->readData(&sSize,2);
				pPacket->readData(&m_ConfigData,sSize);
			
				switch(m_ConfigData.ClinetType)
				{
				case SOURCE_CLIENT:
					{
						bBroadcast = true;
						bMulticast = false;
						bRelay = false;
					}
					break;

				case SINK_CLIENT:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay = false;
					}
					break;

				case MULTICAST_CLIENT:
					{
						bBroadcast	= false;
						bMulticast	= true;
						bRelay = false;
					}
					break;
				case RELAY_CLIENT:
					{
						bBroadcast	= false;
						bMulticast	= true;
						bRelay		= true;
					}
					break;

				case CLIENT_MANAGER:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay	   = false;
					}
					break;

				case TEST_CLIENT:
					{
					//	ini.GetValue( _T("TEST"),_T("BROADCAST"),bBroadcast);
					//	ini.GetValue( _T("TEST"),_T("MULTICAST"),bMulticast);
					//	ini.GetValue(_T("TEST"),_T("RELAYSERVER"),bRelay);
					//	int temp;
					//	ini.GetValue(_T("TEST"),_T("GROUP_ID"),temp);
					//	m_ConfigData.GroupID = temp;
					}
					break;
				default:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay	   = false;
					}
					break;
				}

				iCurrentPlayLevel = NORMAL_STREAM_DATA;

				/*
				this->ClinetInfo.Chanel_1 =  m_ConfigData.Chanel_1;
				this->ClinetInfo.Chanel_2 =  m_ConfigData.Chanel_2;
				this->ClinetInfo.GroupID  =  m_ConfigData.GroupID;
				this->ClinetInfo.ClinetType = m_ConfigData.ClinetType;
				*/

				this->m_StreamDataHead.ChanelID =   m_ConfigData.Chanel_1 ;

				char szWindowName[1024];
				sprintf(szWindowName,("C!R=%d,B=%d,M=%d,G=%d,C1=%d,C2=%d"),bRelay,bBroadcast,bMulticast,m_ConfigData.GroupID,m_ConfigData.Chanel_1,m_ConfigData.Chanel_2);
				SetWindowTextA(hWndMain,szWindowName);
				ShowWindow(hWndMain, SW_SHOW);
				UpdateWindow(hWndMain);


				if(iWorkStatus == CLIENT_MAIN_SERVER_RECONNECT)
				{
					iWorkStatus = CLIENT_MAIN_SERVER_RECONNECT_OK;
				} else {
					iWorkStatus = CLIENT_DSOCK_INIT;
				}

			}
			break;

		case RESET_CONNECTION:  //UDP 서버 접속 재설정
			{
				unsigned short sSize;
				
				pPacket->readData(&sSize,2);
				pPacket->readData(&m_ConfigData,sSize);
			
				switch(m_ConfigData.ClinetType)
				{
				case SOURCE_CLIENT:
					{
						bBroadcast = true;
						bMulticast = false;
						bRelay = false;
					}
					break;

				case SINK_CLIENT:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay = false;
					}
					break;

				case MULTICAST_CLIENT:
					{
						bBroadcast	= false;
						bMulticast	= true;
						bRelay = false;
					}
					break;
				case RELAY_CLIENT:
					{
						bBroadcast	= false;
						bMulticast	= true;
						bRelay		= true;
					}
					break;

				case CLIENT_MANAGER:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay	   = false;
					}
					break;

				case TEST_CLIENT:
					{
						//ini.GetValue( _T("TEST"),_T("BROADCAST"),bBroadcast);
						//ini.GetValue( _T("TEST"),_T("MULTICAST"),bMulticast);
						//ini.GetValue(_T("TEST"),_T("RELAYSERVER"),bRelay);
						//int temp;
						//ini.GetValue(_T("TEST"),_T("GROUP_ID"),temp);
						//m_ConfigData.GroupID = temp;
					}
					break;
				default:
					{
						bBroadcast = false;
						bMulticast = false;
						bRelay	   = false;
					}
					break;
				}

				iCurrentPlayLevel = NORMAL_STREAM_DATA;

				this->ClinetInfo.Chanel_1 =  m_ConfigData.Chanel_1;
				this->ClinetInfo.Chanel_2 =  m_ConfigData.Chanel_2;
				this->ClinetInfo.GroupID  =  m_ConfigData.GroupID;

				this->m_StreamDataHead.ChanelID =   m_ConfigData.Chanel_1 ;
				char szWindowName[1024];
				sprintf(szWindowName,("C!R=%d,B=%d,M=%d,G=%d,C1=%d,C2=%d"),bRelay,bBroadcast,bMulticast,m_ConfigData.GroupID,m_ConfigData.Chanel_1,m_ConfigData.Chanel_2);
				SetWindowTextA(hWndMain,szWindowName);
//				ShowWindow(hWndMain, SW_SHOW);
				UpdateWindow(hWndMain);

				CloseStreamServer();
				iWorkStatus = CLIENT_DSOCK_INIT;
			}
			break;
		case MIC_VOLUME_REQ:
			{
				//OutputDebugStr(_T("GET MIC_VOLUME_REQ\r\n"));
				Packet retPacket;

				//m_curVolume.MicVol = GetMICVolume();

				retPacket.id(MIC_VOLUME_ACK);
				retPacket.writeData2(&m_StreamDataHead,sizeof(StreamDataHead));
				retPacket.writeData2(&m_curVolume, sizeof(m_curVolume));

				//this->m_Sock.SendTo((unsigned char *)retPacket.getPacketBuffer(),retPacket.getPacketSize());
				
			}
			break;
		case MIC_VOLUME_CONTROL_REQ:
			{
//				OutputDebugStr(("GET MIC_VOLUME_CONTROL_REQ\r\n"));
				MicVolume micVol;
				pPacket->readData(&sSize,2);
				pPacket->readData(&micVol, sSize);
				
//				MICVolume(micVol.MicVol);
			}
			break;
#if RS232SET
		case RS232_DATA_REQ:
			{
				char * pTemp;
				pPacket->readData(&sSize, 2);
				pTemp = new char[sSize];
				pPacket->readData(pTemp, sSize);
				if(m_bComOpen)
					m_ComuPort.WriteComm((BYTE*)pTemp, sSize);

				delete [] pTemp;
			}
			break;
#endif

		case WIN_INPUT_CONTROL_REQ:
			{
				
			}
			break;
		default:
		
			break;
		}

	}
	
}

/*
void WorkProcess::MICVolume(short vol)    // Mic volume 조절하는 메소드(0 ~ 15)
{
	short curVol;
	DWORD dwMicVol;
	//==================================================
	if(vol<0)                // volume value setting
		curVol = 0;
	else if(vol>10)
		curVol = 10;
	else
		curVol = vol;

	dwMicVol = (curVol*6553);    // 16 Step.
	//==================================================

	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, 0);

	// Get the line info for the wave in destination line 
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(mxl);
	mxl.dwComponentType = 
		MIXERLINE_COMPONENTTYPE_DST_WAVEIN; //MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; 
	mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

	// Now find the microphone source line connected to this wave in
	// destination
	DWORD cConnections = mxl.cConnections;
	for(DWORD j=0; j<cConnections; j++)
	{ 
		mxl.dwSource = j;
		mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE); 

		if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType) 
			break; 
	} 
	// Find a mute control, if any, of the microphone line 
	LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
	memset(pmxctrl, 0x0, sizeof(MIXERCONTROL));
	MIXERLINECONTROLS mxlctrl = {sizeof(MIXERLINECONTROLS), mxl.dwLineID, 
		MIXERCONTROL_CONTROLTYPE_VOLUME , 1, //MIXERCONTROL_CONTROLTYPE_MUTE

		sizeof MIXERCONTROL, pmxctrl}; 
	MMRESULT mmr;
	if(!(mmr = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, 
		MIXER_GETLINECONTROLSF_ONEBYTYPE)))// Found, so proceed
	{
		DWORD cChannels = mxl.cChannels;

		if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl) 
			cChannels = 1; 

		LPMIXERCONTROLDETAILS_UNSIGNED pbool = (LPMIXERCONTROLDETAILS_UNSIGNED) 
			malloc(cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));

		MIXERCONTROLDETAILS mxcd = { sizeof(mxcd), pmxctrl->dwControlID, cChannels, 
			(HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID) pbool}; 

		if((mmr = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)))
			//OutputDebugStr(_T("MIC VOL SET ERROR\r\n"));
//			MessageBox(NULL, "ERROR", "ERROR", MB_OK);
		// Unmute the microphone line (for both channels)
		pbool[0].dwValue = pbool[cChannels - 1].dwValue = dwMicVol;
		if((mmr = mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)))
			//OutputDebugStr(_T("MIC VOL SET ERROR\r\n"));
//			MessageBox(NULL, "ERROR", "ERROR", MB_OK);

		free(pmxctrl);
		free(pbool);
	} 
	else 
		free(pmxctrl); 

	mixerClose(hmx); 

	return;
}

*/
void WorkProcess::MICVolume(short vol)    // Mic volume 조절하는 메소드(0 ~ 15)
{
	DWORD m_dwmicvolume;
	//==================================================
	if(vol<0)                // volume value setting
		m_dwmicvolume = 0;
	else if(vol>10)
		m_dwmicvolume = 10;
	else
		m_dwmicvolume = vol;

	m_dwmicvolume = (m_dwmicvolume*6553);    // 16 Step.
	//==================================================

	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, 0);

	// Get the line info for the wave in destination line 
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(mxl);
	mxl.dwComponentType = 
		MIXERLINE_COMPONENTTYPE_DST_WAVEIN; //MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; 
	mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

	// Now find the microphone source line connected to this wave in
	// destination
	DWORD cConnections = mxl.cConnections;
	for(DWORD j=0; j<cConnections; j++)
	{ 
		mxl.dwSource = j;
		mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE); 

		if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType) 
			break; 
	} 
	// Find a mute control, if any, of the microphone line 
	LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
	memset(pmxctrl, 0x0, sizeof(MIXERCONTROL));
	MIXERLINECONTROLS mxlctrl = {sizeof(MIXERLINECONTROLS), mxl.dwLineID, MIXERCONTROL_CONTROLTYPE_VOLUME, 1, sizeof MIXERCONTROL, pmxctrl}; 
	MMRESULT mmr;
	if(!(mmr = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, MIXER_GETLINECONTROLSF_ONEBYTYPE)))// Found, so proceed
	{
		DWORD cChannels = mxl.cChannels;

		if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl) 
			cChannels = 1; 

		LPMIXERCONTROLDETAILS_UNSIGNED pbool = (LPMIXERCONTROLDETAILS_UNSIGNED ) malloc(cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
		MIXERCONTROLDETAILS mxcd = { sizeof(mxcd), pmxctrl->dwControlID, cChannels, (HWND)0, sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID) pbool}; 

		//if((mmr = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)))
//			OutputDebugStr(("mixer error\r\n"));
//			MessageBox(NULL, "ERROR", "ERROR", MB_OK);
		// Unmute the microphone line (for both channels)
		pbool[0].dwValue = pbool[cChannels - 1].dwValue = m_dwmicvolume;
//		if((mmr = mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)))
//			OutputDebugStr(("mixer error\r\n"));
//			MessageBox(NULL, "ERROR", "ERROR", MB_OK);

		free(pmxctrl);
		free(pbool);
	} 
	else 
		free(pmxctrl); 

	mixerClose(hmx); 

	return;
}

short WorkProcess::GetMICVolume()    // Mic volume 갖고오는 메소드(0 ~ 15)
{
	/*
	short curVol;
	DWORD dwMicVol;
	//==================================================
	if(vol<0)                // volume value setting
		curVol = 0;
	else if(vol>15)
		curVol = 15;
	else
		curVol = vol;

	dwMicVol = (curVol*4369);    // 16 Step.
	//==================================================
*/
	short retVolume=0;
	float dwVol=0.;
	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, 0);

	// Get the line info for the wave in destination line 
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(mxl);
	mxl.dwComponentType = 
		MIXERLINE_COMPONENTTYPE_DST_WAVEIN; //MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE; 
	mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_COMPONENTTYPE);

	// Now find the microphone source line connected to this wave in
	// destination
	DWORD cConnections = mxl.cConnections;
	for(DWORD j=0; j<cConnections; j++)
	{ 
		mxl.dwSource = j;
		mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_GETLINEINFOF_SOURCE); 

		if (MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE == mxl.dwComponentType) 
			break; 
	} 
	// Find a mute control, if any, of the microphone line 
	LPMIXERCONTROL pmxctrl = (LPMIXERCONTROL)malloc(sizeof(MIXERCONTROL));
	memset(pmxctrl, 0x0, sizeof(MIXERCONTROL));
	//MIXERLINECONTROLS mxlctrl = {sizeof(MIXERLINECONTROLS), mxl.dwLineID,MIXERCONTROL_CONTROLTYPE_VOLUME , 1, sizeof MIXERCONTROL, pmxctrl}; 

	MIXERLINECONTROLS mxlctrl = {sizeof(MIXERLINECONTROLS), mxl.dwLineID, 
		MIXERCONTROL_CONTROLTYPE_VOLUME , 1, //MIXERCONTROL_CONTROLTYPE_MUTE
		sizeof MIXERCONTROL, pmxctrl}; 

	MMRESULT mmr;
	if(!(mmr = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, 
		MIXER_GETLINECONTROLSF_ONEBYTYPE)))// Found, so proceed
	{
		DWORD cChannels = mxl.cChannels;

		if (MIXERCONTROL_CONTROLF_UNIFORM & pmxctrl->fdwControl) 
			cChannels = 1; 

		LPMIXERCONTROLDETAILS_UNSIGNED pbool = (LPMIXERCONTROLDETAILS_UNSIGNED )malloc(cChannels * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
		MIXERCONTROLDETAILS mxcd = { sizeof(mxcd), pmxctrl->dwControlID, cChannels, (HWND)0, 
			sizeof(MIXERCONTROLDETAILS_UNSIGNED), (LPVOID) pbool}; 

//		if((mmr = mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, MIXER_SETCONTROLDETAILSF_VALUE)))
//			OutputDebugStr(("MIC VOL SET ERROR\r\n"));

		dwVol = (float)pbool[0].dwValue;
		retVolume = (short)((dwVol/65535)*10);
		free(pmxctrl);
		free(pbool);
	} 
	else 
		free(pmxctrl); 

	mixerClose(hmx); 

	return retVolume;
}

// Input select. isMic=true : Select microphone, false: Line input
void WorkProcess::SetInputControl(DWORD dwSelLine)    
{

	short retVolume=0;
	float dwVol=0.;
//	DWORD dwSelLine = m_dwLineIndex;
//if(isMic) dwSelLine = m_dwMicIndex;
	DWORD dwLineID; 
//	DWORD dwSelType = MIXERLINE_COMPONENTTYPE_SRC_LINE;

//	if(isMic) dwSelType = MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE;

	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, MIXER_OBJECTF_MIXER);

	// Get the line info for the wave in destination line 
	MIXERLINE mxl;

	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN; 
	if(mixerGetLineInfo((HMIXEROBJ)hmx, &mxl, MIXER_OBJECTF_HMIXER |
		MIXER_GETLINEINFOF_COMPONENTTYPE) != MMSYSERR_NOERROR)
	{

		return;
	}

	dwLineID = mxl.dwLineID;

	
	MIXERCONTROL mxctrl;
	MIXERLINECONTROLS mxlctrl;
	mxlctrl.cbStruct = sizeof(MIXERLINECONTROLS);
	mxlctrl.dwLineID = dwLineID;
	mxlctrl.dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
	mxlctrl.cControls = 1;
	mxlctrl.cbmxctrl = sizeof(MIXERCONTROL);
	mxlctrl.pamxctrl = &mxctrl;

	MMRESULT mmr;
	if(!(mmr = mixerGetLineControls((HMIXEROBJ) hmx, &mxlctrl, 
		MIXER_OBJECTF_HMIXER |
		MIXER_GETLINECONTROLSF_ONEBYTYPE)))// Found, so proceed
	{

		MIXERCONTROLDETAILS_UNSIGNED *p_analog_data = 
			new MIXERCONTROLDETAILS_UNSIGNED[mxctrl.cMultipleItems];

		MIXERCONTROLDETAILS mxcd;
		mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
		mxcd.dwControlID = mxctrl.dwControlID;
		mxcd.cChannels = 1;
		mxcd.cMultipleItems = mxctrl.cMultipleItems;
		mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		mxcd.paDetails = p_analog_data;
		
		if ( mixerGetControlDetails((HMIXEROBJ)hmx, &mxcd, 
			MIXER_OBJECTF_HMIXER | 
			MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
		{
			unsigned int i;

			for( i = 0; i< mxctrl.cMultipleItems; i++){
				p_analog_data[i].dwValue = (dwSelLine == i);
			}

			mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
			mxcd.dwControlID = mxctrl.dwControlID;
			mxcd.cChannels = 1;
			mxcd.cMultipleItems = mxctrl.cMultipleItems;
			mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
			mxcd.paDetails = p_analog_data;

			if( (mixerSetControlDetails((HMIXEROBJ)hmx, &mxcd, 0))
				 == MMSYSERR_NOERROR)
			{
				//m_isMicSel = isMic;
			}
		}

		delete []p_analog_data;
	} 

	mixerClose(hmx); 

}


BOOL WorkProcess::GetSoundInputInfo()
{

	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, MIXER_OBJECTF_MIXER);

	if (hmx == NULL)
	{
		return FALSE;
	}

	// get dwLineID
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
	if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(hmx),
		&mxl,
		MIXER_OBJECTF_HMIXER |
		MIXER_GETLINEINFOF_COMPONENTTYPE)
		!= MMSYSERR_NOERROR)
	{
		return FALSE;
	}

	// get dwControlID
	MIXERCONTROL mxc;
	MIXERLINECONTROLS mxlc;

	DWORD dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;

	mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mxlc.dwLineID = mxl.dwLineID;
	mxlc.dwControlType = dwControlType;
	mxlc.cControls = 1;
	mxlc.cbmxctrl = sizeof(MIXERCONTROL);
	mxlc.pamxctrl = &mxc;
	if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(hmx),
		&mxlc,
		MIXER_OBJECTF_HMIXER |
		MIXER_GETLINECONTROLSF_ONEBYTYPE)
		!= MMSYSERR_NOERROR)
	{
		// no mixer, try MUX
		dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
		mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
		mxlc.dwLineID = mxl.dwLineID;
		mxlc.dwControlType = dwControlType;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = &mxc;
		if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(hmx),
			&mxlc,
			MIXER_OBJECTF_HMIXER |
			MIXER_GETLINECONTROLSF_ONEBYTYPE)
			!= MMSYSERR_NOERROR)
		{
			return FALSE;
		}
	}

	// store dwControlID, cMultipleItems
	CString strDstLineName = mxl.szName;
	CString SelectControlName = mxc.szName;
	DWORD dwSelectControlID = mxc.dwControlID;
	DWORD dwMultipleItems = mxc.cMultipleItems;

	if (dwMultipleItems == 0)
	{
		return FALSE;
	}

	// get the index of the Microphone Select control
	MIXERCONTROLDETAILS_LISTTEXTA *pmxcdSelectText =
		new MIXERCONTROLDETAILS_LISTTEXTA[dwMultipleItems];

	if (pmxcdSelectText != NULL)
	{
		MIXERCONTROLDETAILS mxcd;
		mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
		mxcd.dwControlID = dwSelectControlID;
		mxcd.cChannels = 1;
		mxcd.cMultipleItems = dwMultipleItems;
		mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
		mxcd.paDetails = pmxcdSelectText;
		if (::mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(hmx),
			&mxcd,
			MIXER_OBJECTF_HMIXER |
			MIXER_GETCONTROLDETAILSF_LISTTEXT)
			== MMSYSERR_NOERROR)
		{
			// determine which controls the Microphone source line
			for (DWORD dwi = 0; dwi < dwMultipleItems; dwi++)
			{
				// get the line information
				MIXERLINE mxl;
				mxl.cbStruct = sizeof(MIXERLINE);
				mxl.dwLineID = pmxcdSelectText[dwi].dwParam1;

				if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(hmx),
					&mxl,
					MIXER_OBJECTF_HMIXER |
					MIXER_GETLINEINFOF_LINEID)
					== MMSYSERR_NOERROR)
				{
					if(mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
						m_dwMicIndex = dwi;
					else if(mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_LINE)
						m_dwLineIndex = dwi;


					if (::lstrcmpA(pmxcdSelectText[dwi].szName,
						("Stereo Mix")) == 0)
					{
						// found, dwi is the index.
						m_dwMixIndex = dwi;
						//m_strMicName = pmxcdSelectText[dwi].szName;
						break;
					}
				}

			}
		}

		delete []pmxcdSelectText;
	}

	return (m_dwMicIndex < dwMultipleItems) && (m_dwLineIndex < dwMultipleItems);
}

void WorkProcess::SetLineMicOn()
{

	HMIXER hmx;
	mixerOpen(&hmx, 0, 0, 0, MIXER_OBJECTF_MIXER);

	if (hmx == NULL)
	{
		return;
	}

	// get dwLineID
	MIXERLINE mxl;
	mxl.cbStruct = sizeof(MIXERLINE);
	mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN; 
	if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(hmx),
		&mxl,
		MIXER_OBJECTF_HMIXER |
		MIXER_GETLINEINFOF_COMPONENTTYPE)
		!= MMSYSERR_NOERROR)
	{
		return ;
	}

	// get dwControlID
	MIXERCONTROL mxc;
	MIXERLINECONTROLS mxlc;

	DWORD dwControlType = MIXERCONTROL_CONTROLTYPE_MIXER;

	mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
	mxlc.dwLineID = mxl.dwLineID;
	mxlc.dwControlType = dwControlType;
	mxlc.cControls = 1;
	mxlc.cbmxctrl = sizeof(MIXERCONTROL);
	mxlc.pamxctrl = &mxc;
	if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(hmx),
		&mxlc,
		MIXER_OBJECTF_HMIXER |
		MIXER_GETLINECONTROLSF_ONEBYTYPE)
		!= MMSYSERR_NOERROR)
	{
		// no mixer, try MUX
		dwControlType = MIXERCONTROL_CONTROLTYPE_MUX;
		mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
		mxlc.dwLineID = mxl.dwLineID;
		mxlc.dwControlType = dwControlType;
		mxlc.cControls = 1;
		mxlc.cbmxctrl = sizeof(MIXERCONTROL);
		mxlc.pamxctrl = &mxc;
		if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(hmx),
			&mxlc,
			MIXER_OBJECTF_HMIXER |
			MIXER_GETLINECONTROLSF_ONEBYTYPE)
			!= MMSYSERR_NOERROR)
		{
			return;
		}
	}

	// store dwControlID, cMultipleItems
	CString strDstLineName = mxl.szName;
	CString SelectControlName = mxc.szName;
	DWORD dwSelectControlID = mxc.dwControlID;
	DWORD dwMultipleItems = mxc.cMultipleItems;

	if (dwMultipleItems == 0)
	{
		return ;
	}

	// get the index of the Microphone Select control
	MIXERCONTROLDETAILS_LISTTEXTA *pmxcdSelectText =
		new MIXERCONTROLDETAILS_LISTTEXTA[dwMultipleItems];

	if (pmxcdSelectText != NULL)
	{
		MIXERCONTROLDETAILS mxcd;
		mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
		mxcd.dwControlID = dwSelectControlID;
		mxcd.cChannels = 1;
		mxcd.cMultipleItems = dwMultipleItems;
		mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
		mxcd.paDetails = pmxcdSelectText;
		if (::mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(hmx),
			&mxcd,
			MIXER_OBJECTF_HMIXER |
			MIXER_GETCONTROLDETAILSF_LISTTEXT)
			== MMSYSERR_NOERROR)
		{
			// determine which controls the Microphone source line
			for (DWORD dwi = 0; dwi < dwMultipleItems; dwi++)
			{
				// get the line information
				MIXERLINE mxl;
				mxl.cbStruct = sizeof(MIXERLINE);
				mxl.dwLineID = pmxcdSelectText[dwi].dwParam1;

				if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(hmx),
					&mxl,
					MIXER_OBJECTF_HMIXER |
					MIXER_GETLINEINFOF_LINEID)
					== MMSYSERR_NOERROR)
				{
					/*
					if(mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE)
						m_dwMicIndex = dwi;
					else if(mxl.dwComponentType == MIXERLINE_COMPONENTTYPE_SRC_LINE)
						m_dwLineIndex = dwi;
						*/

							if (::lstrcmpA(pmxcdSelectText[dwi].szName,
								("Stereo Mix")) == 0)
							{
								// found, dwi is the index.
								m_dwMicIndex = dwi;
								//m_strMicName = pmxcdSelectText[dwi].szName;
								break;
							}
				}

			}
		}

		delete []pmxcdSelectText;
	}

	return;// (m_dwMicIndex < dwMultipleItems) && (m_dwLineIndex < dwMultipleItems);
}