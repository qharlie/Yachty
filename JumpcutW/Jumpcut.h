#pragma once
#include <windows.h>
#include <string>
#include <strsafe.h>
#include "resource.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <synchapi.h>
#include <WinUser.h>
#include <commctrl.h>

#define IDM_RSEARCH 201
#define JC_HOTKEY 9000
#define MAX_LOADSTRING 100
#define	WM_USER_SHELLICON WM_USER + 1

using namespace std;

// Globals and constants 
char JC_LOG_FILE[65535];
char JC_HISTORY_FILE[65535];
char JC_CONFIG_FILE[65535];

const int   JC_MAX_MENU_LABEL_LENGTH = 65;
const char* JC_USERS_HOME_DIRECTORY = getenv("USERPROFILE");
const char* JS_WHITESPACE = " \t\n\r\f\v";
const UINT  JC_MAX_RETRY_COUNT = 5;
const UINT  JC_MAX_HISTORY_SIZE = 40;
const int   JC_MENU_ID_BASE = 2000;
const char* JC_APPLICATION_NAME = "JumpcutW_v0.1";

std::string             JC_LAST_CLIPBOARD_ENTRY;
std::deque<std::string> JC_CLIPBOARD_HISTORY;

// Bail out with an error mesage 
void jc_error_and_exit(LPTSTR lpszFunction) {
	DWORD dw = GetLastError();

	// Retrieve the system error message for the last-error code
	if (lpszFunction) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;

		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			dw,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0, NULL);

		// Display the error message and exit the process

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
			(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
		StringCchPrintf((LPTSTR)lpDisplayBuf,
			LocalSize(lpDisplayBuf) / sizeof(TCHAR),
			TEXT("%s failed with error %d: %s"),
			lpszFunction, dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
	}
	ExitProcess(dw);
}

bool case_insensitive_match(string s1, string s2) {
	//convert s1 and s2 into lower case strings
	transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	transform(s2.begin(), s2.end(), s2.begin(), ::tolower);
	if (s1.compare(s2) == 0)
		return 1; //The strings are same
	return 0; //not matched
}

std::vector<std::string> split_string(const std::string& str,
	const std::string& delimiter) {
	std::vector<std::string> strings;

	std::string::size_type pos = 0;
	std::string::size_type prev = 0;
	while ((pos = str.find(delimiter, prev)) != std::string::npos)
	{
		strings.push_back(str.substr(prev, pos - prev));
		prev = pos + delimiter.size();
	}

	// To get the last substring (or only, if delimiter is not found)
	strings.push_back(str.substr(prev));

	return strings;
}
bool replace(std::string& str, const std::string& from, const std::string& to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}
void replaceAll(std::string& str, const std::string& from, const std::string& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
	}
}
// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = JS_WHITESPACE) {
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = JS_WHITESPACE) {
	s.erase(0, s.find_first_not_of(t));
	return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = JS_WHITESPACE) {
	return ltrim(rtrim(s, t), t);
}

// Convert to wide char 
wchar_t* jc_charToCWSTR(const char* charArray) {
	int len = strlen(charArray);
	wchar_t* wString = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, len + 1);
	return wString;
}
// Convert from wide char 
std::string jc_CWSTRToString(const wchar_t* charArray) {

	int len = wcslen(charArray);
	char* str = new char[len + 1];
	memset(str, 0, len);
	wcstombs(str, charArray, len + 1);
	std::string ret = std::string(str);
	delete str;

	return ret;
}

template <typename T>
void move_item_to_tail(std::deque<T>& v, size_t itemIndex) {
	auto it = v.begin() + itemIndex;
	std::rotate(it, it + 1, v.end());
}

template < typename T>
std::pair<bool, int > find_in_collection(const std::deque<T>& vecOfElements, const T& element) {
	std::pair<bool, int > result;
	auto it = std::find(vecOfElements.begin(), vecOfElements.end(), element);
	if (it != vecOfElements.end())
	{
		result.second = distance(vecOfElements.begin(), it);
		result.first = true;
	}
	else
	{
		result.first = false;
		result.second = -1;
	}
	return result;
}


BOOL jc_wait_on_clipboard(HWND hWnd, int maxRetryCount = JC_MAX_RETRY_COUNT)
{
	BOOL success = false;
	int counter = 0;
	success = OpenClipboard(hWnd);
	while (!success)
	{
		counter += 1;
		if (counter >= maxRetryCount) break;
		else Sleep(250);

		success = OpenClipboard(hWnd);
	}
	return success;
}


void jc_alert(std::string item) { MessageBox(NULL, jc_charToCWSTR(item.c_str()), _T("Alert"), MB_OK); }

void jc_set_clipboard(std::string item, HWND hWnd)
{
	BOOL success = jc_wait_on_clipboard(hWnd);
	if (success)
	{
		HGLOBAL clipbuffer;
		char* buffer;
		EmptyClipboard();
		clipbuffer = GlobalAlloc(GMEM_DDESHARE, item.length() + 1);
		buffer = (char*)GlobalLock(clipbuffer);
		strcpy(buffer, item.c_str());
		GlobalUnlock(clipbuffer);
		SetClipboardData(CF_TEXT, clipbuffer);
		CloseClipboard();
	}
}

void jc_append_file(const char* fileName, const char* message, bool shouldReplaceNewLines = false)
{
	string s = message;
	if (shouldReplaceNewLines)
	{
		replaceAll(s, "\r\n", "\\\\n");
	}
	ofstream myfile;
	myfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	myfile.open(fileName, std::ios_base::app);
	myfile << s << "\n";
	myfile.close();

}

void jc_log(const char* msg) { jc_append_file(JC_LOG_FILE, msg); }
void jc_history(const char* msg) { jc_append_file(JC_HISTORY_FILE, msg, true); }

bool read_file_as_lines(std::string fileName, std::vector<std::string>& vecOfStrs) {
	std::ifstream in(fileName.c_str());
	if (!in) return false;

	std::string str;
	while (std::getline(in, str))
	{
		if (str.size() > 0) vecOfStrs.push_back(str);
	}
	//Close The File
	in.close();
	return true;
}

void jc_load_history_file(std::string filePath) {
	std::vector<string> lines;
	JC_CLIPBOARD_HISTORY.clear();

	if (read_file_as_lines(filePath, lines))
	{
		int start_index = max(0, lines.size() - 50);
		for (int count = 0; count < 50; count++)
		{
			int idx = start_index + count;
			if (idx < lines.size())
			{
				string clip = lines[idx];
				replaceAll(clip, "\\\\n", "\r\n");
				JC_CLIPBOARD_HISTORY.push_back(clip);
			}
		}
	}
}


int jc_get_modifier_code_from_string(string item)
{
	if (case_insensitive_match("control", item)) return MOD_CONTROL;
	else if (case_insensitive_match("shift", item)) return MOD_SHIFT;
	else if (case_insensitive_match("alt", item)) return MOD_ALT;
	else if (case_insensitive_match("windows", item)) return MOD_WIN;
	else jc_log(std::string("Couldnt recognize key code " + item).c_str());
	return 0;

}

int jc_get_key_code_from_string(string item) {
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

void jc_load_hotkeys(char* config, HWND hwnd) {

	int modifiers = 0;
	int key = 0;
	std::vector<string> lines;
	if (read_file_as_lines(config, lines)) {
		auto results = split_string(lines[0], "+");
		for (auto item : results)
		{
			modifiers |= jc_get_modifier_code_from_string(trim(item));
			key |= jc_get_key_code_from_string(trim(item));
		}
	}
	else {
		jc_append_file(config, "control+alt+shift+r");
		modifiers = MOD_ALT | MOD_CONTROL | MOD_SHIFT;
		key = jc_get_key_code_from_string("R");
	}

	if (!RegisterHotKey(hwnd, JC_HOTKEY, modifiers, key)) jc_error_and_exit(_TEXT("Couldnt register hotkey bailing out.  Might be ~/.jc_config.txt , valid values are control,alt,shift,windows,A-za-z,F1-F12,0-9 strung togther with '+' and thats it."));

}

std::string jc_get_clipboard(HWND hWnd) {

	BOOL success = jc_wait_on_clipboard(hWnd);
	string ret = "";

	if (success) {

		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData) {
			char* pchData = (char*)GlobalLock(hClipboardData);
			CloseClipboard();
			GlobalUnlock(hClipboardData);
			if (pchData) ret = std::string(pchData);

		}
		else
		{
			CloseClipboard();
			try { GlobalUnlock(hClipboardData); }
			catch (exception e) {}

		}
	}
	else {
		jc_log(std::string("Couldnt handle to clipboard after " + std::to_string(JC_MAX_RETRY_COUNT) + " tries").c_str());
	}
	return ret;

}


VOID jc_start_external_application(LPCTSTR lpApplicationName)
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


HMENU jc_show_popup_menu(POINT& lpClickPoint, const HWND& hWnd, HINSTANCE inst, bool showControls) {
	UINT uFlag = MF_BYPOSITION | MF_STRING;
	HMENU hPopMenu = CreatePopupMenu();
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
			InsertMenu(hPopMenu, 0xFFFFFFFF, uFlag, i + JC_MENU_ID_BASE, jc_charToCWSTR(label.c_str()));
	}

	if (showControls) {
		if (JC_CLIPBOARD_HISTORY.size() > 0)
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));

		InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About JumpcutW"));
		//InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_RSEARCH, _T("Search"));

		InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit  JumpcutW"));

	}
	SetForegroundWindow(hWnd);

	HBITMAP hBitmap = (HBITMAP)LoadImage(inst,
		MAKEINTRESOURCE(IDB_PNG1),
		IMAGE_ICON,
		32,
		32,
		LR_DEFAULTCOLOR);
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof(mii);
	if (GetMenuItemInfo(hPopMenu, IDM_ABOUT, false, &mii)) {

		mii.fMask |= MIIM_BITMAP;
		mii.hbmpItem = hBitmap;
		mii.fType = MIIM_BITMAP;
		if (!SetMenuItemInfo(hPopMenu, IDM_ABOUT, false, &mii)) {}
	}
	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
	return hPopMenu;
}

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
void center_window(HWND hWnd)
{
	RECT rc;

	GetWindowRect(hWnd, &rc);

	int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
	int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

	SetWindowPos(hWnd, 0, xPos, yPos, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
INT_PTR CALLBACK jc_show_rsearch_dialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	HWND hSearchResults;
	int wmId = LOWORD(wParam);
	int wmEvent = HIWORD(wParam);

	switch (message)
	{
	case WM_INITDIALOG:
		center_window(hDlg);
		hSearchResults = GetDlgItem(hDlg, IDC_SEARCH_RESULTS);
		if (hSearchResults != NULL) {
			for (auto str : JC_CLIPBOARD_HISTORY)
			{
				SendMessage(hSearchResults, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(jc_charToCWSTR(str.c_str())));
			}
		}
		SendMessage(hSearchResults, CB_SETMINVISIBLE, (WPARAM)45, 0);
		return (INT_PTR)TRUE;

	case WM_COMMAND:


		if (wmEvent == CBN_SELCHANGE)
		{
			jc_alert("Selection Changed");
		}
		else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}

		break;
	}
	return (INT_PTR)FALSE;
}

string hwnd_to_string(HWND inputA)
{
	string s;
	int len = GetWindowTextLength(inputA);
	if (len > 0)
	{
		s.resize(len + 1);
		len = GetWindowTextA(inputA, &s[0], s.size());
		s.resize(len);
	}
	return s;
}
BOOLEAN jc_is_already_running() {
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, jc_charToCWSTR(JC_APPLICATION_NAME));

	if (!hMutex) {
		hMutex =
			CreateMutex(0, 0, jc_charToCWSTR(JC_APPLICATION_NAME));
		return 0;
	}
	else {
		return 1;
	}
}