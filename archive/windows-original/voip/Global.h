# ifndef GLOBAL_H
# define GLOBAL_H


#define CLIENTID 0x0001


enum CATEGORY_ID
{
	CID_VOICE = 0x0001,
	CID_MUSIC,
	CID_DATA,
	CID_VIDEO,
	CID_ETC,
};

enum RTP_PACKET_ID
{
	RTP_STREAM_DATA				= 9100,
	RTP_CONTROL_NEW_PACKET_REQ = 9000,
	RTP_CONTROL_NEW_PACKET_ACK,

	RTP_CONTROL_SEND_PACKET_DATA,

	RTP_CONTROL_SEND_PACKET_START_REQ,
	RTP_CONTROL_SEND_PACKET_START_ACK,

	RTP_CONTROL_SEND_PACKET_END_REQ,
	RTP_CONTROL_SEND_PACKET_END_ACK,
	
	RTP_CONTROL_SEND_PACKET_DATA_NOREQ,
	RTP_CONTROL_SEND_PACKET_DATA_NOREQ_ACK,

};

enum RTP_CTRL_ID  //RTP 패킷 컨트롤 ID
{
	RTP_CTRL_RECV_PACKET_UPDATE = 0x1001, //받은 패킷 리스트 서버에 전송
	RTP_CTRL_NEW_FRAME,
	RTP_CTRL_TEST,
//	RTP_CTRL_,
};

enum CLIENT_TYPE
{
	SOURCE_CLIENT	=1000,
	SINK_CLIENT	,
	MULTICAST_CLIENT,
	RELAY_CLIENT,
	CLIENT_MANAGER,
	TEST_CLIENT,
};

enum STREAM_DATA_TYPE
{
	NORMAL_STREAM_DATA	=0x1001,
	EXTRA_STREAM_DATA,	
	EMERGENCY_STREAM_DATA,
	M_EXTRA_STREAM_DATA,
	M_EMERGENCY_STREAM_DATA,
};

enum PACKET_ID
{
	ALIVE_CHECK = 0,
	ALIVE_CHECK_CLIENT_REQ, 
	ALIVE_CHECK_CLIENT_ACK, 

	LOGIN_REQ		= 1001,
	LOGIN_ACK,

	RESET_CONNECTION,  //스트림 서버 접속 리셋

	GROUP_LIST_UPDATE_REQ,
	GROUP_LIST_UPDATE_ACK,

	STREAM_SERVER_CONFIG_REQ,
	STREAM_SERVER_CONFIG_ACK,

//++++ CLIENT CONFIG DATA EDIT 
	GET_CONFIG_DATA_REQ,
	GET_CONFIG_DATA_ACK,

	SET_CONFIG_DATA_REQ,
	SET_CONFIG_DATA_ACK,

	ADD_CONFIG_DATA_REQ,
	ADD_CONFIG_DATA_ACK,

	DELETE_CONFIG_DATA_REQ,
	DELETE_CONFIG_DATA_ACK,

	UPDATE_CONFIG_DATA_REQ,
	UPDATE_CONFIG_DATA_ACK,
	
	GET_CLINET_INFO_REQ,
	GET_CLINET_INFO_ACK,
//---- CLIENT CONFIG DATA EDIT

	CREATE_SESSION_REQ = 2000,
	CREATE_SESSION_ACK = 2001,
	DELETE_SESSION_REQ = 2002,
	DELETE_SESSION_ACK = 2003,
	CREATE_AUDIO_SESSION_REQ,
	CREATE_AUDIO_SESSION_ACK,
	CREATE_VIDEO_SESSION_REQ,
	CREATE_VIDEO_SESSION_ACK,

	SEND_STREAM_DATA_TO_SERVER		= 3000,
	RECV_STREAM_DATA_FROM_SERVER	= 3001,
	SEND_STREAM_DATA_TO_SERVER_OK	= 3002,
	RECV_STREAM_DATA_FROM_SERVER_OK	= 3003,
	
	SEND_STREAM_DATA_BY_NAME = 4001,
	RECV_STREAM_DATA_BY_NAME = 4002,

	MULTICAST_STREAM_DATA			= 5001,

	MEDIA_STREAM_DATA				= 6000,
	MEDIA_STREAM_JPEG_DATA,
	MEDIA_STREAM_MPEG4_DATA,
	
	CHANNEL_LEVEL_ACK				= 7000,

	WIN_VOLUME_REQ					= 7010,
	WIN_VOLUME_ACK					= 7011,

	MIC_VOLUME_REQ					= 7020,
	MIC_VOLUME_ACK					= 7021,
	MIC_VOLUME_CONTROL_REQ			= 7023,

	RS232_DATA_REQ					= 7030,

	WIN_INPUT_CONTROL_REQ			= 7040,
};

enum CLIENT_STATUS
{
	CLIENT_HALT						=	0x0000,
	CLIENT_INIT						=	0x0001,
	CLIENT_WAIT,
	CLIENT_DESTROY,
	CLIENT_CHANGE_SETTINGS,
	
	CLIENT_SOCK_INIT				=	0x1001,
	CLIENT_SOCK_WAIT_LOGIN,

	CLIENT_DSOCK_INIT				=	0x2001,
	CLIENT_DSOCK_CONNECTED,
	CLIENT_DSOCK_CLOSED,
	
	CLIENT_STREAM_SERVER_LOGIN		=	0x3001,
	CLIENT_STREAM_SERVER_LOGIN_OK,
	CLIENT_STREAM_SERVER_LOGIN_RETRY,

	CLIENT_MAIN_SERVER_RECONNECT,
	CLIENT_MAIN_SERVER_RECONNECT_WAIT,
	CLIENT_MAIN_SERVER_RECONNECT_OK,
	
	CLIENT_STREAM_ENCODE			=	0x4001,
	CLIENT_STREAM_DECODE,
	CLIENT_STREAM_SEND,
	CLIENT_STREAM_RECV,
	CLIENT_STREAM_RECV_EMERGENCY,
};


typedef struct _tagClientDataHead
{
	char UserName[64];
	unsigned long ClinetType;  //
	unsigned long ClientID;
	unsigned long Chanel_1;
	unsigned long Chanel_2;
	unsigned long GroupID;
	unsigned char MacAddress[6];
}ClientInfoHead;

typedef struct _tagStreamDataHead
{
	char SourceName[64];
	char TargetName[64];
	unsigned long DataType;
	unsigned long ChanelID;
	unsigned long GroupID;
	unsigned long lTimeStemp;
}StreamDataHead;

typedef struct _tagConfigData
{
	unsigned char	StreamServerAddress[24];
	unsigned char	MultiCastGroupIP[24];
	unsigned long	ClinetType;
	unsigned int	MultiCastPortNum;
	unsigned int	RecvPortNum;
	unsigned int	SendPortNum;
	SYSTEMTIME		LastConfigDate;
	unsigned int	GroupID;
	unsigned int	Chanel_1;
	unsigned int	Chanel_2;
	unsigned long	UnitDataSize;
	char			MacAddress[6];
}ConfigData;

typedef struct tagPerSocketContext
{
	SOCKADDR_IN		socket_control;
	SOCKADDR_IN		socket_audio;
	SOCKADDR_IN		socket_video;
	ClientInfoHead	ClientData;
} PerSocketContext, *PPerSocketContext;

typedef struct tagClientListItem
{
  unsigned long LastCheckTime;
  bool	bAlive;
  PerSocketContext SocketContext;
} ClientListItem;



typedef struct _tagChannelLevel
{
	unsigned int	Channel_NO;
	short			level;
}ChannelLevel;

typedef struct _tagWinVolume
{
	unsigned char MacAddress[6];
	unsigned int inVolume;
}WinVolume;

typedef struct _tagMicVolume
{
	unsigned char MacAddress[6];
	short MicVol;
}MicVolume;
/*
typedef struct _tagRS232Data
{
	unsigned char MacAddress[6];
	char * buffer;
}RS232Data;
*/

typedef struct _tagInputControl
{
	unsigned char MacAddress[6];
	bool isMicSel;
}InputControl;



typedef void CALLBACK UserSocketCallBack(BITMAPINFOHEADER  bi,void * lpData,int iLen);
typedef UserSocketCallBack * pUserSocketCallBack;

typedef void CALLBACK UserSocketCallBack1(void * Data,unsigned long DataLen);
typedef UserSocketCallBack1 * pUserSocketCallBack1;


#endif