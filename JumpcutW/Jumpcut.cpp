
#include "stdafx.h"
#include "Jumpcut.h"
#include <stdio.h>
#include "resource.h"


// Forward declarations
ATOM register_class(HINSTANCE hInstance);
BOOL init_instance(HINSTANCE, int);
LRESULT CALLBACK main_event_handler(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK global_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam);
// Global Variables
NOTIFYICONDATA nidApp;
HMENU hPopMenu;
TCHAR szTitle[MAX_LOADSTRING];
TCHAR szWindowClass[MAX_LOADSTRING];
TCHAR szApplicationToolTip[MAX_LOADSTRING];
BOOL bDisable = FALSE;
HWND hwndNextViewer;
HWND callingWindowHWND;
string JUMPCUT_INSTALLER_STRING = "JUMPCUT_INSTALLER";
HHOOK g_hLowLevelKeyHook;
string JC_SINGLE_SEARCH_ITEM = "";

// main entry point 
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	snprintf(JC_LOG_FILE, PATH_STR_SIZE, "%s\\.jc_log.txt", JC_USERS_HOME_DIRECTORY);
	snprintf(JC_HISTORY_FILE, PATH_STR_SIZE, "%s\\.jc_history.txt", JC_USERS_HOME_DIRECTORY);
	snprintf(JC_CONFIG_FILE, PATH_STR_SIZE, "%s\\.jc_config.txt", JC_USERS_HOME_DIRECTORY);

	g_hLowLevelKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, global_keyboard_hook, GetModuleHandle(NULL), NULL);

	JC_INSTANCE = hInstance;
	LPWSTR* szArgList;
	int argCount;

	szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);
	// This is for spawning a clone of this app on install, otherwise the installer hangs
	if (argCount == 2 && szArgList != NULL && jc_CWSTRToString(szArgList[1]) == JUMPCUT_INSTALLER_STRING) {
		jc_start_external_application(szArgList[0]);
		return 0;
	}

	// Ensure only one version of the app 
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


BOOL init_instance(HINSTANCE hInstance, int nCmdShow) {
	HICON hMainIcon;

	JC_MAIN_WINDOW = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!JC_MAIN_WINDOW) return FALSE;

	jc_load_hotkeys_v2(JC_CONFIG_FILE, JC_MAIN_WINDOW);
	AddClipboardFormatListener(JC_MAIN_WINDOW);
	jc_load_history_file(JC_HISTORY_FILE);

	hMainIcon = LoadIcon(hInstance, (LPCTSTR)MAKEINTRESOURCE(IDI_JUMPCUT));

	nidApp.cbSize = sizeof(NOTIFYICONDATA);
	nidApp.hWnd = (HWND)JC_MAIN_WINDOW;
	nidApp.uID = IDI_JUMPCUT;
	nidApp.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nidApp.hIcon = hMainIcon;
	nidApp.uCallbackMessage = WM_USER_SHELLICON;
	LoadString(hInstance, IDS_APPTOOLTIP, nidApp.szTip, MAX_LOADSTRING);
	Shell_NotifyIcon(NIM_ADD, &nidApp);

	return TRUE;
}

// just a hackish way to intercept down key for the search dialog edit control
LRESULT CALLBACK global_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* pkbhs = (KBDLLHOOKSTRUCT*)lParam;
	if (nCode == HC_ACTION && wParam == WM_KEYUP)
	{
		//jc_alert(to_string(pkbhs->vkCode));
		if (pkbhs->vkCode == VK_DOWN) {
			HWND wnd = GetFocus();
			if (!hwnd_to_string(wnd).empty()) {
				if (JC_SEARCH_DIALOG_LIST) {
					SendMessage(JC_SEARCH_DIALOG_LIST, LB_SETCURSEL, 0, 0);
					SetFocus(JC_SEARCH_DIALOG_LIST);
				}
			}
		}
	}
	return CallNextHookEx(g_hLowLevelKeyHook, nCode, wParam, lParam);
}
BOOL CALLBACK jc_try_and_paste_to_other_app(HWND hwnd, LPARAM lParam) {
	//if (hwnd && IsWindowVisible(hwnd)/* && IsWindowEnabled(hwnd)*/) {
	//	jc_log(hwnd_to_string(hwnd).c_str());
	//	PostMessage(hwnd, WM_PASTE, 0, 0);
	//	PostMessage(hwnd, WM_COMMAND, WM_PASTE, 0);
	//}
	return TRUE;
}

INT_PTR CALLBACK jc_search_handler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);
	HWND hSearchEdit = GetDlgItem(hDlg, IDC_EDIT1);
	JC_SEARCH_DIALOG_LIST = GetDlgItem(hDlg, IDC_LIST_SEARCH_RESULTS);

	switch (message)
	{

	case WM_INITDIALOG: {
		center_window(hDlg);
		SetFocus(hSearchEdit);

		if (JC_SEARCH_DIALOG_LIST != NULL) {
			for (auto str : JC_CLIPBOARD_HISTORY) {
				SendMessage(JC_SEARCH_DIALOG_LIST, LB_ADDSTRING, 0, (LPARAM)jc_charToCWSTR(str.c_str()));
			}
		}
		return (INT_PTR)FALSE;
	}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			string str = get_selected_listbox_text(JC_SEARCH_DIALOG_LIST);
			if (!str.empty()) jc_set_clipboard(str, JC_MAIN_WINDOW);
			else {
				// If search results only has one entry then use that as the clipboard entry
				if (!JC_SINGLE_SEARCH_ITEM.empty())
				{
					jc_set_clipboard(JC_SINGLE_SEARCH_ITEM,JC_MAIN_WINDOW);
					JC_SINGLE_SEARCH_ITEM = "";
				}
			}
			ShowWindow(hDlg, SW_HIDE);
			//EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		else if (LOWORD(wParam) == IDCANCEL)
		{
			//EndDialog(hDlg, LOWORD(wParam));
			ShowWindow(hDlg, SW_HIDE);
			return (INT_PTR)TRUE;
		}

		else if (HIWORD(wParam) == EN_CHANGE)
		{
			TCHAR text[256];
			int count = 0;
			SendMessage(hSearchEdit, WM_GETTEXT, sizeof(text) / sizeof(text[0]), LPARAM(text));
			string search_term = jc_CWSTRToString(text);
			SendMessage(JC_SEARCH_DIALOG_LIST, LB_RESETCONTENT, 0, 0);
			for (auto str : JC_CLIPBOARD_HISTORY)
			{
				if (case_insensitive_find(str, search_term))
				{
					count++;
					JC_SINGLE_SEARCH_ITEM = str;
					SendMessage(JC_SEARCH_DIALOG_LIST, LB_ADDSTRING, 0, (LPARAM)jc_charToCWSTR(str.c_str()));
				}
			}
			// If there is only a single item, set this global variable 
			// so we can use it when enter is hit on the search dialog
			if (count != 1)
			{
				JC_SINGLE_SEARCH_ITEM = "";
			}
		}
		else if (HIWORD(wParam) == LBN_DBLCLK)
		{
			string str = get_selected_listbox_text(JC_SEARCH_DIALOG_LIST);
			if (!str.empty()) {
				jc_set_clipboard(str, JC_MAIN_WINDOW);
				ShowWindow(hDlg, SW_HIDE);
				//EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}

		break;
	}
	return (INT_PTR)FALSE;
}
void jc_show_search_dialog()
{
	if (JC_SEARCH_WINDOW)
	{
		ShowWindow(JC_SEARCH_WINDOW, SW_SHOW);
	}
	else
	{
		if (!DialogBox(JC_INSTANCE, MAKEINTRESOURCE(IDD_DIALOG1), JC_MAIN_WINDOW, jc_search_handler))
		{
			jc_error_and_exit(jc_charToCWSTR("rsearch"));
		}
		JC_SEARCH_WINDOW = GetForegroundWindow();
	}
}
LRESULT CALLBACK main_event_handler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (message) {
	case WM_HOTKEY:
		if (wmId == JC_MENU_HOTKEY) {
			POINT lpClickPoint;
			GetCursorPos(&lpClickPoint);
			hPopMenu = jc_show_popup_menu(lpClickPoint, hWnd, JC_INSTANCE, false);
			return TRUE;
		}
		else if (wmId == JC_SEARCH_HOTKEY)
		{
			jc_show_search_dialog();
			return TRUE;
		}
		break;
	case WM_CLIPBOARDUPDATE: {
		string clip = jc_get_clipboard(hWnd);
		if (!clip.empty() && clip != JC_LAST_CLIPBOARD_ENTRY) {
			pair<bool, int> result = find_in_collection(JC_CLIPBOARD_HISTORY, clip);
			if (result.first) move_item_to_tail(JC_CLIPBOARD_HISTORY, result.second);
			else {
				if (JC_CLIPBOARD_HISTORY.size() + 1 > JC_MAX_HISTORY_SIZE) JC_CLIPBOARD_HISTORY.pop_front();
				JC_CLIPBOARD_HISTORY.push_back(clip);
				jc_history(clip.c_str());
				JC_LAST_CLIPBOARD_ENTRY = clip;
			}
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
			POINT lpClickPoint;
			GetCursorPos(&lpClickPoint);
			hPopMenu = jc_show_popup_menu(lpClickPoint, hWnd, JC_INSTANCE, true);
			return TRUE;
		}
		break;
	case WM_COMMAND:

		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(JC_INSTANCE, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, jc_show_about_dialog);
			break;
		case IDM_RSEARCH: {
			jc_show_search_dialog();
			break;
		}
		case IDM_EXIT:
			Shell_NotifyIcon(NIM_DELETE, &nidApp);
			DestroyWindow(hWnd);
			break;
		default: {
			int idx = wmId - JC_MENU_ID_BASE;
			if (idx >= 0 && idx < JC_CLIPBOARD_HISTORY.size()) {
				string item = JC_CLIPBOARD_HISTORY[idx];
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
