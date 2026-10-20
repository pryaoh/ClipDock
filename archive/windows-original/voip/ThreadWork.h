#pragma once
#include "WorkProcess.h"
#include "Sock_Dgram.h"

typedef unsigned (__stdcall *PTHREAD_START) (void *);

#define BEGINTHREADEX(psa, cbStack, pfnStartAddr, \
   pvParam, fdwCreate, pdwThreadId)                 \
      ((HANDLE)_beginthreadex(                      \
         (void *)        (psa),                     \
         (unsigned)      (cbStack),                 \
         (PTHREAD_START) (pfnStartAddr),            \
         (void *)        (pvParam),                 \
         (unsigned)      (fdwCreate),               \
         (unsigned *)    (pdwThreadId)))

class WorkThread
{
public:
	WorkProcess m_WorkProc;
	
	WorkThread(void);
	~WorkThread(void);

	virtual void ProcessingThread(LPVOID lParam);
	
	static DWORD __stdcall THConnectServer(PVOID pvParam);
	
	//TCHAR szError[100];   
	//WSADATA WSAData;                    // Contains details of the 
										// Winsock implementation
};
