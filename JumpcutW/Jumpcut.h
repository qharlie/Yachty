#pragma once
#include <windows.h>
#include <strsafe.h>
#include "resource.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <deque>
#include <synchapi.h>
#include <WinUser.h>
using namespace std;

// Globals and constants 
char JC_LOG_FILE[65535];
char JC_HISTORY_FILE[65535];
const int JC_MAX_MENU_LABEL_LENGTH = 65;
char* JC_USERS_HOME_DIRECTORY = getenv("USERPROFILE");
const char* JS_WHITESPACE = " \t\n\r\f\v";
const UINT JC_MAX_RETRY_COUNT = 5;
const UINT JC_MAX_HISTORY_SIZE = 50;
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

	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}
// Convert from wide char 
std::string jc_CWSTRToString(const wchar_t* charArray) {

	int len = wcslen(charArray);
	char* str = new char[len];
	memset(str, 0, len);
	wcstombs(str, charArray, 12);
	std::string ret = std::string(str);
	delete str;

	return ret;
}
template <typename T>
void moveItemToBack(std::vector<T>& v, size_t itemIndex) {
	auto it = v.begin() + itemIndex;
	std::rotate(it, it + 1, v.end());
}

template <typename T>
void moveItemToBack(std::deque<T>& v, size_t itemIndex) {
	auto it = v.begin() + itemIndex;
	std::rotate(it, it + 1, v.end());
}

template < typename T>
std::pair<bool, int > findInCollection(const std::vector<T>& vecOfElements, const T& element) {
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

template < typename T>
std::pair<bool, int > findInCollection(const std::deque<T>& vecOfElements, const T& element) {
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
template <typename T, int MaxLen, typename Container = std::deque<T>>
class FixedQueue : public std::deque<T, Container> {
public:
	void push(const T& value) {
		if (this->size() == MaxLen) {
			this->c.pop_front();
		}
		this->push_back(value);
	}
};


template <typename T, int MaxLen, typename Container = std::vector<T>>
class FixedVector : public std::vector<T, Container> {
public:
	void push(const T& value) {
		if (this->size() == MaxLen) {
			this->c.pop_front();
		}
		std::vector<T, Container>::push(value);
	}
};

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
			jc_error_and_exit(jc_charToCWSTR("CLIPBOARD_READ_FAILED, now bailing out."));
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
	else jc_error_and_exit(TEXT("SET_CLIPBOARD"));
}
void jc_appendToFile(const char* fileName, const char* message)
{
	ofstream myfile;
	myfile.open(fileName, std::ios_base::app);
	myfile << message << "\n";
	myfile.close();

}

void jc_log(const char* msg)
{
	jc_appendToFile(JC_LOG_FILE, msg);
}


void jc_save_history(const char* msg)
{
	jc_appendToFile(JC_HISTORY_FILE, msg);
}

std::string jc_get_clipboard(HWND hWnd)
{
	BOOL success = jc_wait_on_clipboard(hWnd);

	if (success) {
		//{
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		char* pchData = (char*)GlobalLock(hClipboardData);
		CloseClipboard();
		GlobalUnlock(hClipboardData);
		return std::string(pchData);
	}
	else {
		jc_error_and_exit(TEXT("ERROR ACCESS DENIED TO OPENCLIPBOARD"));
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


HMENU jc_show_popup_menu(POINT& lpClickPoint, const HWND& hWnd, HINSTANCE inst)
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
	if (JC_CLIPBOARD_HISTORY.size() > 0)
		InsertMenu(hPopMenu, 0xFFFFFFFF, MF_SEPARATOR, IDM_SEP, _T("SEP"));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_ABOUT, _T("About JumpcutW"));
	InsertMenu(hPopMenu, 0xFFFFFFFF, MF_BYPOSITION | MF_STRING, IDM_EXIT, _T("Exit  JumpcutW"));

	SetForegroundWindow(hWnd);

	HBITMAP hBitmap = (HBITMAP)LoadImage(inst,
		MAKEINTRESOURCE(IDB_PNG1),
		IMAGE_ICON,
		32,
		32,
		LR_DEFAULTCOLOR);
	MENUITEMINFO mii = { 0 };
	mii.cbSize = sizeof(mii);
	if (!GetMenuItemInfo(hPopMenu, IDM_ABOUT, false, &mii)) {
		jc_error_and_exit(TEXT("getMenuItemInfo"));
	}
	mii.fMask |= MIIM_BITMAP;
	mii.hbmpItem = hBitmap;
	mii.fType = MIIM_BITMAP;
	if (!SetMenuItemInfo(hPopMenu, IDM_ABOUT, false, &mii)) { jc_alert("FAILED"); }

	TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, lpClickPoint.x, lpClickPoint.y, 0, hWnd, NULL);
	return hPopMenu;
}

//VOID CALLBACK jc_show_menu_at_current_point(HWND hwnd, UINT message, UINT idTimer, DWORD dwTime) {
//
//	RECT rc;
//	POINT pt;
//
//	if (GetCursorPos(&pt))
//	{
//		jc_show_popup_menu(pt, hwnd, h;);
//	}
//	else jc_log("Failed to GetCursorPos()");
//}

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
BOOLEAN jc_is_only_instance() {
	HANDLE hMutex = OpenMutex(MUTEX_ALL_ACCESS, 0, jc_charToCWSTR(JC_APPLICATION_NAME));

	if (!hMutex)
		hMutex =
		CreateMutex(0, 0, jc_charToCWSTR(JC_APPLICATION_NAME));
	else
		return 0;
}