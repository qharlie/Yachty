
#include "stdafx.h"
#include "Jumpcut.h"
#include <stdio.h>
#include "resource.h"
#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1
#define JC_HOTKEY 9000

// Forward declarations
ATOM				register_class(HINSTANCE hInstance);
BOOL				init_instance(HINSTANCE, int);
LRESULT CALLBACK	main_event_handler(HWND, UINT, WPARAM, LPARAM);

// Global Variables:
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
	sprintf(JC_CONFIG_FILE, "%s\\.jc_config.txt", JC_USERS_HOME_DIRECTORY);

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
	if (jc_is_already_running()) return 0;

	MSG msg;
	HACCEL hAccelTable;

	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_JUMPCUT, szWindowClass, MAX_LOADSTRING);

	register_class(hInstance);

	if (!init_instance(hInstance, nCmdShow))
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

ATOM register_class(HINSTANCE hInstance)
{
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

int jc_get_modifier_code_from_string(string item)
{
	if (case_insensitive_match("control", item))
	{
		return MOD_CONTROL;
	}
	else if (case_insensitive_match("shift", item))
	{
		return MOD_SHIFT;
	}
	else if (case_insensitive_match("alt", item))
	{
		return MOD_ALT;
	}
	else if (case_insensitive_match("windows", item))
	{
		return MOD_WIN;
	}
	else jc_log(std::string("Couldnt recognize key code " + item).c_str());
	return 0;
}

int jc_get_key_code_from_string(string item)
{
	if (item == "0") return 0x30;
	else if (item == "1") return 0x31;
	else if (item == "2") return 0x32;
	else if (item == "3") return 0x33;
	else if (item == "4") return 0x34;
	else if (item == "5") return 0x35;
	else if (item == "6") return 0x36;
	else if (item == "7") return 0x37;
	else if (item == "8") return 0x38;
	else if (item == "9") return 0x39;
	else if (item == "A" || item == "a") return 0x41;
	else if (item == "B" || item == "b") return 0x42;
	else if (item == "C" || item == "c") return 0x43;
	else if (item == "D" || item == "d") return 0x44;
	else if (item == "E" || item == "e") return 0x45;
	else if (item == "F" || item == "f") return 0x46;
	else if (item == "G" || item == "g") return 0x47;
	else if (item == "H" || item == "h") return 0x48;
	else if (item == "I" || item == "i") return 0x49;
	else if (item == "J" || item == "j") return 0x4A;
	else if (item == "K" || item == "k") return 0x4B;
	else if (item == "L" || item == "l") return 0x4C;
	else if (item == "M" || item == "m") return 0x4D;
	else if (item == "N" || item == "n") return 0x4E;
	else if (item == "O" || item == "o") return 0x4F;
	else if (item == "P" || item == "p") return 0x50;
	else if (item == "Q" || item == "q") return 0x51;
	else if (item == "R" || item == "r") return 0x52;
	else if (item == "S" || item == "s") return 0x53;
	else if (item == "T" || item == "t") return 0x54;
	else if (item == "U" || item == "u") return 0x55;
	else if (item == "V" || item == "v") return 0x56;
	else if (item == "W" || item == "w") return 0x57;
	else if (item == "X" || item == "x") return 0x58;
	else if (item == "Y" || item == "y") return 0x59;
	else if (item == "Z" || item == "z") return 0x5A;

	else if (item == "F1" || item == "f1") return VK_F1;
	else if (item == "F2" || item == "f2") return VK_F2;
	else if (item == "F3" || item == "f3") return VK_F3;
	else if (item == "F4" || item == "f4") return VK_F4;
	else if (item == "F5" || item == "f5") return VK_F5;
	else if (item == "F6" || item == "f6") return VK_F6;
	else if (item == "F7" || item == "f7") return VK_F7;
	else if (item == "F8" || item == "f8") return VK_F8;
	else if (item == "F9" || item == "f9") return VK_F9;

	else if (item == "F10" || item == "f10") return VK_F10;
	else if (item == "F11" || item == "f11") return VK_F11;
	else if (item == "F12" || item == "f12") return VK_F12;
	else return 0;



}


void jc_load_hotkeys(char* config)
{

	int modifiers = 0;
	int key = 0;
	std::vector<string> lines;
	if (read_file_as_lines(config, lines))
	{
		auto results = split_string(lines[0], "+");
		for (auto item : results)
		{
			modifiers |= jc_get_modifier_code_from_string(trim(item));
			key |= jc_get_key_code_from_string(trim(item));
		}
	}
	else
	{
		jc_append_file(config, "control+alt+shift+r");
		modifiers = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
		key = jc_get_key_code_from_string("R");
	}

	if (!RegisterHotKey(
		globalHWND,
		JC_HOTKEY,
		modifiers,
		key)) jc_error_and_exit(_TEXT("Couldnt register hotkey bailing out.  Might be ~/.jc_config.txt , valid values are control,alt,shift,windows,A-za-z,F1-F12,0-9 strung togther with '+' and thats it."));

}
BOOL init_instance(HINSTANCE hInstance, int nCmdShow)
{
	HICON hMainIcon;

	globalHWND = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!globalHWND)
	{
		return FALSE;
	}
	jc_load_hotkeys(JC_CONFIG_FILE);

	AddClipboardFormatListener(globalHWND);
	jc_load_history_file();

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
	if (hwnd && IsWindowVisible(hwnd)/* && IsWindowEnabled(hwnd)*/)
	{
		//EnumChildWindows(hwnd, jc_try_and_paste_to_other_app, NULL);

		jc_log(hwnd_to_string(hwnd).c_str());
		PostMessage(hwnd, WM_PASTE, 0, 0);
		PostMessage(hwnd, WM_COMMAND, WM_PASTE, 0);

	}
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
				if (result.first) {
					move_item_to_tail(JC_CLIPBOARD_HISTORY, result.second);
				}
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
		// systray msg callback 
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

			int BASE = 2000;
			int idx = wmId - BASE;
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