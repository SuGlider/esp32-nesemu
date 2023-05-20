#pragma once

#include <windows.h>
#include <gdiplus.h>
//#include "resource.h"
#define WM_FLUSHSCREEN (WM_USER + 1)
extern DWORD* g_pScreenBuffer;
extern HWND g_hWnd;

#define delay Sleep

#ifdef _M_AMD64
#error 由于指针截断，不要用64位编译
#endif