// hookmouse.h

#ifndef __HOOKMOUSE_H
#define __HOOKMOUSE_H
#define BUILDDLL_EXPORTS 
#define WCM_UPDATE_CURPOS (WM_APP+0x05)

#ifdef BUILDDLL_EXPORTS
#define BUILDDLL_API __declspec(dllexport)
#else
#define BUILDDLL_API __declspec(dllimport)
#endif 
#pragma pack(4)
struct BUILDDLL_API CMouseShared
{
public:
	DWORD hWnd;

};
extern BUILDDLL_API CMouseShared gm_Shared;
#pragma pack()

// 후크 프로시저
BUILDDLL_API LRESULT CALLBACK MyMouseHook( int nCode, WPARAM wParam, LPARAM lParam ); 

// 후크프로시저를 설치하는 함수
BUILDDLL_API void InstallHook(void); 

BUILDDLL_API void SetMainHwnd(HWND hWnd); 

// 메모리에 설치된 후크 프로시저를 해제하는 함수
BUILDDLL_API void UnInstallHook(void);
 
 #endif 

