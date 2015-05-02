#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include <string.h>

using namespace std;

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) ;

int main1()
{
	
	HCURSOR hCurs1;
	POINT pt;
	HWND hWnd;
	
	WNDCLASS WC;
	WC.lpfnWndProc = WndProc;
	
	char c = '1';

	WCHAR str[50];
	//hWnd = GetDesktopWindow();
	hWnd = GetForegroundWindow();

	
	do
	{
		//while (!_kbhit())
		//	_cputs( "Hit me!! " );
		c = _getch();

		GetCursorPos(&pt);
		if(c == '0')
			break;
		cout << pt.x << " " << pt.y << endl;
		//SetCursorPos(10, 10);
		
		//hWnd = GetDesktopWindow();
		//hWnd = GetWindow(hWnd, GW_HWNDFIRST);
		GetWindowText(hWnd, str, 100);
		wprintf(L"%s", str);
	}
	while(1);

	return 0;
	
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) 
{
	POINT pt;
	switch (message)                  
	{
		case WM_MOUSEMOVE:
			GetCursorPos(&pt);
			SetCursorPos(10, 10);
			return 0;
		case WM_LBUTTONDOWN:
			GetCursorPos(&pt);
			SetCursorPos(pt.x - 10, pt.y - 10);
			return 0;
		case WM_RBUTTONDOWN:
			GetCursorPos(&pt);
			SetCursorPos(pt.x + 10, pt.y + 10);
			return 0;
	}
	return 0;
}