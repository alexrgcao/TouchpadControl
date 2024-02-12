#pragma comment(lib, "comctl32.lib")
#include "framework.h"
#include "TouchpadControl.h"
#include <windows.h>
#include <string>
#include "TouchpadInit.h"
#include "Touchpad_Func.h"
#include <CommCtrl.h>
#include <shellapi.h>
#include <commctrl.h>

#define MAX_LOADSTRING 100

#define WM_APP_TRAYMSG (WM_APP + 1)
#define ID_TRAY_APP_CALIBRATE 3000
#define ID_TRAY_APP_PAUSE 1000
#define ID_TRAY_APP_START 1001

#define IDM_X_WIDTH 3
#define IDM_X_HEIGHT 4
#define IDM_Y_WIDTH 5
#define IDM_Y_HEIGHT 6
#define IDM_LEFT_CORNER 7
#define IDM_RIGHT_CORNER 8
HWND hwndXWidth, hwndXHeight, hwndYWidth, hwndYHeight, hwndLeftCorner, hwndRightCorner;
TouchpadFunc touchpadFunc;

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

TouchpadInit touchpadInit;

POINT minTouchpadPoint = {INT_MAX, INT_MAX};
POINT maxTouchpadPoint = {INT_MIN, INT_MIN};
#define IDM_START_CALIBRATION 1
#define IDM_STOP_CALIBRATION 2
bool isCalibrating = false;
bool isCalibrated = false;

bool state = true;

NOTIFYICONDATA nid = { sizeof(nid) };
HWND hwndButton;
HHOOK hHook = NULL;
bool isAdjusting = false;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, 
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TOUCHPADAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TOUCHPADAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TOUCHPADAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_TOUCHPADAPP);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

/*
 * This function initializes an instance of the application.
 * It determine if JSON file exist in order to initate the application correctly.
 * 
 */
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;
    state = true;
    HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        return FALSE;
    }
    nid.hWnd = hWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP_TRAYMSG;
    nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TOUCHPADAPP));
    lstrcpy(nid.szTip, _T("Touchpad App"));

    if (touchpadInit.LoadCalibrationData()) {
        isCalibrated = true;
        isCalibrating = false;
        
        double left_right_width = 0.08;
        double left_right_height = 0.75;
        double top_width = 0.75;
        double top_height = 0.09;
        double left_corner = 0.09;
        double right_corner = 0.09;

        
        touchpadFunc.set_values(touchpadInit, left_right_width, left_right_height, top_width, top_height, left_corner, right_corner);
        touchpadInit.mRegisterRawInput(hWnd);
        state = true;
    } else {
        isCalibrated = false;
    }

    if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
        return FALSE;
    }

    return TRUE;
}

/*
 * This is a low level mouse hook that is used to prevent 
 * the mouse from moving when the user is adjusting the brightness or volume.
 */
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && isAdjusting && wParam == WM_MOUSEMOVE) {
        isAdjusting = false;
        return 1;
    }

    return CallNextHookEx(hHook, nCode, wParam, lParam); 
}

/*
 * This function is the main window procedure for the application.
 * case WM_RBUTTONUP: is used to create a context menu for the application.
 * case WM_INPUT: is used to handle the input from the touchpad 
 *                including the calibrating touchpad to obtain the size and brightness/volume adjustment.
 * case WM_COMMAND: is used to handle the context menu options.
 *                  case ID_TRAY_APP_PAUSE: is used to pause the application.
 *                  case ID_TRAY_APP_START: is used to start the application.
 *                  case ID_TRAY_APP_CALIBRATE: is used to calibrate the touchpad.
 * 
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_APP_TRAYMSG:
        switch (lParam)
        {
        case WM_RBUTTONUP:
            {
                POINT pt;
                GetCursorPos(&pt);

                HMENU hMenu = CreatePopupMenu();
                if (hMenu) {
                    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_APP_CALIBRATE, _T("Calibrate"));
                    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_APP_PAUSE, _T("Pause"));
                    InsertMenu(hMenu, -1, MF_BYPOSITION, ID_TRAY_APP_START, _T("Start"));

                    InsertMenu(hMenu, -1, MF_BYPOSITION, IDM_EXIT, _T("Exit"));
                    SetForegroundWindow(hWnd);
                    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
                    DestroyMenu(hMenu);
                }
                
            }
            break;
        }
        break;
    
    case WM_INPUT:
        {
            if (!isCalibrated && isCalibrating) {
                touchpadInit.CalibrateTouchpad(hWnd, message, wParam, lParam);
            } 
            if (isCalibrated) {
                UINT size = sizeof(RAWINPUT);
                static BYTE lpb[sizeof(RAWINPUT)];
                GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &size, sizeof(RAWINPUTHEADER)); 

                RAWINPUT* raw = (RAWINPUT*)malloc(size); 

                if (raw == NULL) {
                    OutputDebugStringA("Memory allocation failed\n");
                    return 0;
                } else {
                    UINT result = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, raw, &size, sizeof(RAWINPUTHEADER));

                    if (result == (UINT)-1) {
                        DWORD error = GetLastError();
                        return;
                    } else {
                        if (state == false) {
                            return 0;
                        }
                        int x = raw->data.hid.bRawData[2] | (raw->data.hid.bRawData[3] << 8);
                        int y = raw->data.hid.bRawData[4] | (raw->data.hid.bRawData[5] << 8);

                        if (x >= touchpadFunc.x_left[0] && x <= touchpadFunc.x_left[1] && y >= touchpadFunc.x_left[2] && y <= touchpadFunc.x_left[3]) {
                            isAdjusting = true;
                            if (hHook == NULL) {
                                hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
                            }
                            int y_range = touchpadFunc.x_left[3] - touchpadFunc.x_left[2];
                            DWORD newBrightness = (touchpadFunc.x_left[3] - y) * 100 / y_range;
                            touchpadFunc.setBrightness(newBrightness);
                            if (hHook != NULL) {
                                UnhookWindowsHookEx(hHook);
                                hHook = NULL;
                            }
                            return 0;
                        } else if (x >= touchpadFunc.x_right[0] && x <= touchpadFunc.x_right[1] && y >= touchpadFunc.x_right[2] && y <= touchpadFunc.x_right[3]) {
                            isAdjusting = true;
                            if (hHook == NULL) {
                                hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, NULL, 0);
                            }
                            int y_range = touchpadFunc.x_right[3] - touchpadFunc.x_right[2];
                            int newVolume = (touchpadFunc.x_right[3] - y) * 100 / y_range; 
                            touchpadFunc.setVolume(newVolume);
                            
                            if (hHook != NULL) {
                                UnhookWindowsHookEx(hHook);
                                hHook = NULL;
                            }
                            return 0;
                        } 
                    }
                    free(raw);
                }
            } else {
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        }
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case ID_TRAY_APP_PAUSE:
                state = false;
                break;
            case ID_TRAY_APP_START:
                state = true;
                break;
            case ID_TRAY_APP_CALIBRATE:
                if (!isCalibrating) {
                    isCalibrated = false;
                    touchpadInit.mRegisterRawInput(hWnd);
                    isCalibrating = true;
                } else {
                    isCalibrating = false;
                    touchpadInit.SaveCalibrationData();
                    isCalibrated = true;

                    double left_right_width = 0.08;
                    double left_right_height = 0.75;
                    double top_width = 0.75;
                    double top_height = 0.09;
                    double left_corner = 0.09;
                    double right_corner = 0.09;

                    touchpadFunc.set_values(touchpadInit, left_right_width, left_right_height, top_width, top_height, left_corner, right_corner);
                    
                }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
