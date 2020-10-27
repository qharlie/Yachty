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
const UINT JC_MAX_CLIPBOARD_BUFFER_SIZE = 524288;
char JC_LOG_FILE[65535];
const char* JS_WHITESPACE = " \t\n\r\f\v";
const UINT JC_MAX_RETRY_COUNT = 5;
std::string JC_LAST_CLIPBOARD_ENTRY;
std::vector<std::string> JC_CLIPBOARD_HISTORY(50);

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
inline std::string& rtrim(std::string& s, const char* t = JS_WHITESPACE)
{
	s.erase(s.find_last_not_of(t) + 1);
	return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = JS_WHITESPACE)
{
	s.erase(0, s.find_first_not_of(t));
	return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = JS_WHITESPACE)
{
	return ltrim(rtrim(s, t), t);
}

wchar_t* jc_charToCWSTR(const char* charArray)
{
	wchar_t* wString = new wchar_t[4096];
	MultiByteToWideChar(CP_ACP, 0, charArray, -1, wString, 4096);
	return wString;
}

template <typename T>
void moveItemToBack(std::vector<T>& v, size_t itemIndex)
{
	auto it = v.begin() + itemIndex;
	std::rotate(it, it + 1, v.end());
}

template < typename T>
std::pair<bool, int > findInVector(const std::vector<T>& vecOfElements, const T& element)
{
	std::pair<bool, int > result;
	// Find given element in vector
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
class FixedQueue : public std::queue<T, Container> {
public:
	void push(const T& value) {
		if (this->size() == MaxLen) {
			this->c.pop_front();
		}
		std::queue<T, Container>::push(value);
	}
};

BOOL jc_wait_on_clipboard(HWND hWnd)
{
	BOOL success = false;
	int i = 0;
	success = OpenClipboard(hWnd);
	while (!success)
	{
		i += 1;
		if (i >= JC_MAX_RETRY_COUNT)
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
void jc_appendToFile(char* fileName, char* message)
{
	ofstream myfile;
	myfile.open(fileName, std::ios_base::app);
	myfile << message << "\n";
	myfile.close();

}

void jc_appendToLog(char* msg)
{
	jc_appendToFile(JC_LOG_FILE, msg);
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