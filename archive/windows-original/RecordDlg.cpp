// RecordDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "ScreenEncoder.h"
#include "RecordDlg.h"


// CRecordDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CRecordDlg, CDialog)

CRecordDlg::CRecordDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecordDlg::IDD, pParent)
{

}

CRecordDlg::~CRecordDlg()
{
}

void CRecordDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RECORD, m_RecordButton);
	DDX_Control(pDX, IDC_bitrate2, M_BitRate);
	DDX_Control(pDX, IDC_FRAMERATE, m_FrameRate);
	DDX_Control(pDX, IDC_SCREEN, m_ScreenSizeSel);
	DDX_Control(pDX, IDC_SPIN3, m_FrameSelSpin);
	DDX_Control(pDX, IDC_SPIN2, m_BitrateSelSpin);
	DDX_Control(pDX, IDC_CHECK2, m_bShowPreview);
	DDX_Control(pDX, IDC_ALPHA, m_AlphaSel);
}


BEGIN_MESSAGE_MAP(CRecordDlg, CDialog)
	ON_BN_CLICKED(IDOK, &CRecordDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CRecordDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_RECORD, &CRecordDlg::OnBnClickedRecord)
	ON_CBN_SELCHANGE(IDC_SCREEN, &CRecordDlg::OnCbnSelchangeScreen)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN3, &CRecordDlg::OnDeltaposSpin3)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN2, &CRecordDlg::OnDeltaposSpin2)
	ON_EN_CHANGE(IDC_FRAMERATE, &CRecordDlg::OnEnChangeFramerate)
	ON_EN_CHANGE(IDC_bitrate2, &CRecordDlg::OnEnChangebitrate2)
	ON_BN_CLICKED(IDC_CHECK2, &CRecordDlg::OnBnClickedCheck2)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_CHECK3, &CRecordDlg::OnBnClickedCheck3)
END_MESSAGE_MAP()


// CRecordDlg 메시지 처리기입니다.

void CRecordDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
//	OnOK();
}

void CRecordDlg::OnBnClickedCancel()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
//	OnCancel();
}

void	CRecordDlg::SetRecord(bool bRecord)
{
	if(bRecord)
	{
		//m_RecordMethod.EnableWindow(0);
		//this->ShowWindow(SW_MINIMIZE);
		m_RecordButton.SetWindowText("Stop");
		m_RecordButton.SetCheck(bRecord);
		//aviFile = NULL;
		//::AfxBeginThread(EncodingThreadProc,this,THREAD_PRIORITY_TIME_CRITICAL);
		//this->m_PauseButton.EnableWindow(true);
		//this->m_Close.EnableWindow(false);

		//this->M_BitRate.EnableWindow(0);
		//m_FrameRate.EnableWindow(0);
		//m_KeyFrame.EnableWindow(0);
		//m_Width.EnableWindow(0);
		//m_Height.EnableWindow(0);
		//m_ScreenSize.EnableWindow(0);
		//m_SoundRecord.EnableWindow(0);

		//sg_Shared.SetRecord(true);
		//iTotalFrameCount = 0;
		//iWaveCount = 0;
	} else {
		//sg_Shared.SetRecord(false);
		
		//if(m_SCConfig.bInit)
		{
			//m_SCConfig.bInit = false;
		}

		//this->M_BitRate.EnableWindow(1);
		//m_RecordMethod.EnableWindow(1);
		//m_SoundRecord.EnableWindow(1);
		m_RecordButton.SetWindowText("Record");
		m_RecordButton.SetCheck(bRecord);
		//this->m_PauseButton.EnableWindow(false);
		//this->m_Close.EnableWindow(true);

	}
}

void CRecordDlg::OnBnClickedRecord()
{
	
	::SendMessage(hWndMaster,WM_KEYDOWN,VK_CONTROL,(1<<30));
	::SendMessage(hWndMaster,WM_KEYDOWN,VK_F10,(1<<30));
	::SendMessage(this->hWndMaster,WM_KEYUP,VK_F10,(1<<31));
	::SendMessage(this->hWndMaster,WM_KEYUP,VK_CONTROL,(1<<31));
	
	
}

void CRecordDlg::OnCbnSelchangeScreen()
{
	int iCurSel = this->m_ScreenSizeSel.GetCurSel();
	::SendMessage(this->hWndMaster,WM_COMMAND,IDC_UPDATE_SCREEN_SIZE,iCurSel );
}



void CRecordDlg::Update()
{
	char szTemp[255];
	double AspectRatio = ((double)GetSystemMetrics(SM_CXSCREEN))/((double)GetSystemMetrics(SM_CYSCREEN));
	double fRet = fabs(AspectRatio - 4.0/3.0 );

	int iCurSel = this->m_ScreenSizeSel.GetCurSel();
	for (int i = this->m_ScreenSizeSel.GetCount()-1; i >= 0; i--)
	{
	   this->m_ScreenSizeSel.DeleteString( i );
	}

	//if(this->m_Wide.GetCheck())
	{
		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",160,(int)((double)160.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",320,(int)((double)320.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",480,(int)((double)480.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",640,(int)((double)640.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",800,(int)((double)800.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",1024,(int)((double)1024.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		memset(szTemp,0x00,sizeof(szTemp));
		sprintf_s(szTemp,"%d X %d",1280,(int)((double)1280.0/AspectRatio));
		this->m_ScreenSizeSel.AddString(szTemp);

		this->m_ScreenSizeSel.AddString("현재 해상도");
		this->m_ScreenSizeSel.AddString("사용자 정의");

	}
 /* else {
		this->bWide = this->m_Wide.GetCheck();
		
		
		this->m_ScreenSize.AddString("160 X 120");
		this->m_ScreenSize.AddString("320 X 240");
		this->m_ScreenSize.AddString("480 X 360");
		this->m_ScreenSize.AddString("640 X 480");
		this->m_ScreenSize.AddString("800 X 600");
		this->m_ScreenSize.AddString("1024 X 768");
		this->m_ScreenSize.AddString("1280 X 1024");
		this->m_ScreenSize.AddString("현재 해상도");
		this->m_ScreenSize.AddString("사용자 정의");
	}
	*/
	
	this->m_ScreenSizeSel.SetCurSel(iCurSel);
	//this->OnSelchangeScreensize();
}
BOOL CRecordDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	Update();

	m_Alpha = 70;
	this->m_AlphaSel.SetRange(0,100);
	this->m_AlphaSel.SetPos(m_Alpha);
	m_Preview.Create(m_Preview.IDD,this);
	m_Preview.ShowWindow(SW_HIDE);
	// TODO:  여기에 추가 초기화 작업을 추가합니다.

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}

void CRecordDlg::OnDeltaposSpin3(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	char buffer[255];
	this->iFrameRate -= pNMUpDown->iDelta;

	memset(buffer,0x00,sizeof(buffer));
	sprintf_s(buffer,"%d",iFrameRate);
	this->m_FrameRate.SetWindowTextA(buffer);
	
	::SendMessage(this->hWndMaster,WM_COMMAND,ID_UPDATE_FRAME_RATE,iFrameRate );

	*pResult = 0;
}

void CRecordDlg::OnDeltaposSpin2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	char buffer[255];

	iBitRate -= pNMUpDown->iDelta*1000;

	memset(buffer,0x00,sizeof(buffer));
	sprintf_s(buffer,"%d",iBitRate);
	this->M_BitRate.SetWindowTextA(buffer);
	::SendMessage(this->hWndMaster,WM_COMMAND,ID_UPDATE_BITRATE,iBitRate );

	*pResult = 0;
}

void CRecordDlg::OnEnChangeFramerate()
{
	char buffer[255];

	memset(buffer,0x00,sizeof(buffer));
	this->m_FrameRate.GetWindowTextA(buffer,255);
	sscanf(buffer,"%d",&iFrameRate);
	::SendMessage(this->hWndMaster,WM_COMMAND,ID_UPDATE_FRAME_RATE,iFrameRate );
}

void CRecordDlg::OnEnChangebitrate2()
{
	char buffer[255];
	memset(buffer,0x00,sizeof(buffer));
	this->M_BitRate.GetWindowTextA(buffer,255);
	sscanf(buffer,"%d",&iBitRate);
	::SendMessage(this->hWndMaster,WM_COMMAND,ID_UPDATE_BITRATE,iBitRate );
	// TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CRecordDlg::OnBnClickedCheck2()
{
	if(this->m_bShowPreview.GetCheck())
	{
		this->m_Preview.UnsetParent();
		this->m_Preview.SetParentWithAlpha(this->GetParent()->GetParent()->GetParent()->GetSafeHwnd());
		this->m_Preview.ChangeAlpha(100-m_Alpha);
		//m_ShowWeb.SetWindowTextA("숨기기");
	} else {
		this->m_Preview.UnsetParent();
		//m_ShowWeb.SetWindowTextA("보이기");
	}

	::SendMessage(this->hWndMaster,WM_COMMAND,ID_PREVIEW,this->m_bShowPreview.GetCheck());
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CRecordDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	if(m_Alpha != this->m_AlphaSel.GetPos())
	{
		m_Alpha = this->m_AlphaSel.GetPos();
		m_Preview.ChangeAlpha(m_Alpha);
	}
	CDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRecordDlg::OnBnClickedCheck3()
{
	/*
	HWND hWnd = ::FindWindowA(NULL,"곰플레이어");
	if(hWnd )
	{
		::SetParent(hWnd,this->GetParent()->GetParent()->GetParent()->GetSafeHwnd());
		::SetWindowLong(hWnd, GWL_STYLE,
				::GetWindowLong(hWnd, GWL_STYLE)& ~WS_POPUP |WS_CHILD);
		::SetWindowLong(hWnd, GWL_EXSTYLE,
					::GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		Sleep(100);
		::SetLayeredWindowAttributes(hWnd, 0, (255 * 60) / 100, LWA_ALPHA);
		::ShowWindow(hWnd,SW_SHOW);

		::SetWindowLong(hWnd, GWL_EXSTYLE,
		::GetWindowLong(hWnd, GWL_EXSTYLE) & ~WS_EX_LAYERED);

		::ShowWindow(hWnd,SW_HIDE);
		::SetParent(hWnd,NULL);
		::SetParent(hWnd,this->GetParent()->GetParent()->GetParent()->GetSafeHwnd());
		::SetWindowLong(hWnd, GWL_STYLE,
				::GetWindowLong(hWnd, GWL_STYLE)& ~WS_POPUP |WS_CHILD);
		Sleep(100);
		::SetWindowLong(hWnd, GWL_EXSTYLE,
					::GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

		Sleep(100);
		::SetLayeredWindowAttributes(hWnd, 0, (255 * 60) / 100, LWA_ALPHA);
		::ShowWindow(hWnd,SW_SHOW);

	}
	*/
}
