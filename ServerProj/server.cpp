#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>
#include <fstream>
#include <olectl.h>

#include "ConvertJpegToBmp.h"
#include "window.h"

// щя как закомичу

using namespace std;

#define SIZE_OF_NAME_SERVER 20

HHOOK hHookMouse;
HHOOK hHookKeyboard;
SOCKET ClientSocket;		
HWND hWndImage;

int ResolutionScreenClientX = 0;
int ResolutionScreenClientY = 0;

double kCursorX = 1;
double kCursorY = 1;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"

HANDLE hNamedPipe;
HANDLE hKeybordPipe;
HANDLE hEventMouse = CreateEvent(NULL, FALSE, TRUE, NULL);
HANDLE hEventNextUpdate = CreateEvent(NULL, FALSE, TRUE, L"EventNextUpdate");

void SendToClient(int x, int y, int mouseButton, char c, int delta);

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

DWORD WINAPI ThreadRecvImageFunction (LPVOID lpParameters);

int main() 
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;
	SOCKET ListenSocket;

	char ServerName[SIZE_OF_NAME_SERVER];
	DWORD sizeOfServerName;
	GetComputerNameA(ServerName, &sizeOfServerName);
	ServerName[sizeOfServerName] = '\0';

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;			// будет использоваться IPv4
	hints.ai_socktype = SOCK_STREAM;	// последовательное двусторонний поток байтов
	hints.ai_protocol = IPPROTO_TCP;	// протокол - Transmission Control Protocol
	hints.ai_flags = AI_PASSIVE;		// клиент сможет подключиться с любым адресом


	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup() failed. Error: "<< iResult << endl;
		_getch();
		return 0;
	}

	hostent *sh;
	sh = gethostbyname(ServerName);

	int nAdapter = 0;
	cout << "Your ip-addresses:" << endl;
	while (sh->h_addr_list[nAdapter])
	{
		struct sockaddr_in adr;
		memcpy(&adr.sin_addr, sh->h_addr_list[nAdapter], sh->h_length);
		cout << inet_ntoa(adr.sin_addr);
		nAdapter++;
		cout << endl;
	}

	cout << "Waiting for client connection..." << endl;

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

	cout << "Client has connected." << endl;

		//передаем имя компьютера
	int iSendResult = send(ClientSocket, ServerName, SIZE_OF_NAME_SERVER, 0);

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

	hNamedPipe = CreateNamedPipe(L"\\\\.\\pipe\\synchroPipe", PIPE_ACCESS_DUPLEX, NULL, 1, 100, 100, INFINITE, &sa);
	if(hNamedPipe == INVALID_HANDLE_VALUE)
	{	
		cout << "Pipe creating failed. Error: " << GetLastError() << endl;
		_getch();
		return 0;
	}

	if(!ConnectNamedPipe(hNamedPipe, NULL))
	{
		cout << "Pipe connecting error: " << GetLastError() << endl;
	}

	hKeybordPipe = CreateNamedPipe(L"\\\\.\\pipe\\keyboardPipe", PIPE_ACCESS_DUPLEX, NULL, 1, 1000, 1000, INFINITE, &sa);
	if(hKeybordPipe == INVALID_HANDLE_VALUE)
	{	
		cout << "Pipe creating failed. Error: " << GetLastError() << endl;
		_getch();
		return 0;
	}

	if(!ConnectNamedPipe(hKeybordPipe, NULL))
	{
		cout << "Pipe connecting error: " << GetLastError() << endl;
	}
	
	HANDLE hThreadImage;
	DWORD IDThreadImage;
	hThreadImage = CreateThread(NULL, NULL, ThreadRecvImageFunction, (void*)ClientSocket, 0, &IDThreadImage);
	
	hHookMouse = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
	hHookKeyboard = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0); 
	MSG msg = { 0 };

	while(GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
        DispatchMessage(&msg);
	}

	freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
	CloseHandle(hNamedPipe);
	CloseHandle(hEventNextUpdate);
	CloseHandle(hEventMouse);
	TerminateThread(hThreadImage, NO_ERROR);

	UnhookWindowsHookEx(hHookMouse);
	UnhookWindowsHookEx(hHookKeyboard);

	remove("TempImageServer.bmp");
	remove("TempImageServer.jpeg");
	cout << "end of all";
	_getch();
	
	return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	MSLLHOOKSTRUCT *mh = (MSLLHOOKSTRUCT*)lParam;
	signed short length;

	if(hWndImage != GetForegroundWindow())
		return CallNextHookEx(hHookMouse, nCode, wParam, lParam);

	WaitForSingleObject(hEventMouse, INFINITE);

	switch(wParam)
	{
	case WM_MOUSEMOVE:	
		SendToClient(mh->pt.x, mh->pt.y, WM_MOUSEMOVE, '\0', 0);
		break;
	case WM_LBUTTONDOWN:
		SendToClient(mh->pt.x, mh->pt.y, WM_LBUTTONDOWN, '\0', 0);
		break;
	case WM_LBUTTONUP:
		SendToClient(mh->pt.x, mh->pt.y, WM_LBUTTONUP, '\0', 0);
		break;
	case WM_RBUTTONDOWN:
		SendToClient(mh->pt.x, mh->pt.y, WM_RBUTTONDOWN, '\0', 0);
		break;
	case WM_RBUTTONUP:
		SendToClient(mh->pt.x, mh->pt.y, WM_RBUTTONUP, '\0', 0);
		break;
	case WM_MBUTTONDOWN:
		SendToClient(mh->pt.x, mh->pt.y, WM_MBUTTONDOWN, '\0', 0);
		break;
	case WM_MBUTTONUP:
		SendToClient(mh->pt.x, mh->pt.y, WM_MBUTTONUP, '\0', 0);
		break;
	case WM_MOUSEWHEEL:
		length = HIWORD(mh->mouseData);
		length /= 120;
		if(length > 0)
			SendToClient(mh->pt.x, mh->pt.y, WM_MOUSEWHEEL, '+', length);
		if(length < 0)
			SendToClient(mh->pt.x, mh->pt.y, WM_MOUSEWHEEL, '-', length);
		break;
	case WM_MOUSEHWHEEL:
		length = HIWORD(mh->mouseData);
		length /= 120;
		if(length > 0)
			SendToClient(mh->pt.x, mh->pt.y, WM_MOUSEHWHEEL, '+', length);
		if(length < 0)
			SendToClient(mh->pt.x, mh->pt.y, WM_MOUSEHWHEEL, '-', length);
		break;
	default:
		 break;
	}
 
    return CallNextHookEx(hHookMouse, nCode, wParam, lParam);
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT*) lParam;
	DWORD numberOfWrittenBytes;
	char sendbuf[10] = {'\0'};
	if(hWndImage != GetForegroundWindow())
		return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);

	sprintf(sendbuf, "%d_%d_", wParam, ks->vkCode);
	WriteFile(hKeybordPipe, sendbuf, 10, &numberOfWrittenBytes, NULL);

    return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);
}

void SendToClient(int x, int y, int mouseButton, char c, int delta)
{
	int iSendResult;
	char sendbuf[20];
	int sendbuflen = 20;	

	if(x > ResolutionScreenClientX)
		x = ResolutionScreenClientX - 1;
	if(y > ResolutionScreenClientY)
		y = ResolutionScreenClientY - 1;

	sprintf(sendbuf, "%d_%d_%d_%c%d\0", (int)(x * kCursorX), (int)(y * kCursorY), mouseButton, c, delta);
	
	iSendResult = send(ClientSocket, sendbuf, sendbuflen, 0);
	if (iSendResult == SOCKET_ERROR)
	{
		cout << "Send of cursor failed. Error: " << WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();

		CloseHandle(hNamedPipe);
		CloseHandle(hEventNextUpdate);
		CloseHandle(hEventMouse);

		UnhookWindowsHookEx(hHookMouse);
		UnhookWindowsHookEx(hHookKeyboard);
		cout << "Client disconnected." << endl;
		_getch();
		return;
	}
	SetEvent(hEventMouse);
}

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

	hWndImage = CreateNewWindow(ResolutionScreenClientX, ResolutionScreenClientY);

	// получаем изображение
	HANDLE hFileImage;
	do 
	{
		char* bufferForImage = NULL;
		char* bufferForSize = NULL;
		unsigned int sizeOfFile = 0;
		//lpParameters contain  ClientSocket
		//сначала считываем размер, потом сам файл
		bufferForSize = new char[10];
		int i = 0;
		while(i < 10)
			bufferForSize[i++] = NULL;

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
			cout << "Recv() failed. Error: " << WSAGetLastError() << endl;
			_getch();
			return 0;
		}
		free(bufferForSize);

		bufferForImage = new char[sizeOfFile];

		WriteFile(hNamedPipe, "*", 1, &numberOfWrittenBytes, NULL);

		iResult = recv(ClientSocket, bufferForImage, sizeOfFile, MSG_WAITALL);
		
		if (iResult == 0)
		{
			cout << "Connection closed. Image not received." << endl;
			CloseHandle(hNamedPipe);
			CloseHandle(hEventNextUpdate);
			CloseHandle(hEventMouse);
			UnhookWindowsHookEx(hHookMouse);
			UnhookWindowsHookEx(hHookKeyboard);
			_getch();
			return 0;
		}
		if (iResult < 0)
		{
			cout << "Recv failed. Error: " << WSAGetLastError() << endl;
			_getch();
			return 0;
		}
		do
		{
			hFileImage = CreateFile(L"TempImageServer.jpeg", GENERIC_READ | GENERIC_WRITE, 0,NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		}
		while(hFileImage == INVALID_HANDLE_VALUE);

		WriteFile(hFileImage, bufferForImage, sizeOfFile, &numberOfWrittenBytes, NULL);	

		if(!ConvertToBMP(hFileImage))
		{
			free(bufferForImage);
			continue;
		}

		free(bufferForImage);

		InvalidateRect(hWndImage, NULL, NULL);
		WaitForSingleObject(hEventNextUpdate, INFINITE);
		UpdateWindow(hWndImage);
		CloseHandle(hFileImage);
	} 
	while (1);
}