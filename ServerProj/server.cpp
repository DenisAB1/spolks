#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>
#include <fstream>
#include <olectl.h>

#include "ConvertJpegToBmp.h"

using namespace std;

HHOOK hHook;
SOCKET ClientSocket;		//переделать на локальное объявление
int ResolutionScreenX = 0;
int ResolutionScreenY = 0;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"

void CursorPosToString(char* sendbuf, int mouseButton);
void SendToClient(int mouseButton);

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

DWORD WINAPI ThreadRecvImageFunction (LPVOID lpParameters);

int main() 
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;
	SOCKET ListenSocket;
	

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;			// будет использоваться IPv4
	hints.ai_socktype = SOCK_STREAM;	// последовательное двусторонний поток байтов
	hints.ai_protocol = IPPROTO_TCP;	// протокол - Transmission Control Protocol
	hints.ai_flags = AI_PASSIVE;		// клиент сможет подключиться с любым адресом

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed: "<< iResult << endl;
		return 0;
	}

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);	// создаем структуру адреса сокета
	if (iResult != 0) 
	{
		cout <<"getaddrinfo failed: " << iResult << endl;
		WSACleanup();
		return 0;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	// создаем конечную точку соединения
	
	if (ListenSocket == INVALID_SOCKET) 
	{
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();			// для закрытия WS2_32 DLL
		return 0;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // привязывает имя к сокету
    if (iResult == SOCKET_ERROR) 
	{
        cout << "Bind() failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 0;
    }

		// устанавливаем готовность принимать входящие соединения и задаем размер очереди
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) 
	{
		cout << "Listen failed with error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}

	ClientSocket = accept(ListenSocket, NULL, NULL);	// ждем подключение к серверу
	if (ClientSocket == INVALID_SOCKET) 
	{
		cout << "accept failed: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 0;
	}

	cout << "Client has connected." << endl;

	HANDLE hThread;
	DWORD IDThread;
	hThread = CreateThread(NULL, NULL, ThreadRecvImageFunction, (void*)ClientSocket, 0, &IDThread);
	
	POINT pt;
	
	hHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
	MSG msg = { 0 };

	while(GetMessage(&msg, NULL, 0, 0));

	freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
	TerminateThread(hThread, NO_ERROR);
	cout << "end of all";
	_getch();
	return 0;
}

void CursorPosToString(char* sendbuf, int mouseButton)
{
	POINT pt;
	char xPos[5];
	char yPos[5];
	char mouse[5];
	GetCursorPos(&pt);
	sendbuf[0] = '\0';
	itoa(pt.x, xPos, 10);
	itoa(pt.y, yPos, 10);
	strcat(sendbuf, xPos);
	strcat(sendbuf, "_");
	strcat(sendbuf, yPos);
	strcat(sendbuf, "_");
	strcat(sendbuf, itoa(mouseButton, mouse, 10));
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    DWORD dwKCode = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	switch(wParam)
	{
	case WM_MOUSEMOVE:
		//cout << "Mouse is moving!" << endl;
		SendToClient(WM_MOUSEMOVE);
		Sleep(5);
		break;
	case WM_LBUTTONDOWN:
		//cout << "Left button clicked!" << endl;
		SendToClient(WM_LBUTTONDOWN);
		break;
	case WM_RBUTTONDOWN:
		//cout << "Right button clicked!" << endl;
		SendToClient(WM_RBUTTONDOWN);
		break;
	}
 
    return CallNextHookEx(hHook, nCode, wParam, lParam);
}

void SendToClient(int mouseButton)
{
	int iSendResult;
	char sendbuf[128];
	int sendbuflen = 128;	
	CursorPosToString(sendbuf, mouseButton);
	sendbuflen = strlen(sendbuf);
	sendbuf[sendbuflen] = '\0';
	iSendResult = send(ClientSocket, sendbuf, sendbuflen+1, 0);
	if (iSendResult == SOCKET_ERROR)
	{
		cout << "Send failed: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}
	Sleep(15);
}

/*WM_MOUSEMOVE = 0x200,
WM_LBUTTONDOWN = 0x201,
WM_LBUTTONUP = 0x202,
WM_LBUTTONDBLCLK = 0x203,
WM_RBUTTONDOWN = 0x204,
WM_RBUTTONUP = 0x205,
WM_RBUTTONDBLCLK = 0x206,
WM_MBUTTONDOWN = 0x207,
WM_MBUTTONUP = 0x208,
WM_MBUTTONDBLCLK = 0x209,
WM_MOUSEWHEEL = 0x20A,
WM_XBUTTONDOWN = 0x20B,
WM_XBUTTONUP = 0x20C,
WM_XBUTTONDBLCLK = 0x20D,
WM_MOUSEHWHEEL = 0x20E*/


DWORD WINAPI ThreadRecvImageFunction (LPVOID lpParameters)
{


	wchar_t szClassName[ ] = L"WindowClass";

	//принимаем разрешение экрана
	int iResult = 0;
	char bufferForScr[9] = {'\0'};
	iResult = recv(ClientSocket, bufferForScr, 9, 0);
	if (iResult == 0)
		cout << "Connection closed." << endl;
	if (iResult < 0)
		cout << "Recv failed: " << WSAGetLastError() << endl;

	//переводим строку в числа
	int i = 0;
	while(bufferForScr[i] != '_')
	{
		ResolutionScreenX *= 10;
		ResolutionScreenX += bufferForScr[i++] - '0';
	}
	i++;
	while(bufferForScr[i] != '\0')
	{
		ResolutionScreenY *= 10;
		ResolutionScreenY += bufferForScr[i++] - '0';
	}

	//открываем новое окно нужного размера
	HINSTANCE hInstance = GetModuleHandle(NULL); // получение HINSTANCE приложения
    HWND hwnd;
    MSG msg;
    WNDCLASSEX wincl;
 
    wincl.hInstance = hInstance;							/*				*/
    wincl.lpszClassName = szClassName;						//
    wincl.lpfnWndProc = WndProc;							//
    wincl.style = CS_DBLCLKS | CS_VREDRAW | CS_HREDRAW;		//
    wincl.cbSize = sizeof (WNDCLASSEX);
 
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);					// для регистрации класса окна
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;
    wincl.cbClsExtra = 0;									//
    wincl.cbWndExtra = 0;									/*				*/
 
    wincl.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
 
    if (!RegisterClassEx (&wincl))							// регистрация класса для создания окна
        return 0;
    
    //hInst = hInstance;

 
    hwnd = CreateWindowEx (								// создаем окно, но не показываем
           NULL,
           szClassName,
           L"",
           WS_POPUP,
           0,
           0,
           ResolutionScreenX,
           ResolutionScreenY,
           HWND_DESKTOP,
           NULL,
           hInstance,
           NULL
           );

	ShowWindow (hwnd, 1);	// вместо 1 параметр nCmdShow

	/*while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	*/
	// получаем изображение
	do 
	{
		
		char* bufferForImage = NULL;
		char* bufferForSize = NULL;
		unsigned int sizeOfFile = 0;
		//lpParameters contain  ClientSocket
		//сначала считываем размер, потом сам файл
		bufferForSize = new char[10];

		iResult = recv(ClientSocket, bufferForSize, 10, 0);
		if (iResult > 0)
		{
			sizeOfFile = atoi(bufferForSize);
		}
		else if (iResult == 0)
			cout << "Connection closed." << endl;
		else
			cout << "Recv failed: " << WSAGetLastError() << endl;


		bufferForImage = new char[sizeOfFile];

		iResult = recv(ClientSocket, bufferForImage, sizeOfFile, 0);

		if (iResult == 0)
			cout << "Connection closed." << endl;
		if (iResult < 0)
			cout << "Recv failed: " << WSAGetLastError() << endl;

		FILE *fImage = fopen("TempImage2.jpeg", "wb");

		iResult = fwrite(bufferForImage, 1, sizeOfFile, fImage);

		free(bufferForSize);
		free(bufferForImage);

		fclose(fImage);

		Sleep(10);

		ConvertToBMP();

		InvalidateRect(hwnd, NULL, NULL);
		UpdateWindow(hwnd);
///////////////////////////////////////////////////////////////////////////////////////
		
	} 
	while (1);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
        static HBITMAP hImage;
		static BITMAP bm;
        
        HDC hPaintDC, hMemDC;
         PAINTSTRUCT ps;
        HGDIOBJ hOld;		// дескриптор графического объекта
        
    switch (message)
    {
        case WM_PAINT:
			 
			
            hImage = (HBITMAP)LoadImage(NULL,L"TempImage2.bmp", IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
			GetObject(hImage,sizeof(BITMAP),&bm);
            hPaintDC = BeginPaint (hwnd, &ps);
            hMemDC = CreateCompatibleDC (hPaintDC);
            hOld = SelectObject (hMemDC, hImage);
             
            BitBlt (hPaintDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);// после этого появляется изображение
             
            SelectObject (hMemDC, hOld);
            DeleteDC (hMemDC);
			//DeleteDC (hPaintDC);
			//DeleteObject(hImage);
			
            EndPaint (hwnd, &ps);
			
            break;
        case WM_DESTROY:
            DeleteObject (hImage);
            PostQuitMessage (0);
            break;
        default:
            return DefWindowProc (hwnd, message, wParam, lParam);
    }
 
    return 0;
}