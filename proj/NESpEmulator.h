#pragma once

#include <windows.h>
#include <gdiplus.h>
//#include "resource.h"
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define WM_FLUSHSCREEN (WM_USER + 1)
extern DWORD* g_pScreenBuffer;
extern HWND g_hWnd;

#ifdef _M_AMD64
#error ����ָ��ضϣ���Ҫ��64λ����
#endif