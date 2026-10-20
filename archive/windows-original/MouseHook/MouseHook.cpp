// MouseHook.cpp : DLL 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
//#include <windows.h>
//#include <stdlib.h> 

#define BUILDDLL_EXPORTS 

#include "MouseHook.h"




/*
#pragma data_seg(".shared")

static HINSTANCE			hInstance = NULL;
static HHOOK				hKeyHook = NULL;
static HHOOK				hMouseHook = NULL;
static UINT					uTickCount = 0;

#pragma data_seg()

#pragma comment(linker, "/section:.shared,RWS")
*/

#pragma data_seg(".shared") // ".HKT" ".SHARED"
static CMouseShared gm_Shared = {0};		// API to present to the Master EXE 
static HHOOK hook;
static HWND hMainWnd;
#pragma data_seg()
#pragma comment(linker, "/section:.shared,rws") // tell the linker what this is.


BOOL APIENTRY DllMain( HINSTANCE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
           case DLL_PROCESS_ATTACH:
           case DLL_THREAD_ATTACH:
           case DLL_THREAD_DETACH:
           case DLL_PROCESS_DETACH:
                     break;
    }
    return TRUE;
 } 

// DLL안의 마우스후크 함수

BUILDDLL_API void SetMainHwnd(HWND hWnd)
{
	hMainWnd = hWnd;
}

BUILDDLL_API LRESULT CALLBACK MyMouseHook( int code, WPARAM wp, LPARAM lp )

{
    if( code < 0 )
            return CallNextHookEx( hook, code, wp, lp );
    else
    {
            switch( wp )
            {
                    case WM_LBUTTONDOWN :
                    case WM_RBUTTONDOWN :
                     {
                        MOUSEHOOKSTRUCT* pm = (MOUSEHOOKSTRUCT*)lp;
						if(::IsWindow(hMainWnd))
						{
							SendMessage( hMainWnd, WM_COMMAND, (WPARAM)WCM_UPDATE_CURPOS, MAKELPARAM(pm->pt.x,pm->pt.y));
						}
                      }
                      break;
           }
     }
     return CallNextHookEx( hook, code, wp, lp );;
} 

// 후크프로시저를 설치하는 함수
BUILDDLL_API void InstallHook(void)
{
       // DLL 명칭으로부터 모듈의 핸들을 얻습니다.
       HMODULE hmod = GetModuleHandleA("mousehook.dll");
       hook = SetWindowsHookEx( WH_MOUSE, (HOOKPROC)MyMouseHook, hmod, 0L ); 

       // hook가 NULL이면  GetLastError가 전하는 에러메시지를 확인해봅니다. 
       if( hook == NULL )
       {
       

       } 

        /*
       // thread id ( DLL안에 스레드수준의 후크함수를 두는 경우 )
       HMODULE hmod = GetModuleHandle( "hookmouse.dll");

       DWORD dw = GetCurrentThreadId();
       char str[128]; 

       // check if hmod is for dll of str
       GetModuleFileName( hmod, str, sizeof(str) );
       MessageBox( NULL, str, "Message", MB_OK );

       hook = SetWindowsHookEx( WH_MOUSE, (HOOKPROC)MyMouseHook, hmod, dw );

       if( hook == NULL )
       {
            LPVOID lpMsgBuf;
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,NULL,GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),(LPTSTR) &lpMsgBuf,0,NULL);

            MessageBox( NULL, (LPCSTR)lpMsgBuf, "Message", MB_OK );
            LocalFree( lpMsgBuf );
            exit( EXIT_FAILURE );
       }
       */
} 

BUILDDLL_API void UnInstallHook(void)
{
         if( UnhookWindowsHookEx(hook) == 0 )
         {
          
          }
} 
