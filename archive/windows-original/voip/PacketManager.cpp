#include "StdAfx.h"
#include "PacketManager.h"
#include "DataStruct.h"
#include "resource.h"		// 주 기호입니다.
//#include <Mmsystem.h>
//#include "SockTest_ClientDlg.h"

_GlobalData GlobalData;
CPacketManager * pThis;

void AddDebugString(CString str)
{


//	CString szTemp;
	
	/*

	iLength= pThis->m_pEdit->GetWindowTextLength();
	pThis->m_pEdit->SetSel(iLength,iLength,false);

	szTemp = str;
	szTemp += "\r\n";
	pThis->m_pEdit->ReplaceSel(szTemp);

	iLength= pThis->m_pEdit->GetWindowTextLength();
	pThis->m_pEdit->SetSel(iLength,iLength,false);

	pThis->m_pEdit->GetScrollRange(SB_VERT,&iMin,&iMax);
	pThis->m_pEdit->SetScrollPos(SB_VERT,iMax);
	*/

}


VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	char temp[2];
	temp[0]='1';
	temp[1]=0;
	
	if(pThis->AckCheck[idEvent])
	{
		pThis->RequestSend(idEvent,temp);
	}
}

CPacketManager::CPacketManager(void)
{
	pSock = NULL;
	pThis= this;

	memset(AckCheck,0x00,sizeof(AckCheck));

//	CString FilePath;

	//::GetCurrent
/*
	::GetCurrentDirectory(256,CurrentPath);
	
	FilePath = CurrentPath;

	FilePath +="\\Packets.dat";


	file.Open(FilePath,CFile::modeCreate|CFile::modeReadWrite);
	*/
	
}

CPacketManager::~CPacketManager(void)
{
//	file.Close();
	
}


bool RunParser()
{
	BYTE CRC;
	CString szTemp;
	unsigned char buffer[256];
	memset(buffer,0x00,sizeof(buffer));
	CPacket	SendPacket;

	::SYSTEMTIME time;
	::GetLocalTime(&time);

	CRC = pThis->RecvPacket.GetCRC(pThis->RecvPacket.ParserData.pData,pThis->RecvPacket.ParserData.TotalLen-1);

	int iCase = pThis->RecvPacket.GetCode();


	GlobalData.iSendCount = 0;

	if(pThis->AckCheck[iCase])
	{
		pThis->AckCheck[iCase] = false;
		::KillTimer(NULL,iCase);
	}

	switch(iCase)
		{
		case 0x09:
			AddDebugString("오류 발생 ACK");

			//pThis->RecvPacket.GetCode();


			buffer[0] = pThis->RecvPacket.GetChar();
			buffer[1] = pThis->RecvPacket.GetChar();
			buffer[2] =0;

			szTemp.Format(_T("코드 %s 에서 에러가 발생"),buffer);
			
			AddDebugString(szTemp);

			if(pThis->RecvPacket.GetChar() !=0x3 ) break;

			
			if(pThis->RecvPacket.GetChar() != CRC) break;

			return true;

			
		case 0x15:
			AddDebugString("오류 발생 ACK");

			break;
		case ID_POSITION_T2S:			//=	31,	//위치보고 
			{
				AddDebugString("ID_POSITION_T2S : 31  위치보고 ACK");

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_ALLOCATION:				//=	32,	//배차 보고
			{
				AddDebugString("ID_ALLOCATION: 32  배차 보고 ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_BOARDING:				//=	33,	//승차 보고
			{
				AddDebugString("ID_BOARDING: 33  승차 보고 ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_LEAVING:				//=	34,	//하차 보고
			{
				AddDebugString("ID_LEAVING:	34,	하차 보고 ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_ALLOCATION_CANCEL_T2S:	//=	35,	//배차 취소 보고
			{
				AddDebugString("ID_ALLOCATION_CANCEL_T2S: 35, 배차 취소 보고 ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_APPOINTMENT:			//=	36,	//예약 보고
			{
				AddDebugString("ID_APPOINTMENT:	36,	예약 보고  ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_APPOINTMENT_CANCEL:		//=	37, 예약 취소 보고
			{
				AddDebugString("ID_APPOINTMENT_CANCEL: 37, 예약 취소 보고 ACK ");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_EMERGENCY:				//=	38, 긴급보고
			{
				AddDebugString("ID_EMERGENCY:	38, 긴급보고  ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_CONFIGT_T2S:			//=	39,  설정정보 요청
			{
				AddDebugString("ID_CONFIGT_T2S:	39,  설정정보 요청  ACK");
				GlobalData.EmptyCycle = pThis->RecvPacket.GetShort();
				GlobalData.FullCycle  = pThis->RecvPacket.GetShort();

				GlobalData.Callback  = pThis->RecvPacket.GetChar();
				GlobalData.Empty_GPS_Num = pThis->RecvPacket.GetChar();
				GlobalData.Full_GPS_Num	 = pThis->RecvPacket.GetChar();

				GlobalData.ResCancelLength  = pThis->RecvPacket.GetShort();
				GlobalData.ResCancelTime	= pThis->RecvPacket.GetShort();

				GlobalData.WaitingListLength	= pThis->RecvPacket.GetChar();
				memset(GlobalData.WaitList,0x00,sizeof(GlobalData.WaitList));

				pThis->RecvPacket.GetString(GlobalData.WaitList,GlobalData.WaitingListLength);

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

			}
			return true;

		case ID_ALLOCATION_WAIT:		//=	40, 배차대기 보고 
			{
				AddDebugString("ID_ALLOCATION_WAIT:	40, 배차대기 보고 ACK");

			
				GlobalData.WaitingCount =pThis->RecvPacket.GetChar();

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;

		case ID_ALLOCATION_WAIT_CANCEL:	//=	41,배차대기 취소보고
			{
				AddDebugString("ID_ALLOCATION_WAIT_CANCEL: 41,배차대기 취소보고  ACK");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
			}
			return true;


		case ID_POSITION_S2T: //= 61   위치
			{
				AddDebugString("ID_POSITION_S2T : 위치정보 요청");
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
				

				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);

				
				//==> 위치요청 응답 함수 추가
				//pThis->RequestSend(ID_POSITION_T2S,NULL); // 위치 보고 함수
			}

			return true;

		case ID_TAGET_POSITION:			//=	62,	//목적지
			{
				AddDebugString("ID_TAGET_POSITION : 목적지 전송  요청");

				GlobalData.TargetLatitude = pThis->RecvPacket.GetInt();
				GlobalData.TargetLongitude = pThis->RecvPacket.GetInt();
				GlobalData.TargetMessageLength = pThis->RecvPacket.GetChar();
				memset(GlobalData.TargetMessage,0x00,sizeof(GlobalData.TargetMessage));
				pThis->RecvPacket.GetString(GlobalData.TargetMessage,GlobalData.TargetMessageLength);

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;


				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);

				
				//==> 목적지전송 응답 함수 추가 

				//==> 네비게이션 프로그램에도 전달해 주어야 함
			}
			return true;


		case IID_ALLOCATION_CANCEL_S2T:		//=	63,	//예약 취소
			{
				AddDebugString("ID_ALLOCATION_CANCEL_S2T : 예약취소 요청");

				int AllocationIDX =pThis->RecvPacket.GetInt();

				if(GlobalData.AllocationIndex == AllocationIDX)
				{
					AddDebugString("\t 배차 최소를 위한 인덱스 전송받음");

				} else {
					GlobalData.AllocationIndex = AllocationIDX;
					AddDebugString("존재하지 않는 배차 인덱스 전송받음");
				}
				GlobalData.AllocationCancelCode = pThis->RecvPacket.GetChar(); // 배차취소 코드
				
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				//==> 배차취소요청에 대한 처리 추가

				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);


			}
			return true;


		case ID_MESSAGE:				//=	64,	//메시지 전송	
			{
				AddDebugString("ID_MESSAGE : 메시지 전송");
				int iTitleLength;
				int iMessgeLength;
				char szTitle[64];
				char szMessge[256];

				int MessageType = pThis->RecvPacket.GetChar();

				switch(MessageType)
				{
				case 0x00:
					AddDebugString("공지");
					break;
				case 0x01:
					AddDebugString("광고");
					break;
				}

				iTitleLength = pThis->RecvPacket.GetChar();
				memset(szTitle,0x00,sizeof(szTitle));
				pThis->RecvPacket.GetString(szTitle,iTitleLength);
				AddDebugString(szTitle);

				iMessgeLength = pThis->RecvPacket.GetChar();
				memset(szMessge,0x00,sizeof(szMessge));
				pThis->RecvPacket.GetString(szMessge,iMessgeLength);
				AddDebugString(szMessge);
				
				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);
			}
			return true;


		case ID_CONFIG_S2T:				//=	65,	//설정
			{

				AddDebugString("ID_CONFIG_S2T : 설정");

				GlobalData.EmptyCycle = pThis->RecvPacket.GetShort();
				GlobalData.FullCycle  = pThis->RecvPacket.GetShort();

				GlobalData.Callback  = pThis->RecvPacket.GetChar();
				GlobalData.Empty_GPS_Num = pThis->RecvPacket.GetChar();
				GlobalData.Full_GPS_Num	 = pThis->RecvPacket.GetChar();

				GlobalData.ResCancelLength  = pThis->RecvPacket.GetShort();
				GlobalData.ResCancelTime	= pThis->RecvPacket.GetShort();

				GlobalData.WaitingListLength	= pThis->RecvPacket.GetChar();
				memset(GlobalData.WaitList,0x00,sizeof(GlobalData.WaitList));

				pThis->RecvPacket.GetString(GlobalData.WaitList,GlobalData.WaitingListLength);

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				//==>설정 전송에 대한 처리 추가

				//전송이 완료되었다는 신호를 보냄

				pThis->RequestSend(iCase,NULL);
				
				GlobalData.LastReportTime = timeGetTime();
				GlobalData.LastSendTime = timeGetTime();

				GlobalData.bUpdated = true;

			}
			return true;


		case ID_SERVER_INFO:			//=	66,	//서버정보
			{
				AddDebugString("ID_SERVER_INFO : 서버정보 요청");

				GlobalData.Server_IP[0] = pThis->RecvPacket.GetChar();
				GlobalData.Server_IP[1] = pThis->RecvPacket.GetChar();
				GlobalData.Server_IP[2] = pThis->RecvPacket.GetChar();
				GlobalData.Server_IP[3] = pThis->RecvPacket.GetChar();

				GlobalData.Server_Port = pThis->RecvPacket.GetShort();

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
				//==> 서버 정보 전송에 대한 처리 추가

				szTemp.Format(_T("server IP : %d.%d.%d.%d Port: %d "),
								GlobalData.Server_IP[0],
								GlobalData.Server_IP[1],
								GlobalData.Server_IP[2],
								GlobalData.Server_IP[3],
								GlobalData.Server_Port);

				AddDebugString(szTemp);

				//전송이 완료되었다는 신호를 보냄
			pThis->RequestSend(iCase,NULL);
			}
			return true;


		case ID_ALLOCATION_INFO:			//=	67, //배차지령 전송
			{
				AddDebugString("ID_ALLOCATION_INFO : 배차지령 전송");

				GlobalData.AllocationIndex = pThis->RecvPacket.GetInt();
				GlobalData.AllocationLatitude = pThis->RecvPacket.GetInt();
				GlobalData.AllocationLongitude = pThis->RecvPacket.GetInt();
				GlobalData.AllocationMessageLength = pThis->RecvPacket.GetChar();
				
				memset(GlobalData.AllocationMessage,
					0x00,
					sizeof(GlobalData.AllocationMessage));
				pThis->RecvPacket.GetString(GlobalData.AllocationMessage,GlobalData.AllocationMessageLength);

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;
				//==> 배차 지령 전송에 대한 처리 추가
				szTemp.Empty();
				szTemp.Format(_T("%d %d %d %d %s"),
						GlobalData.AllocationIndex,
						GlobalData.AllocationLatitude,
						GlobalData.AllocationLongitude,
						GlobalData.AllocationMessageLength,
						GlobalData.AllocationMessage);
				AddDebugString(szTemp);

				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);
			}
			return true;


		case ID_ALLOCATION_ACCEPT_TYPE1:		//=	68, //배차당첨전송(1)
			{
				AddDebugString("ID_ALLOCATION_ACCEPT_TYPE1 : 배차당첨전송(1)");
				GlobalData.AllocationIndex = pThis->RecvPacket.GetInt();
				GlobalData.AllocationDetailMessageLength = pThis->RecvPacket.GetChar();
				memset(GlobalData.AllocationDetailMessage,0x00,sizeof(GlobalData.AllocationDetailMessage));
				pThis->RecvPacket.GetString(GlobalData.AllocationDetailMessage,
								 GlobalData.AllocationDetailMessageLength);


				GlobalData.ClientPhoneNumLength = pThis->RecvPacket.GetChar();
				memset(GlobalData.ClientPhoneNum,0x00,sizeof(GlobalData.ClientPhoneNum));
				pThis->RecvPacket.GetString(GlobalData.ClientPhoneNum,
								 GlobalData.ClientPhoneNumLength);


				GlobalData.DeferredPayment=	pThis->RecvPacket.GetChar();

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);
				

				GlobalData.bVoiceInit = false;
				GlobalData.iTRSMode = VOICE_MODE;
				//::SendMessage(hWndMain,WM_COMMAND,MAKELONG(iCase,IDE_PACKET_RECV),NULL);

				//::PostMessage(hWndMain,WM_COMMAND,

				//==> 배차당첨 전송 1 에 대한 함수 추가
				szTemp.Empty();
				szTemp.Format(_T("%d %d %s %d %s %d"),
						GlobalData.AllocationIndex,
						GlobalData.AllocationDetailMessageLength,
						GlobalData.AllocationDetailMessage,
						GlobalData.ClientPhoneNumLength ,
						GlobalData.ClientPhoneNum,
						GlobalData.DeferredPayment);

				//;
//				MessageBox(hWndMain,A2CW(GlobalData.ClientPhoneNum),NULL,MB_OK);
				AddDebugString(szTemp);
			}
			return true;


		case ID_ALLOCATION_ACCEPT_TYPE2:		//=	69, //배차당첨전송(2)
			{
				AddDebugString("ID_ALLOCATION_ACCEPT_TYPE2 : 배차당첨전송(2)");

				GlobalData.AllocationIndex = pThis->RecvPacket.GetInt();
				GlobalData.AllocationLatitude = pThis->RecvPacket.GetInt();
				GlobalData.AllocationLongitude = pThis->RecvPacket.GetInt();
				GlobalData.AllocationMessageLength = pThis->RecvPacket.GetChar();
				
				memset(GlobalData.AllocationMessage,
					0x00,
					sizeof(GlobalData.AllocationMessage));
				pThis->RecvPacket.GetString(GlobalData.AllocationMessage,GlobalData.AllocationMessageLength);


				GlobalData.AllocationDetailMessageLength = pThis->RecvPacket.GetChar();
				memset(GlobalData.AllocationDetailMessage,0x00,sizeof(GlobalData.AllocationDetailMessage));
				pThis->RecvPacket.GetString(GlobalData.AllocationDetailMessage,
								 GlobalData.AllocationDetailMessageLength);


				GlobalData.ClientPhoneNumLength = pThis->RecvPacket.GetChar();
				memset(GlobalData.ClientPhoneNum,0x00,sizeof(GlobalData.ClientPhoneNum));
				pThis->RecvPacket.GetString(GlobalData.ClientPhoneNum,
								 GlobalData.ClientPhoneNumLength);


				GlobalData.DeferredPayment=	pThis->RecvPacket.GetChar();

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				//==> 배차당첨 전송2 에 대한 함수 추가
				szTemp.Empty();
				szTemp.Format(_T("%d  %d %d %d %s %d %s %d %s %d"),
						GlobalData.AllocationIndex,
						GlobalData.AllocationLatitude,
						GlobalData.AllocationLongitude,
						GlobalData.AllocationMessageLength,
						GlobalData.AllocationMessage,
						GlobalData.AllocationDetailMessageLength,
						GlobalData.AllocationDetailMessage,
						GlobalData.ClientPhoneNumLength ,
						GlobalData.ClientPhoneNum,
						GlobalData.DeferredPayment);
				AddDebugString(szTemp);


				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);
				
			}
			return true;


		case ID_ALLOCATION_FALSE:			//=	70, //배차실패보고	
			{
				AddDebugString("ID_ALOCATION_FALSE : 배차실패보고");

				int AllocationCancelIDX ;
				AllocationCancelIDX = pThis->RecvPacket.GetInt();

				GlobalData.AllocationCancelMessgeLen = pThis->RecvPacket.GetChar();
				memset(GlobalData.AllocationCancelMessge,0x00,sizeof(GlobalData.AllocationCancelMessge));
				pThis->RecvPacket.GetString(GlobalData.AllocationCancelMessge,GlobalData.AllocationCancelMessgeLen);

				if(pThis->RecvPacket.GetChar() !=0x3 ) break;
				
				if(pThis->RecvPacket.GetChar() != CRC) break;

				szTemp.Empty();
				szTemp.Format(_T("%d %d %s "),
						AllocationCancelIDX,
						GlobalData.AllocationLatitude,
						GlobalData.AllocationCancelMessge);
				AddDebugString(szTemp);


				//전송이 완료되었다는 신호를 보냄
				pThis->RequestSend(iCase,NULL);
			}
			return true;


		case ID_ERR_STX_FIELD_FAIL:		//에러발생

			return false;
		}

	AddDebugString("━━━━┳━━━━━━");
	AddDebugString(      "\t┃");
	AddDebugString(      "\t┗━━━━━━━▶잘못된 정보가 전송됨");
	return true;
}

void WINAPI CPacketManager::PacketParser(void * pData, int iLen)
{

	CString szTemp,szTemp2;
	pThis->RecvPacket.ParserData.pData	= (unsigned char *)pData;
	pThis->RecvPacket.ParserData.TotalLen = iLen;


	bool bFail = false;

	/*
	AddDebugString("\r\nSTART!\r\n");

	::SYSTEMTIME time;
	::GetLocalTime(&time);

	szTemp.Format(_T("\r\n%4d년 %2d월%2d일 %2d시 %2d분 %2d초 %3d"),time.wYear,time.wMonth,time.wDay,time.wHour,time.wHour,time.wMinute,time.wSecond,time.wMilliseconds);
	szTemp += "\r\n------------------------START-------------------------\r\n";

	for(int i =0 ;i<iLen;i++)
	{
		szTemp2.Format(_T("%4d:[0x%x] ==> %12d %12c \r\n"),i,pThis->RecvPacket.ParserData.pData[i],pThis->RecvPacket.ParserData.pData[i],pThis->RecvPacket.ParserData.pData[i]);
		szTemp +=szTemp2;
	}

		szTemp += "\r\n-----------------------END-----------------------------\r\n";
*/

//	RunParser();

//	while(!bFail)
	{
		bFail = !RunParser();
		
/*		
		if(!bFail)
			AddDebugString("");	
*/			
	}


	memset(pData,0x00,iLen);

	AddDebugString("END!");

	return ;
}


void CPacketManager::RequestSend(int RequestType, void * pData)
{
	unsigned char buffer[256];
	int iTemp,i;

	CPacket	SendPacket;
	SendPacket.SendData.pData = buffer;
	SendPacket.SendData.iPos = 0;
	SendPacket.SendData.TotalLen = 0;
	SYSTEMTIME localTime;

	switch(RequestType)
	{
	case ID_POSITION_T2S:			//=	31,	//위치보고 
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);

			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			}

			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);

			SendPacket.SetChar(GlobalData.CarState);
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;

	case ID_ALLOCATION:				//=	32,	//배차 보고
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);


			SendPacket.SetChar(((char *)pData)[0]); //0x00 콜 요청,  0x01 콜 수락 0x02 콜 거부
			SendPacket.SetInt(GlobalData.AllocationIndex);
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			
//			SendPacket.CRC(buffer,SendPacket.SendData.TotalLen);
			//SendPacket.SetChar(0x20);

		}
		break;
	case ID_BOARDING:				//=	33,	//승차 보고
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);


			SendPacket.SetInt(GlobalData.AllocationIndex);	// 고유 배차 인덱스
			GlobalData.CarState = 0x01;						// 차량상태 실차 0x01 
			SendPacket.SetChar(GlobalData.CarState);		// 차량상태 실차 0x01 

			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			//SendPacket.SetChar(0x20);

		}
		break;
	case ID_LEAVING:				//=	34,	//하차 보고
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);


			SendPacket.SetInt(GlobalData.AllocationIndex);  // 고유 배차 인덱스
			SendPacket.SetInt(GlobalData.Distance);			// 주행거리
			SendPacket.SetInt(GlobalData.Cash);				// 주행요금

			SendPacket.SetChar(GlobalData.Payments);		// 결제방법 0x00 신용 0x01 현금 0x02 교통 0x03 후불 0x04 기타

			GlobalData.CarState = 0x00 ;					// 하차시 공차로 설정한다
			SendPacket.SetChar(GlobalData.CarState);		// 현재 자동차의 상태를 보고한다.

//			GlobalData.CarState = 0x01;						// 차량상태 실차 0x01 
		//	SendPacket.SetChar(GlobalData.CarState);		// 차량상태 실차 0x01 

			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;
	case ID_ALLOCATION_CANCEL_T2S:	//=	35,	//배차 취소 보고
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);

			SendPacket.SetInt(GlobalData.AllocationIndex);  // 고유 배차 인덱스
			SendPacket.SetChar(((char *)pData)[0]);					// 취소코드 
															// 0 다른차량 이용 
															// 1:고객이 취소 
															// 2:전와 안 받음 
															// 3:그냥 끊음 
															// 4:불량고객 
															// 5:기사 취소 
															// 6:기타 

			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;
	case ID_APPOINTMENT:			//=	36,	//예약 보고
		{

			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);

		}
		break;
	case ID_APPOINTMENT_CANCEL:		//=	37, 예약 취소 보고
		{

			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);


			SendPacket.SetChar(((char *)pData)[0]); //상태 0x00 버튼 0x01(예약 반경 오버) 0x02(예약 제한시간 초과)
			
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);

		}
		break;
	case ID_EMERGENCY:				//=	38, 긴급보고
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);

			
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;
	case ID_CONFIGT_T2S:			//=	39,  설정정보 요청
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);

			
			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;
	case ID_ALLOCATION_WAIT:		//=	40,
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);


			SendPacket.SetChar(((char *)pData)[0]);		// 내려받음 리스트 순서대로 했을 시 선택된 인덱스

			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;
	case ID_ALLOCATION_WAIT_CANCEL:	//=	41,
		{
			SendPacket.SetChar(0x02);
			SendPacket.SetCode(RequestType);
			SendPacket.SetChar(GlobalData.Terminal_IP[0]);
			SendPacket.SetChar(GlobalData.Terminal_IP[1]);
			SendPacket.SetChar(GlobalData.Terminal_IP[2]);
			SendPacket.SetChar(GlobalData.Terminal_IP[3]);
			iTemp = GlobalData.GPS_Num;
			SendPacket.SetChar(iTemp);
			for(i = 0;i<iTemp;i++)
			{
				_GPS_Format Data = GlobalData.GPS[i];
				SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
				SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
				SendPacket.SetInt(Data.Latitude);
				SendPacket.SetInt(Data.Longitude);
				SendPacket.SetChar(Data.Speed);
				SendPacket.SetShort(Data.Heading);
			
			}
			::GetLocalTime(&localTime);
			SendPacket.SetChar((unsigned char)(localTime.wYear%100));
			SendPacket.SetChar((unsigned char)localTime.wMonth);
			SendPacket.SetChar((unsigned char)localTime.wDay);
			SendPacket.SetChar((unsigned char)localTime.wHour);
			SendPacket.SetChar((unsigned char)localTime.wMinute);
			SendPacket.SetChar((unsigned char)localTime.wSecond);

			SendPacket.SetChar(0x03);
			SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
		}
		break;

		
		case ID_POSITION_S2T: //= 61   위치
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_POSITION_S2T);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);


				SendPacket.SetChar(GlobalData.GPS_Num);
				iTemp = GlobalData.GPS_Num;
				for(i = 0;i<iTemp;i++)
				{
					_GPS_Format Data = GlobalData.GPS[i];
					SendPacket.SetChar(Data.year); SendPacket.SetChar(Data.month);	SendPacket.SetChar(Data.date);
					SendPacket.SetChar(Data.hour); SendPacket.SetChar(Data.min); SendPacket.SetChar(Data.sec);
					SendPacket.SetInt(Data.Latitude);
					SendPacket.SetInt(Data.Longitude);
					SendPacket.SetChar(Data.Speed);
					SendPacket.SetShort(Data.Heading);
				
				}

				::GetLocalTime(&localTime);
				SendPacket.SetChar((unsigned char)(localTime.wYear%100));
				SendPacket.SetChar((unsigned char)localTime.wMonth);
				SendPacket.SetChar((unsigned char)localTime.wDay);
				SendPacket.SetChar((unsigned char)localTime.wHour);
				SendPacket.SetChar((unsigned char)localTime.wMinute);
				SendPacket.SetChar((unsigned char)localTime.wSecond);

				SendPacket.SetChar(GlobalData.CarState);

				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}

			break;

		case ID_TAGET_POSITION:			//=	62,	//목적지
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_TAGET_POSITION);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);

			}
			break;


		case IID_ALLOCATION_CANCEL_S2T:		//=	63,	//예약 취소
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(IID_ALLOCATION_CANCEL_S2T);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_MESSAGE:				//=	64,	//메시지 전송	
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_MESSAGE);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_CONFIG_S2T:				//=	65,	//설정
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_CONFIG_S2T);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_SERVER_INFO:			//=	66,	//서버정보
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_SERVER_INFO);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_ALLOCATION_INFO:			//=	67, //배차지령 전송
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_ALLOCATION_INFO);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_ALLOCATION_ACCEPT_TYPE1:		//=	68, //배차당첨전송(1)
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_ALLOCATION_ACCEPT_TYPE1);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);				
			}
			break;


		case ID_ALLOCATION_ACCEPT_TYPE2:		//=	69, //배차당첨전송(2)
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_ALLOCATION_ACCEPT_TYPE2);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;


		case ID_ALLOCATION_FALSE:			//=	70, //배차실패보고	
			{
				//전송이 완료되었다는 신호를 보냄
				SendPacket.SetSendData(buffer);
				SendPacket.SetChar(0x02);
				SendPacket.SetCode(ID_ALLOCATION_FALSE);
				SendPacket.SetChar(GlobalData.Terminal_IP[0]);
				SendPacket.SetChar(GlobalData.Terminal_IP[1]);
				SendPacket.SetChar(GlobalData.Terminal_IP[2]);
				SendPacket.SetChar(GlobalData.Terminal_IP[3]);
				SendPacket.SetChar(0x03);
				SendPacket.SetCRC(buffer,SendPacket.SendData.TotalLen);
			}
			break;

	}

	if(RequestType < 60)
	{

		if(!pThis->AckCheck[RequestType])
		{
			pThis->AckCheck[RequestType] = true;
			::SetTimer(hWndMain,(unsigned int)RequestType,2000,TimerProc);

			
		} else {
			pThis->AckCheck[RequestType] = false;
		//	::MessageBox(hWndMain,_T("Kill Timer!"),NULL,MB_OK);
			::KillTimer(hWndMain,(unsigned int)RequestType);
			
		}
	}

	//int				iSendCount;
	//unsigned long	LastSendTime;

	CString sztemp;

	if(GlobalData.iSendCount == 0)	GlobalData.LastSendTime = timeGetTime();

	GlobalData.iSendCount += 1;


	//sztemp.Format(_T("Send Data  %d %d"),GlobalData.iSendCount, timeGetTime() - GlobalData.LastSendTime );
	//MessageBox(hWndMain,sztemp,NULL,MB_OK);



	this->pSock->SendTo(SendPacket.SendData.pData,SendPacket.SendData.TotalLen);
}
