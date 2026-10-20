; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CScreenEncoderDlg
LastTemplate=CWinThread
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "ScreenEncoder.h"

ClassCount=3
Class1=CScreenEncoderApp
Class2=CScreenEncoderDlg

ResourceCount=2
Resource1=IDR_MAINFRAME
Class3=CEncodingThread
Resource2=IDD_SCREENENCODER_DIALOG

[CLS:CScreenEncoderApp]
Type=0
HeaderFile=ScreenEncoder.h
ImplementationFile=ScreenEncoder.cpp
Filter=N

[CLS:CScreenEncoderDlg]
Type=0
HeaderFile=ScreenEncoderDlg.h
ImplementationFile=ScreenEncoderDlg.cpp
Filter=D
BaseClass=CDialog
VirtualFilter=dWC
LastObject=IDC_PAUSE



[DLG:IDD_SCREENENCODER_DIALOG]
Type=1
Class=CScreenEncoderDlg
ControlCount=13
Control1=IDOK,button,1342242817
Control2=IDC_RECORD,button,1342242816
Control3=IDC_SCREENSIZE,combobox,1344339970
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,button,1342177287
Control6=IDC_WIDTH,edit,1082204288
Control7=IDC_HEIGHT,edit,1082204288
Control8=IDC_CROSS1,static,1073872896
Control9=IDC_STATIC,static,1342308352
Control10=IDC_KEYFRAME,edit,1350639744
Control11=IDC_STATIC,static,1342308352
Control12=IDC_FRAMERATE,edit,1350639744
Control13=IDC_PAUSE,button,1342242816

[CLS:CEncodingThread]
Type=0
HeaderFile=EncodingThread.h
ImplementationFile=EncodingThread.cpp
BaseClass=CWinThread
Filter=N

