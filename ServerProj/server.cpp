#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>
#include <fstream>
#include <olectl.h>

#include "ConvertJpegToBmp.h"
#include "window.h"

using namespace std;

HHOOK hHookMouse;
HHOOK hHookKeyboard;
SOCKET ClientSocket;		//переделать на локальное объявление

double kCursorX = 1;
double kCursorY = 1;


#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"

HANDLE hNamedPipe;

void CursorPosToString(char* sendbuf, int mouseButton);
void SendToClient(int mouseButton);

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

DWORD WINAPI ThreadKeyboardFunction (LPVOID lpParameters);
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

	cout << "Waiting for client connection..." << endl;

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup() failed. Error: "<< iResult << endl;
		_getch();
		return 0;
	}

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);	// создаем структуру адреса сокета
	if (iResult != 0) 
	{
		cout <<"Getaddrinfo() failed. Error: " << iResult << endl;
		WSACleanup();
		_getch();
		return 0;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);	// создаем конечную точку соединения
	
	if (ListenSocket == INVALID_SOCKET) 
	{
		cout << "Socket() failed. Error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();			// для закрытия WS2_32 DLL
		_getch();
		return 0;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen); // привязывает имя к сокету
    if (iResult == SOCKET_ERROR) 
	{
        cout << "Bind() failed. Error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
		_getch();
        return 0;
    }

		// устанавливаем готовность принимать входящие соединения и задаем размер очереди
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) 
	{
		cout << "Listen() failed. Error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		_getch();
		return 0;
	}

	ClientSocket = accept(ListenSocket, NULL, NULL);	// ждем подключение к серверу
	if (ClientSocket == INVALID_SOCKET) 
	{
		cout << "Accept() failed. Error: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		_getch();
		return 0;
	}

	char ServerName[20];
	DWORD sizeOfServerName;
	GetComputerNameA(ServerName, &sizeOfServerName);
	ServerName[sizeOfServerName] = '\0';

	cout << "Client has connected." << endl;
	Sleep(200);
	int iSendResult = send(ClientSocket, ServerName, 7, 0);

	if (iSendResult == SOCKET_ERROR)
	{
		cout << "Server name sending failed. Error: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		_getch();
		return 0;
	}

	SECURITY_ATTRIBUTES sa;
	SECURITY_DESCRIPTOR sd;
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE; // дескриптор канала ненаследуемый
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION); 
	SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
	sa.lpSecurityDescriptor = &sd; 


	//передать имя компьютера
	
	hNamedPipe = CreateNamedPipe(L"\\\\.\\pipe\\synchroPipe", PIPE_ACCESS_DUPLEX, NULL, 1, 100, 100, NULL, &sa);
	if(hNamedPipe == INVALID_HANDLE_VALUE)
	{	
		cout << "Pipe creating failed. Error: " << GetLastError() << endl;
		_getch();
		return 0;
	}

	HANDLE hThreadImage;
	HANDLE hThreadKeyboard;
	DWORD IDThreadImage;
	DWORD IDThreadKeyboard;
	hThreadImage = CreateThread(NULL, NULL, ThreadRecvImageFunction, (void*)ClientSocket, 0, &IDThreadImage);
	hThreadImage = CreateThread(NULL, NULL, ThreadKeyboardFunction, (void*)ClientSocket, 0, &IDThreadKeyboard);
	
	POINT pt;
	
	hHookMouse = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
	hHookKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0); // IDThreadKeyboard
	MSG msg = { 0 };

	while(GetMessage(&msg, NULL, 0, 0));

	freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
	CloseHandle(hNamedPipe);
	TerminateThread(hThreadImage, NO_ERROR);
	TerminateThread(hThreadKeyboard, NO_ERROR);

	UnhookWindowsHookEx(hHookMouse);
	UnhookWindowsHookEx(hHookKeyboard);

	remove("TempImageServer.bmp");
	remove("TempImageServer.jpeg");
	cout << "end of all";
	_getch();
	
	return 0;
}

void CursorPosToString(char* sendbuf, int mouseButton)
{
	static POINT pt;
	GetCursorPos(&pt);
	sendbuf[0] = '\0';
	//if(kCursorX != 1 || kCursorX != 1) 
	sprintf(sendbuf, "%d_%d_%d", (int)(pt.x * kCursorX), (int)(pt.y * kCursorY), mouseButton);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static DWORD dwKCode = ((KBDLLHOOKSTRUCT*)lParam)->vkCode;
	MOUSEHOOKSTRUCT *mh = (MOUSEHOOKSTRUCT*)lParam;
	
	switch(wParam)
	{
	case WM_MOUSEMOVE:	
		SendToClient(WM_MOUSEMOVE);
		Sleep(30);
		break;
	case WM_LBUTTONDOWN:
		cout << "Left button down!" << &wParam << endl;
		SendToClient(WM_LBUTTONDOWN);
		Sleep(20);
		break;
	case WM_LBUTTONUP:
		cout << "Left button up!" << endl;
		SendToClient(WM_LBUTTONUP);
		Sleep(50);
		break;
	case WM_RBUTTONDOWN:
		cout << "Right button down!" << endl;
		SendToClient(WM_RBUTTONDOWN);
		break;
	case WM_RBUTTONUP:
		cout << "Right button up!" << endl;
		SendToClient(WM_RBUTTONUP);
		break;
	case WM_MBUTTONDOWN:
		cout << "Middle button down!" << endl;
		SendToClient(WM_MBUTTONDOWN);
		break;
	case WM_MBUTTONUP:
		cout << "Middle button up!" << endl;
		SendToClient(WM_MBUTTONUP);
		break;
	default:
		break;
	}
 
    return CallNextHookEx(hHookMouse, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*) lParam;
	switch(wParam)
	{
	case WM_KEYDOWN:
		switch(ks->vkCode)
		{
			case VK_LEFT:
				cout << "left_arrow_down" << endl;
                break; 
            case VK_RIGHT: 
                break; 
            case VK_UP: 
                break; 
            case VK_DOWN: 
				break; 
		}
		
	default:
		break;
	}
 
    return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);
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
		cout << "Send of cursor failed. Error: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		_getch();
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
	DWORD numberOfWrittenBytes;
	static wchar_t szClassName[ ] = L"WindowClass";

	//принимаем разрешение экрана
	int iResult = 0;
	static char bufferForScr[9] = {'\0'};
	iResult = recv(ClientSocket, bufferForScr, 9, 0);
	if (iResult == 0)
	{
		cout << "Connection closed. Unable to get resolution of screen." << endl;
		_getch();
		return 0;
	}
	if (iResult < 0)
	{
		cout << "Recv failed. Error: " << WSAGetLastError() << endl;
		_getch();
		return 0;
	}

	//переводим строку в числа
	int ResolutionScreenClientX = 0;
	int ResolutionScreenClientY = 0;
	int i = 0;

	while(bufferForScr[i] != '_')
	{
		ResolutionScreenClientX *= 10;
		ResolutionScreenClientX += bufferForScr[i++] - '0';
	}
	i++;
	while(bufferForScr[i] != '\0')
	{
		ResolutionScreenClientY *= 10;
		ResolutionScreenClientY += bufferForScr[i++] - '0';
	}

	CheckWidthScreen(&kCursorX, &ResolutionScreenClientX);
	ChechHeigthScreen(&kCursorY, &ResolutionScreenClientY);

	HWND hwnd = CreateNewWindow(ResolutionScreenClientX, ResolutionScreenClientY);

	// получаем изображение
	do 
	{
		fflush(stdin);
		fflush(stdout);
		char* bufferForImage = NULL;
		char* bufferForSize = NULL;
		unsigned int sizeOfFile = 0;
		//lpParameters contain  ClientSocket
		//сначала считываем размер, потом сам файл
		bufferForSize = new char[10];

		WriteFile(hNamedPipe, "*", 1, &numberOfWrittenBytes, NULL);
		iResult = recv(ClientSocket, bufferForSize, 10, 0);

		if (iResult > 0)
		{
			sizeOfFile = atoi(bufferForSize);
		}
		else if (iResult == 0)
		{
			cout << "Connection closed. Size of image not recieved." << endl;
			_getch();
			return 0;
		}
		else
		{
			cout << "Recv failed. Error: " << WSAGetLastError() << endl;
			_getch();
			return 0;
		}
		free(bufferForSize);
		

		bufferForImage = new char[sizeOfFile];

		WriteFile(hNamedPipe, "*", 1, &numberOfWrittenBytes, NULL);
		iResult = recv(ClientSocket, bufferForImage, sizeOfFile, 0);

		if (iResult == 0)
		{
			cout << "Connection closed. Image not received." << endl;
			_getch();
			return 0;
		}
		if (iResult < 0)
		{
			cout << "Recv failed. Error: " << WSAGetLastError() << endl;
			_getch();
			return 0;
		}

		HANDLE hFileImage = CreateFile(L"TempImageServer.jpeg", GENERIC_READ | GENERIC_WRITE, 0,NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);

		WriteFile(hFileImage, bufferForImage, sizeOfFile, &numberOfWrittenBytes, NULL);	

		ConvertToBMP(hFileImage);

		CloseHandle(hFileImage);

		free(bufferForImage);

		InvalidateRect(hwnd, NULL, NULL);
		UpdateWindow(hwnd);	

		//remove("TempImageServer.bmp");
		//remove("TempImageServer.jpeg");
	} 
	while (1);
}

DWORD WINAPI ThreadKeyboardFunction (LPVOID lpParameters)
{
	return 0;
}