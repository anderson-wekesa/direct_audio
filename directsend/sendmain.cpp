#pragma comment (lib, "Dsound.lib")
#pragma comment (lib, "Dxguid.lib")
#pragma comment (lib, "Ws2_32.Lib")

#include "sendmain.h"


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HWND hMainWindow;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{

    const wchar_t CLASS_NAME[] = L"DSSend";

    WNDCLASS wc = {};
    wc.hInstance = hInstance;
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc))
    {
        MessageBoxW(NULL, L"Failed to Register Class!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }


    hMainWindow = CreateWindowEx(
        0,
        CLASS_NAME,
        L"DS Sender",
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 650, 250,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hMainWindow)
    {
        MessageBoxW(NULL, L"Failed to Create Window!", L"Error", MB_OK | MB_ICONERROR);
        return 0;
    }


    ShowWindow(hMainWindow, nCmdShow);

    MSG msg = {};

    while (GetMessage(&msg, hMainWindow, NULL, NULL) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    return 0;
}


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
		case WM_PAINT:
		{
			PAINTSTRUCT ps = {};
			HDC hdc = BeginPaint(hWnd, &ps);

			FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW));

			EndPaint(hWnd, &ps);
		}
		break;

		case WM_CREATE:
		{
			InitializeAudio();	
			Initialize(hWnd);
		}
		break;

		case WM_COMMAND:
		{
			switch(wParam)
			{
				case BTN_CONNECT:
				{
					DirectConnect();
				}
				break;

				case BTN_DISCONNECT:
				{
					DirectDisconnect();
				}
				break;
			}
		}
		break;

		case WM_CLOSE:
		{
			CleanUp();
			DestroyWindow(hWnd);
		}
		break;

    }



    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

