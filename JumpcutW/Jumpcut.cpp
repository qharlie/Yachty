
#include "stdafx.h"
#include "Jumpcut.h"
#include <stdio.h>
#include "resource.h"
#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1


int _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow);

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);


// Global Variables:
HINSTANCE globalInstance;	// current instance
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szApplicationToolTip[MAX_LOADSTRING];	    // the main window class name
BOOL bDisable = FALSE;							// keep application state
HWND hwndNextViewer;
HWND globalHWND;



// main entry point 
int APIENTRY _tWinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	sprintf(JC_LOG_FILE, "%s\\.jc_log.txt", JC_USERS_HOME_DIRECTORY);
	sprintf(JC_HISTORY_FILE, "%s\\.jc_history.txt", JC_USERS_HOME_DIRECTORY);

	globalInstance = hInstance;
	LPWSTR* szArgList;
	int argCount;
	char buffer[500];

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	// This is for spawning a clone of this app on install, otherwise the installer hangs
	if (argCount == 2) { // And argv[1] == 'INSTALL'	
		jc_start_external_application(szArgList[0]);
		return 0;
	}

	// #Ensure only one version of the app 
	if (!jc_is_only_instance()) return 0;

	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_JUMPCUT, szWindowClass, MAX_LOADSTRING);

	MyRegisterClass(hInstance);
	//UINT_PTR timerid = SetTimer(NULL, 0, 5000, (TIMERPROC)jc_show_menu_at_current_point);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_JUMPCUT));

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
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_JUMPCUT));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = MAKEINTRESOURCE(IDC_JUMPCUT);
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HICON hMainIcon;

	globalHWND = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!globalHWND)
	{
		return FALSE;
	}
	AddClipboardFormatListener(globalHWND);
	hMainIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_JUMPCUT));

	nidApp.cbSize = sizeof(NOTIFYICONDATA); // sizeof the struct in bytes 
	nidApp.hWnd = (HWND)globalHWND;              //handle of the window which will process this app. messages 
	nidApp.uID = IDI_JUMPCUT;           //ID of the icon that willl appear in the system tray 
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
				std::pair<bool, int> result = findInCollection(JC_CLIPBOARD_HISTORY, clip);
				if (result.first) {
					moveItemToBack(JC_CLIPBOARD_HISTORY, result.second);
				}
				else {
					jc_save_history(clip.c_str());
					if (JC_CLIPBOARD_HISTORY.size() + 1 > JC_MAX_HISTORY_SIZE) {
						JC_CLIPBOARD_HISTORY.pop_front();
					}
					JC_CLIPBOARD_HISTORY.push_back(clip);
					jc_log(clip.c_str());
					JC_LAST_CLIPBOARD_ENTRY = clip;
				}
			}
			else jc_log("Skipping Duplicate");
		}
		else jc_error_and_exit(TEXT("ERROR ACCESS DENIED TO OPENCLIPBOARD, PLEASE REPORT THIS!"));

		break;
	}
	case WM_CHANGECBCHAIN:
		if ((HWND)wParam == hwndNextViewer)
			hwndNextViewer = (HWND)lParam;
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
			GetCursorPos(&lpClickPoint);
			hPopMenu = jc_show_popup_menu(lpClickPoint, hWnd, globalInstance);
			return TRUE;
		}
		break;
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(globalInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, jc_show_about_dialog);
			break;
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default: {

			int BASE = 2000;
			int idx = wmId - BASE;
			if (idx >= 0 && idx < JC_CLIPBOARD_HISTORY.size()) {
				std::string item = JC_CLIPBOARD_HISTORY[idx];
				jc_set_clipboard(item, hWnd);
			}
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		}
		break;


	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}