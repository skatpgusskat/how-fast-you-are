#define ASIO_STANDALONE
#include "asio.hpp"

#define _WIN32_WINNT 0x0400
#pragma comment( lib, "user32.lib" )
#include <iostream>
#include <windows.h>
#include <stdio.h>

using asio::ip::udp;

HHOOK hKeyboardHook;

asio::io_context io_context;
udp::socket udpSocket = udp::socket(io_context, udp::endpoint(udp::v4(), 0));
udp::resolver resolver(io_context);
udp::resolver::results_type endpoints = resolver.resolve(udp::v4(), "localhost", "58825");

int previousKey = 0;

// code from https://stackoverflow.com/questions/29734263/creating-global-keyboard-hook
__declspec(dllexport) LRESULT CALLBACK KeyboardEvent(int nCode, WPARAM wParam, LPARAM lParam)
{
	DWORD SHIFT_key = 0;
	DWORD CTRL_key = 0;
	DWORD ALT_key = 0;


	if ((nCode == HC_ACTION) && ((wParam == WM_SYSKEYDOWN) || (wParam == WM_KEYDOWN)))
	{
		KBDLLHOOKSTRUCT hooked_key = *((KBDLLHOOKSTRUCT*)lParam);
		DWORD dwMsg = 1;
		dwMsg += hooked_key.scanCode << 16;
		dwMsg += hooked_key.flags << 24;
		wchar_t lpszKeyName[1024] = { 0 };

		int i = GetKeyNameText(dwMsg, (lpszKeyName + 1), 0xFF) + 1;

		int key = hooked_key.vkCode;

		if (key != previousKey) {
			SHIFT_key = GetAsyncKeyState(VK_SHIFT);
			CTRL_key = GetAsyncKeyState(VK_CONTROL);
			ALT_key = GetAsyncKeyState(VK_MENU);

			printf("Keycode = %c\n", key);

			udpSocket.send_to(asio::buffer("key", 3), *endpoints.begin());
			previousKey = key;
		}

	}
	return CallNextHookEx(hKeyboardHook, nCode, wParam, lParam);
}

void MessageLoop()
{
	MSG message;
	while (GetMessage(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}

DWORD WINAPI my_HotKey(LPVOID lpParm)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if (!hInstance) hInstance = LoadLibrary((LPCWSTR)lpParm);
	if (!hInstance) return 1;

	hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)KeyboardEvent, hInstance, NULL);
	MessageLoop();
	UnhookWindowsHookEx(hKeyboardHook);
	return 0;
}

int main(int argc, char** argv)
{
	HANDLE hThread;
	DWORD dwThread;

	printf("CTRL-y  for  H O T K E Y  \n");
	printf("CTRL-q  to quit  \n");

	hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)my_HotKey, (LPVOID)argv[0], NULL, &dwThread);

	/* uncomment to hide console window */
	//ShowWindow(FindWindowA("ConsoleWindowClass", NULL), false);

	if (hThread) return WaitForSingleObject(hThread, INFINITE);
	else return 1;

}