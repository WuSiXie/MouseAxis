// Monitor Force Feedback (FFB) vJoy device
#include "stdafx.h"
//#include "Devioctl.h"
#include "public.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include "vjoyinterface.h"
#include "Math.h"
// Default device ID (Used when ID not specified)
#define DEV_ID		1
UINT DevID = DEV_ID;
PVOID pPositionMessage;
TCHAR statusTextBuffer[100];
BYTE id = 1;

// Prototypes
void  CALLBACK FfbFunction(PVOID data);
void  CALLBACK FfbFunction1(PVOID cb, PVOID data);

BOOL PacketType2Str(FFBPType Type, LPTSTR Str);
BOOL EffectType2Str(FFBEType Ctrl, LPTSTR Str);
BOOL DevCtrl2Str(FFB_CTRL Type, LPTSTR Str);
BOOL EffectOpStr(FFBOP Op, LPTSTR Str);
int  Polar2Deg(BYTE Polar);
int  Byte2Percent(BYTE InByte);
int TwosCompByte2Int(BYTE in);


int ffb_direction = 0;
int ffb_strenght = 0;
int serial_result = 0;


JOYSTICK_POSITION_V2 iReport; // The structure that holds the full position data



#include <windows.h>
#include <gdiplus.h>
using namespace Gdiplus;

POINT mousePosition;
int prevx;
int prevy;
// 全局变量，存储十字的长度
int crossLength = 100;
HHOOK g_hook;
bool DrawSmallCross = true;
HWND hWnd;
UINT8 MasterKeys[] = { VK_DELETE,VK_RSHIFT,VK_RCONTROL };
bool MasterSwitch = true;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        if (wParam == WM_KEYDOWN) {
            KBDLLHOOKSTRUCT* pKeyStruct = (KBDLLHOOKSTRUCT*)lParam;
            if (pKeyStruct->vkCode == VK_ADD) {
                crossLength += 10;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (pKeyStruct->vkCode == VK_SUBTRACT) {
                crossLength -= 10;
                InvalidateRect(hWnd, NULL, TRUE);
            }
            else if (pKeyStruct->vkCode == VK_INSERT)
            {
                DrawSmallCross = !DrawSmallCross;
                if(DrawSmallCross and hWnd)
                    SetTimer(hWnd, 1, 10, NULL);
            }
            else if (pKeyStruct->vkCode == MasterKeys[0])
            {
                bool breakused = false;
                for (int i = 1; i<sizeof(MasterKeys); i++)
                {
                    if (!(GetAsyncKeyState(MasterKeys[i]) & 0x8000))
                        breakused = true;
                        break;
                }
                if (!breakused)
                {
                    MasterSwitch = !MasterSwitch;
                    if (MasterSwitch)
                    {
                        DrawSmallCross = false;
                        ShowWindow(hWnd, SW_SHOW);
                    }
                    else
                        ShowWindow(hWnd, SW_HIDE);
                }
            }
        }
    }

    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        if (wParam == 1) {
            // 获取窗口DC
            HDC hdc = GetDC(hwnd);
            if (hdc) {
                // 获取窗口尺寸
                RECT rect;
                GetClientRect(hwnd, &rect);
                int width = rect.right - rect.left;
                int height = rect.bottom - rect.top;

                // 创建GDI+绘图对象
                Graphics graphics(hdc);

                // 获取鼠标位置
                POINT mousePosition;
                GetCursorPos(&mousePosition);

                // 限制小十字位置在大十字的范围内
                int crossLeft = width / 2 - crossLength / 2;
                int crossTop = height / 2 - crossLength / 2;
                int crossRight = width / 2 + crossLength / 2;
                int crossBottom = height / 2 + crossLength / 2;

                if (mousePosition.x < crossLeft)
                    mousePosition.x = crossLeft;
                else if (mousePosition.x > crossRight)
                    mousePosition.x = crossRight;

                if (mousePosition.y < crossTop)
                    mousePosition.y = crossTop;
                else if (mousePosition.y > crossBottom)
                    mousePosition.y = crossBottom;

                // 创建背景色画刷
                HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
                if (hBrush) 
                {
                    // 擦除之前的小十字
                    if (prevx and prevy)
                    {
                        RECT eraseRect = { prevx - 11, prevy - 11, prevx + 11, prevy + 11 };
                        FillRect(hdc, &eraseRect, hBrush);
                    }
                    Pen pen(Color(255, 255, 255, 255));
                    if (MasterSwitch)
                    {
                        if (!DrawSmallCross)
                        {
                            iReport.wAxisX = 16384;
                            iReport.wAxisY = 16384;
                            KillTimer(hwnd, 1);
                        }
                        else
                        {
                            iReport.wAxisX = (mousePosition.x - (width/2) + (crossLength/2))*32767/crossLength;
                            iReport.wAxisY = (mousePosition.y - (height/2) + (crossLength/2))*32767/crossLength;
                            // 绘制新的小十字
                            graphics.DrawLine(&pen, static_cast<INT>(mousePosition.x) - 10, static_cast<INT>(mousePosition.y), static_cast<INT>(mousePosition.x) + 10, static_cast<INT>(mousePosition.y));
                            graphics.DrawLine(&pen, static_cast<INT>(mousePosition.x), static_cast<INT>(mousePosition.y) - 10, static_cast<INT>(mousePosition.x), static_cast<INT>(mousePosition.y) + 10);
                       }
                        prevx = mousePosition.x;
                        prevy = mousePosition.y;
                        graphics.DrawLine(&pen, width / 2 - crossLength / 2, height / 2, width / 2 + crossLength / 2, height / 2);
                        graphics.DrawLine(&pen, width / 2, height / 2 - crossLength / 2, width / 2, height / 2 + crossLength / 2);
                    }
                    else
                    {
                        iReport.wAxisX = 16384;
                        iReport.wAxisY = 16384;
                        KillTimer(hwnd, 1);
                        ShowWindow(hwnd, SW_HIDE);
                    }
                    // 释放画刷
                    DeleteObject(hBrush);
                }
                id = (BYTE)DevID;
                iReport.bDevice = id;
                pPositionMessage = (PVOID)(&iReport);
                UpdateVJD(DevID, pPositionMessage);
                // 释放窗口DC
                ReleaseDC(hwnd, hdc);
            }
        }
        break;



    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // 获取窗口尺寸
        RECT rect;
        GetClientRect(hwnd, &rect);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        // 创建GDI+绘图对象
        Graphics graphics(hdc);

        // 设置透明背景
        graphics.Clear(Color(0, 0, 0, 0));

        // 绘制十字
        Pen pen(Color(255, 255, 255, 255));
        graphics.DrawLine(&pen, width / 2 - crossLength / 2, height / 2, width / 2 + crossLength / 2, height / 2);
        graphics.DrawLine(&pen, width / 2, height / 2 - crossLength / 2, width / 2, height / 2 + crossLength / 2);
        // 计算大十字的边界
        int crossLeft = width / 2 - crossLength / 2;
        int crossTop = height / 2 - crossLength / 2;
        int crossRight = width / 2 + crossLength / 2;
        int crossBottom = height / 2 + crossLength / 2;

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEACTIVATE:
        SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        return MA_NOACTIVATE;
    /*case WM_KEYDOWN:
        // 按下小键盘+增加长度，按下小键盘-减少长度
        if (wParam == VK_ADD)
        {
            crossLength += 10;
            //MessageBox(hwnd, L"按下了小键盘+", L"按键消息", MB_OK);
        }
        else if (wParam == VK_SUBTRACT)
        {
            crossLength -= 10;
            //MessageBox(hwnd, L"按下了小键盘-", L"按键消息", MB_OK);
        }
        // 限制十字的最小长度为10
        if (crossLength < 10)
            crossLength = 10;

        // 重新绘制窗口
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;*/
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
    int stat = 0;
    USHORT X = 0;
    USHORT Y = 0;
    USHORT Z = 0;
    LONG   Btns = 0;


    UINT	IoCode = LOAD_POSITIONS;
    UINT	IoSize = sizeof(JOYSTICK_POSITION);
    // HID_DEVICE_ATTRIBUTES attrib;
    UINT iInterface = 1;

    // Define the effect names
    static FFBEType FfbEffect = (FFBEType)-1;
    LPCTSTR FfbEffectName[] ={ "NONE", "Constant Force", "Ramp", "Square", "Sine", "Triangle", "Sawtooth Up","Sawtooth Down", "Spring", "Damper", "Inertia", "Friction", "Custom Force" };
    if (!vJoyEnabled())
    {
        MessageBox(NULL, "无法和Vjoy驱动程序建立连接\n请尝试重新安装驱动程序", "无法运行", MB_ICONERROR);
        int dummy = getchar();
        stat = -2;
        return 0;
    }
TryGetVJDStatus:
    VjdStat status = GetVJDStatus(DevID);
    switch (status)
    {
    case VJD_STAT_OWN:
        wsprintf(statusTextBuffer, TEXT("已和 %d 号设备建立连接"),DevID);
        MessageBox(NULL, statusTextBuffer, "提示", MB_OK);
        break;
    case VJD_STAT_FREE:
        break;
    case VJD_STAT_BUSY:
        DevID += 1;
        goto TryGetVJDStatus;
    case VJD_STAT_MISS:
        MessageBox(NULL, "没有空闲的Vjoy虚拟设备\n请尝试添加设备或解除占用", "无法运行", MB_ICONERROR);
        return -4;
    default:
        MessageBox(NULL, "Vjoy出现未知错误", "无法运行", MB_ICONERROR);
        return -1;
    };
    if (!AcquireVJD(DevID))
    {
        wsprintf(statusTextBuffer, TEXT("无法和 %d 号设备建立连接\n正在尝试其他设备"), DevID);
        MessageBox(NULL, statusTextBuffer, "提示", MB_OK);
        int dummy = getchar();
        DevID += 1;
        goto TryGetVJDStatus;
    }
    else
    {
        wsprintf(statusTextBuffer, TEXT("和 %d 号设备成功建立连接"), DevID);
        MessageBox(NULL, statusTextBuffer, "提示", MB_OK);
    }
    
    BOOL Ffbstarted = FfbStart(DevID);
    if (!Ffbstarted)
    {
        wsprintf(statusTextBuffer, TEXT("无法开启 %d 号设备Ffb\n正在尝试其他设备"), DevID);
        MessageBox(NULL, statusTextBuffer, "提示", MB_OK);
        int dummy = getchar();
        DevID += 1;
        goto TryGetVJDStatus;
    }
    else
    {
        wsprintf(statusTextBuffer, TEXT("开启 %d 号设备Ffb成功"), DevID);
        MessageBox(NULL, statusTextBuffer, "提示", MB_OK);
    }
    
    id = (BYTE)DevID;
    iReport.bDevice = id;
    iReport.wAxisX = 16384;
    iReport.wAxisY = 16384;


    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    // 初始化GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 注册窗口类
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "StupidKaisr";
    RegisterClass(&wc);

    // 获取屏幕大小
    RECT desktopRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &desktopRect, 0);
    int screenWidth = desktopRect.right - desktopRect.left;
    int screenHeight = desktopRect.bottom - desktopRect.top;

    // 计算窗口大小
    int windowWidth = screenWidth ;
    int windowHeight = screenHeight ;

    // 创建窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
        "StupidKaisr",
        "凯锶大笨蛋",
        WS_POPUP,
        (screenWidth - windowWidth) , (screenHeight - windowHeight) , // 窗口位置居中
        windowWidth, windowHeight, // 窗口大小
        NULL, NULL, NULL, NULL);
    hWnd = hwnd;
    // 创建定时器，每10毫秒触发一次
    SetTimer(hwnd, 1, 10, NULL);

    // 设置窗口透明
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    /*// 将窗口移动到屏幕中心
    RECT desktopRect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &desktopRect, 0);
    int desktopWidth = desktopRect.right - desktopRect.left;
    int desktopHeight = desktopRect.bottom - desktopRect.top;
    RECT windowRect;
    GetWindowRect(hwnd, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    int posX = (desktopWidth - windowWidth) / 2;
    int posY = (desktopHeight - windowHeight) / 2;
    SetWindowPos(hwnd, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    */

    // 显示窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 关闭GDI+
    GdiplusShutdown(gdiplusToken);

    UnhookWindowsHookEx(g_hook);
Exit:
    RelinquishVJD(DevID);
    return 0;
}
