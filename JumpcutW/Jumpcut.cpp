
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
void jc_show_popup_menu(POINT& lpClickPoint, const HWND& hWnd);
INT_PTR CALLBACK	jc_show_about_dialog(HWND, UINT, WPARAM, LPARAM);
HWND hwndNextViewer;
HWND globalHWND;

char* JC_USERS_HOME_DIRECTORY = getenv("USERPROFILE");
int JC_MAX_MENU_LABEL_LENGTH = 65;

VOID startup(LPCTSTR lpApplicationName)
{
	// additional information
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	// set the size of the structures
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// start the program up
	CreateProcess(lpApplicationName,   // the path
		NULL,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}
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
	
	LPWSTR* szArgList;
	int argCount;
	char buffer[500];

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	if (argCount == 2)
	{
		startup(szArgList[0]);
		return 0;
	}

	// #Ensure only one version of the app 
	HANDLE hMutex = OpenMutex(
		MUTEX_ALL_ACCESS, 0, jc_charToCWSTR(JC_APPLICATION_NAME));

	if (!hMutex)
		hMutex =
		CreateMutex(0, 0, jc_charToCWSTR(JC_APPLICATION_NAME));
	else
		return 0;

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

	hInst = hInstance; // Store instance handle in our global variable

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
					jc_appendToHistory(clip.c_str());
					if (JC_CLIPBOARD_HISTORY.size() + 1 > JC_MAX_HISTORY_SIZE)
					{
						JC_CLIPBOARD_HISTORY.pop_front();
					}
					JC_CLIPBOARD_HISTORY.push_back(clip);
					char buf[1024];
					sprintf(buf, "CLIP_HISTORY_SIZE=%d", JC_CLIPBOARD_HISTORY.size());
					jc_log(buf);
					jc_log(clip.c_str());
					JC_LAST_CLIPBOARD_ENTRY = clip;
				}
			}
			else jc_log("Skipping Duplicate");
		}
		else jc_error_and_exit(TEXT("ERROR ACCESS DENIED TO OPENCLIPBOARD"));

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
			jc_show_popup_menu(lpClickPoint, hWnd);
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
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, jc_show_about_dialog);
			break;
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default: {

			int BASE = 2000;
			int idx = wmId - BASE;
			if (idx >= 0 && idx < JC_CLIPBOARD_HISTORY.size())
			{
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
VOID CALLBACK jc_show_menu_at_current_point(
	HWND hwnd,        // handle to window for timer messages 
	UINT message,     // WM_TIMER message 
	UINT idTimer,     // timer identifier 
	DWORD dwTime)     // current system time 
{

	RECT rc;
	POINT pt;

	// If the window is minimized, compare the current 
	// cursor position with the one from 10 seconds earlier. 
	// If the cursor position has not changed, move the 
	// cursor to the icon. 

	if (GetCursorPos(&pt))
	{
		jc_show_popup_menu(pt, globalHWND);
	}
	else jc_log("Failed to GetCursorPos()");
}

void jc_show_popup_menu(POINT& lpClickPoint, const HWND& hWnd)
{
	UINT uFlag = MF_BYPOSITION | MF_STRING;
	//GetCursorPos(&lpClickPoint);
	hPopMenu = CreatePopupMenu();
	int BASE = 2000;
	for (int i = 0; i < JC_CLIPBOARD_HISTORY.size(); i++)
	{
		std::string item = JC_CLIPBOARD_HISTORY[i];
		std::string label = trim(item);
		label.resize(min(item.length(), JC_MAX_MENU_LABEL_LENGTH));
		if (label.length() == JC_MAX_MENU_LABEL_LENGTH)
		{
			label += "...";
		}
		if (!item.empty())
			InsertMenu(hPopMenu, 0xFFFFFFFF, uFlag, i + BASE, jc_charToCWSTR(label.c_str()));
	}
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About JumpcutW"));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit  JumpcutW"));

	SetForegroundWindow(hWnd);
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);

}

// Message handler for about box.
INT_PTR CALLBACK jc_show_about_dialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
