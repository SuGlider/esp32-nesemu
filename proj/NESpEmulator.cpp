// NesEmulator.cpp : 定义应用程序的入口点。
//

//#include "framework.h"
#include "NESpEmulator.h"
//#include "Emulator.h"
#include <shellapi.h>
#include <stdio.h>

#define MAX_LOADSTRING 100
#define SAFE_DELETE(p) if(p) { delete p; p = NULL; }

using namespace Gdiplus;
#pragma comment(lib, "gdiplus.lib")

// 全局变量:
HINSTANCE hInst;                                       // 当前实例
WCHAR szTitle[MAX_LOADSTRING] = L"NESpEmulator";       // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING] = L"NESpEmulator"; // 主窗口类名

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
HWND                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

Bitmap* g_pScreen = NULL;
HWND g_hWnd;

int nofrendo_main(int argc, char* argv[]);
DWORD WINAPI GameboyProc(LPVOID lpThreadParameter);
void KeyCallback(int key, int action);
DWORD g_idThread = 0;
DWORD* g_pScreenBuffer = NULL;

// 输出尺寸
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240


int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    g_pScreen = new Bitmap(SCREEN_WIDTH, SCREEN_HEIGHT, PixelFormat32bppARGB);
    g_pScreenBuffer = new DWORD[SCREEN_WIDTH * SCREEN_HEIGHT];
#if 0
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(lpCmdLine, &argc);
    char buffer[MAX_PATH] = {0};
    WideCharToMultiByte(CP_ACP, 0, argv[0], lstrlenW(argv[0]), buffer, MAX_PATH, NULL, NULL);
    if (argc == 1)
    {
        FILE* fp = fopen(buffer, "rb");
        if (fp)
        {
            fseek(fp, 0, 2);
            int size = ftell(fp);
            fseek(fp, 0, 0);
            unsigned char* ptr = new unsigned char[size];
            fread(ptr, size, 1, fp);
            fclose(fp);

            fp = fopen("dump.txt", "wb");
            for (int i = 0; i < size; i++)
            {
                if (i > 0 && (i & 0xf) == 0)
                {
                    fprintf(fp, "\n");
                }
                fprintf(fp, "0x%02x, ", ptr[i]);
            }
            fclose(fp);

            delete[] ptr;
        }
        return 1;
    }
#endif

    // TODO: 在此处放置代码。
    //Emulator emulator(buffer);
    //emulator.init();

    HANDLE hThread = CreateThread(NULL, 0, GameboyProc, NULL, 0, &g_idThread);
    CloseHandle(hThread);

    // 初始化全局字符串
    //LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    //LoadStringW(hInstance, IDC_NESEMULATOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    HWND hWnd = InitInstance(hInstance, nCmdShow);
    if (!hWnd)
    {
        return FALSE;
    }

    //SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)&emulator);

    //HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NESEMULATOR));

    MSG msg;

    // 主消息循环:
    while (true)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                //emulator.exit();
                break;
            }

            // if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else {
            //emulator.loop();
        }
    }

    SAFE_DELETE(g_pScreenBuffer);
    SAFE_DELETE(g_pScreen);
    GdiplusShutdown(gdiplusToken);

    return (int) msg.wParam;
}

void Flush(const DWORD* pSource)
{
    Gdiplus::BitmapData bd = { 0 };
    Gdiplus::Status s = g_pScreen->LockBits(NULL, Gdiplus::ImageLockModeWrite, PixelFormat32bppRGB, &bd);
    int width = g_pScreen->GetWidth();
    int height = g_pScreen->GetHeight();
    if (s == Gdiplus::Ok)
    {
        DWORD* pColor;
        //for (int y = 0; y < height; y++) {
        //    int index = y * width;
        //    pColor = (DWORD*)((INT_PTR)bd.Scan0 + y * bd.Stride);

        //    for (int x = 0; x < width; x++, index++, pColor++, pSource++) {
        //        DWORD d = *pSource;
        //        d = ((d & 0xff0000) >> 16) | ((d & 0xff) << 16) | (d & 0xff00ff00);
        //        *pColor = d;
        //    }
        //}

        for (int x = width - 1; x >= 0; x--)
        {
            pColor = (DWORD*)((INT_PTR)bd.Scan0 + x * sizeof(DWORD) + (height - 1) * bd.Stride);

            for (int y = 0; y < height; y++, pSource++)
            {
                DWORD d = *pSource;
                d = ((d & 0xff0000) >> 16) | ((d & 0xff) << 16) | (d & 0xff00ff00);
                *pColor = d;
                pColor = (DWORD*)((INT_PTR)pColor - bd.Stride);

            }
        }

        g_pScreen->UnlockBits(&bd);
    }

    InvalidateRect(g_hWnd, NULL, FALSE);
}

DWORD WINAPI GameboyProc(LPVOID lpThreadParameter)
{
    nofrendo_main(0, NULL);
    return 0;
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = NULL;// LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NESEMULATOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL; //MAKEINTRESOURCEW(IDC_NESEMULATOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = NULL;// LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 400, 400, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   g_hWnd = hWnd;

   return hWnd;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    // case WM_COMMAND:
    //     {
    //         int wmId = LOWORD(wParam);
    //         // 分析菜单选择:
    //         switch (wmId)
    //         {
    //         case IDM_ABOUT:
    //             DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
    //             break;
    //         case IDM_EXIT:
    //             DestroyWindow(hWnd);
    //             break;
    //         default:
    //             return DefWindowProc(hWnd, message, wParam, lParam);
    //         }
    //     }
    //     break;
    case WM_KEYDOWN:
    {
      //Emulator* pEmulator = reinterpret_cast<Emulator*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      KeyCallback(wParam, WM_KEYDOWN);
    }
      break;

    //case WM_CHAR:
    //  break;

    case WM_KEYUP:
    {
      //Emulator* pEmulator = reinterpret_cast<Emulator*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
      KeyCallback(wParam, WM_KEYUP);
    }
    break;

    case WM_FLUSHSCREEN:
        Flush(g_pScreenBuffer);
        break;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT rect;
        GetClientRect(hWnd, &rect);
        HDC hdc = BeginPaint(hWnd, &ps);

        Gdiplus::Graphics g(hdc);
        // g.SetInterpolationMode(InterpolationModeNearestNeighbor);
        const int width = SCREEN_WIDTH;
        const int height = -SCREEN_HEIGHT;
        g.DrawImage(g_pScreen, Rect((rect.right - rect.left - width) / 2, (rect.bottom - rect.top - height) / 2, width, height), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, UnitPixel);
        // TODO: 在此处添加使用 hdc 的任何绘图代码...
        EndPaint(hWnd, &ps);
    } break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
