
#include "stdafx.h"
#include "Jumpcut.h"
#include <stdio.h>
#include "resource.h"


// Forward declarations
ATOM register_class(HINSTANCE hInstance);
BOOL init_instance(HINSTANCE, int);
LRESULT CALLBACK main_event_handler(HWND, UINT, WPARAM, LPARAM);

// Global Variables
HINSTANCE globalInstance;
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szApplicationToolTip[MAX_LOADSTRING];
BOOL bDisable = FALSE;
HWND hwndNextViewer;
HWND globalHWND;
HWND callingWindowHWND;
std::string JUMPCUT_INSTALLER_STRING = "JUMPCUT_INSTALLER";

// main entry point 
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	sprintf(JC_LOG_FILE, "%s\\.jc_log.txt", JC_USERS_HOME_DIRECTORY);
	sprintf(JC_HISTORY_FILE, "%s\\.jc_history.txt", JC_USERS_HOME_DIRECTORY);
	sprintf(JC_CONFIG_FILE, "%s\\.jc_config.txt", JC_USERS_HOME_DIRECTORY);

	globalInstance = hInstance;
	LPWSTR* szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	// This is for spawning a clone of this app on install, otherwise the installer hangs
	if (argCount == 2 && szArgList != NULL && jc_CWSTRToString(szArgList[1]) == JUMPCUT_INSTALLER_STRING) {
		jc_start_external_application(szArgList[0]);
		return 0;
	}

	// #Ensure only one version of the app 
	if (jc_is_already_running()) return 0;

	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_JUMPCUT, szWindowClass, MAX_LOADSTRING);

	register_class(hInstance);

	if (!init_instance(hInstance, nCmdShow)) return FALSE;

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

ATOM register_class(HINSTANCE hInstance) {
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = main_event_handler;
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


BOOL init_instance(HINSTANCE hInstance, int nCmdShow)
{
	HICON hMainIcon;

	globalHWND = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!globalHWND) return FALSE;

	jc_load_hotkeys(JC_CONFIG_FILE, globalHWND);
	AddClipboardFormatListener(globalHWND);
	jc_load_history_file(JC_HISTORY_FILE);

	hMainIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_JUMPCUT));

	nidApp.cbSize = sizeof(NOTIFYICONDATA);
	nidApp.hWnd = (HWND)globalHWND;
	nidApp.uID = IDI_JUMPCUT;
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nidApp.hIcon = hMainIcon;
	nidApp.uCallbackMessage = WM_USER_SHELLICON;
	LoadString(hInstance, IDS_APPTOOLTIP, nidApp.szTip, MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp);

	return TRUE;
}



BOOL CALLBACK jc_try_and_paste_to_other_app(HWND hwnd, LPARAM lParam)
{
	//if (hwnd && IsWindowVisible(hwnd)/* && IsWindowEnabled(hwnd)*/) {
	//	jc_log(hwnd_to_string(hwnd).c_str());
	//	PostMessage(hwnd, WM_PASTE, 0, 0);
	//	PostMessage(hwnd, WM_COMMAND, WM_PASTE, 0);
	//}
	return TRUE;
}


LRESULT CALLBACK main_event_handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT lpClickPoint;

	switch (message)
	{
	case WM_HOTKEY:
		callingWindowHWND = GetForegroundWindow();
		GetCursorPos(&lpClickPoint);
		hPopMenu = jc_show_popup_menu(lpClickPoint, hWnd, globalInstance, false);
		return TRUE;
		break;
	case WM_CLIPBOARDUPDATE: {
		std::string clip = jc_get_clipboard(hWnd);
		if (!clip.empty())
		{
			if (clip != JC_LAST_CLIPBOARD_ENTRY)
			{
				std::pair<bool, int> result = find_in_collection(JC_CLIPBOARD_HISTORY, clip);
				if (result.first) move_item_to_tail(JC_CLIPBOARD_HISTORY, result.second);
				else {
					if (JC_CLIPBOARD_HISTORY.size() + 1 > JC_MAX_HISTORY_SIZE) {
						JC_CLIPBOARD_HISTORY.pop_front();
					}
					JC_CLIPBOARD_HISTORY.push_back(clip);
					jc_history(clip.c_str());
					jc_log(clip.c_str());
					JC_LAST_CLIPBOARD_ENTRY = clip;
				}
			}
			else jc_log("Skipping Duplicate");
		}
		break;
	}
	case WM_DESTROY:
		ChangeClipboardChain(hWnd, hwndNextViewer);
		PostQuitMessage(0);
		break;
	case WM_CREATE:
		hwndNextViewer = SetClipboardViewer(hWnd);
		break;
	case WM_USER_SHELLICON:
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			GetCursorPos(&lpClickPoint);
			hPopMenu = jc_show_popup_menu(lpClickPoint, hWnd, globalInstance, true);
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
		case IDM_RSEARCH:
			DialogBox(globalInstance, MAKEINTRESOURCE(IDD_RSEARCH), hWnd, jc_show_rsearch_dialog);
			break;
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default: {

			int idx = wmId - JC_MENU_ID_BASE;
			if (idx >= 0 && idx < JC_CLIPBOARD_HISTORY.size()) {
				std::string item = JC_CLIPBOARD_HISTORY[idx];
				jc_set_clipboard(item, hWnd);
				SetForegroundWindow(callingWindowHWND);
				EnumChildWindows(callingWindowHWND, jc_try_and_paste_to_other_app, NULL);
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