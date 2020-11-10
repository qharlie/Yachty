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


using namespace std;

// Globals and constants 
char JC_LOG_FILE[65535];
char JC_HISTORY_FILE[65535];
char JC_CONFIG_FILE[65535];

const int JC_MAX_MENU_LABEL_LENGTH = 65;
const char* JC_USERS_HOME_DIRECTORY = getenv("USERPROFILE");
const char* JS_WHITESPACE = " \t\n\r\f\v";
const UINT JC_MAX_RETRY_COUNT = 5;
const UINT JC_MAX_HISTORY_SIZE = 40;
const char* JC_APPLICATION_NAME = "JumpcutW_v0.1";
std::string JC_LAST_CLIPBOARD_ENTRY;
std::deque<std::string> JC_CLIPBOARD_HISTORY;

// Bail out with an error mesage 
void jc_error_and_exit(LPTSTR lpszFunction)
{
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
	const std::string& delimiter)
{
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
	int i = 0;
	success = OpenClipboard(hWnd);
	while (!success)
	{
		i += 1;
		if (i >= maxRetryCount)
		{
			break;
		}
		else {
			Sleep(250);
		}
		success = OpenClipboard(hWnd);
	}
	return success;
}


void jc_alert(std::string item)
{
	MessageBox(NULL, jc_charToCWSTR(item.c_str()), _T("Alert"), MB_OK);
}
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

void jc_log(const char* msg)
{
	jc_append_file(JC_LOG_FILE, msg);
}


void jc_history(const char* msg)
{
	jc_append_file(JC_HISTORY_FILE, msg, true);
}

bool read_file_as_lines(std::string fileName, std::vector<std::string>& vecOfStrs)
{
	// Open the File
	std::ifstream in(fileName.c_str());
	// Check if object is valid
	if (!in)
	{
		std::cerr << "Cannot open the File : " << fileName << std::endl;
		return false;
	}
	std::string str;
	// Read the next line from File untill it reaches the end.
	while (std::getline(in, str))
	{
		// Line contains string of length > 0 then save it in vector
		if (str.size() > 0)
			vecOfStrs.push_back(str);
	}
	//Close The File
	in.close();
	return true;
}
void jc_load_history_file() {
	std::vector<string> lines;
	JC_CLIPBOARD_HISTORY.clear();
	//JC_CLIPBOARD_HISTORY.
	if (read_file_as_lines(JC_HISTORY_FILE, lines))
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

std::string jc_get_clipboard(HWND hWnd)
{
	BOOL success = jc_wait_on_clipboard(hWnd);

	if (success) {

		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		if (hClipboardData) {
			char* pchData = (char*)GlobalLock(hClipboardData);
			CloseClipboard();
			GlobalUnlock(hClipboardData);
			if (pchData)
				return std::string(pchData);
			else return "";
		}
		else return "";
	}
	else {
		return std::string("");
	}

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


HMENU jc_show_popup_menu(POINT& lpClickPoint, const HWND& hWnd, HINSTANCE inst, bool showControls)
{
	UINT uFlag = MF_BYPOSITION | MF_STRING;
	//GetCursorPos(&lpClickPoint);
	HMENU hPopMenu = CreatePopupMenu();
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

	if (showControls) {
		if (JC_CLIPBOARD_HISTORY.size() > 0)
			InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));

		InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About JumpcutW"));
		InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_RSEARCH, _T("Search"));

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
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
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