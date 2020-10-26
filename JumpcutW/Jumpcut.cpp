
#include "stdafx.h"
#include "Jumpcut.h"
#include <stdio.h>

#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1

// Global Variables:
HINSTANCE hInst;	// current instance
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szApplicationToolTip[MAX_LOADSTRING];	    // the main window class name
BOOL bDisable = FALSE;							// keep application state

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
HWND hwndNextViewer;

char* JC_USERS_HOME_DIRECTORY = getenv("USERPROFILE");

//FixedQueue<std::string, 50> JC_CLIPBOARD_HISTORY;


int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	sprintf(JC_LOG_FILE, "%s\\jc_search_history.txt", JC_USERS_HOME_DIRECTORY);
	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SYSTRAYDEMO, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SYSTRAYDEMO));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SYSTRAYDEMO));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_SYSTRAYDEMO);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	HICON hMainIcon;

	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}
	AddClipboardFormatListener(hWnd);
	hMainIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_SYSTRAYDEMO));

	nidApp.cbSize = sizeof(NOTIFYICONDATA); // sizeof the struct in bytes 
	nidApp.hWnd = (HWND)hWnd;              //handle of the window which will process this app. messages 
	nidApp.uID = IDI_SYSTRAYDEMO;           //ID of the icon that willl appear in the system tray 
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; //ORing of all the flags 
	nidApp.hIcon = hMainIcon; // handle of the Icon to be displayed, obtained from LoadIcon 
	nidApp.uCallbackMessage = WM_USER_SHELLICON;
	LoadString(hInstance, IDS_APPTOOLTIP, nidApp.szTip, MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp);

	return TRUE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT lpClickPoint;

	switch (message)
	{

	case WM_CLIPBOARDUPDATE: {
		std::string clip = jc_get_clipboard(hWnd);
		if (!clip.empty())
		{
			if (clip != JC_LAST_CLIPBOARD_ENTRY)
			{
				JC_CLIPBOARD_HISTORY.push_back(clip);
				JC_LAST_CLIPBOARD_ENTRY = clip;
			}
			else jc_appendToLog("Skipping Duplicate");
		}
		else {
			jc_error_and_exit(TEXT("ERROR ACCESS DENIED TO OPENCLIPBOARD"));
		}
		break;
	}
	case WM_CHANGECBCHAIN:

		// If the next window is closing, repair the chain. 

		if ((HWND)wParam == hwndNextViewer)
			hwndNextViewer = (HWND)lParam;

		// Otherwise, pass the message to the next link. 

		else if (hwndNextViewer != NULL)
			SendMessage(hwndNextViewer, message, wParam, lParam);

		break;

	case WM_DESTROY:
		ChangeClipboardChain(hWnd, hwndNextViewer);
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		hwndNextViewer = SetClipboardViewer(hWnd);
		break;
	case WM_USER_SHELLICON:
		// systray msg callback 
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			UINT uFlag = MF_BYPOSITION | MF_STRING;
			GetCursorPos(&lpClickPoint);
			hPopMenu = CreatePopupMenu();
			int BASE = 2000;
			for (int i = 0; i < JC_CLIPBOARD_HISTORY.size(); i++)
			{
				std::string item = JC_CLIPBOARD_HISTORY[i];
				if (!item.empty())
					InsertMenu(hPopMenu, 0xFFFFFFFF, uFlag, i + BASE, jc_charToCWSTR(trim(item).c_str()));
			}
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About JumpcutW"));
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit  JumpcutW"));

			SetForegroundWindow(hWnd);
			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
			return TRUE;

		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_DISABLE:
			bDisable = TRUE;
			break;
		case IDM_ENABLE:
			bDisable = FALSE;
			break;
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default: {

			char msg[100];
			int BASE = 2000;
			int idx = wmId - BASE;
			std::string item = JC_CLIPBOARD_HISTORY[idx];
			jc_set_clipboard(item);
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		}
		break;
		/*
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;*/

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
