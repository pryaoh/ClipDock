//
// stdafx.h
// included in both EXE and DLL
//
#if _MSC_VER > 1000
#pragma once
#endif
#define USE_DX	// remove this to compile if u dont have the DirectX SDK
#define USE_LOGFILE

// NOTE: This causes a hang bug when closing Taksi before closing the app!
// #define USE_GDIP	// use Gdiplus::Bitmap to save images as PNG (or JPEG)

// System includes always go first.

//#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers


#include <windows.h>

#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include "./dll/taksi_global.h"
