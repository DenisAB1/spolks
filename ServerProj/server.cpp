#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

using namespace std;

HHOOK hHook;
SOCKET ClientSocket;		//переделать на локальное объявление

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"

void CursorPosToString(char* sendbuf, int mouseButton);
void SendToClient(int mouseButton);
//LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);

int main() 
{

	
	/*int choice = 1;
	do
	{
		cout << "Input 1: ";
		HWND hwnd;
		POINT pt;
		cin >> choice;
		GetCursorPos(&pt);
		hwnd = WindowFromPoint(pt);
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN, pt.x, pt.y, 0, 0); // нажали
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP, pt.x, pt.y, 0, 0); //отпустили
	}
	while(choice);*/

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

	//char sendbuf[128];							//
	//char xPos[5];
	//char yPos[5];
	//int iSendResult;
	//int sendbuflen = 128;						//
	POINT pt;

	
	hHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
	MSG msg = { 0 };

	while(GetMessage(&msg, NULL, 0, 0));

//	Sleep(11);
	//do 
	//{


		/*CursorPosToString(sendbuf);
	
		sendbuflen = strlen(sendbuf);
		sendbuf[sendbuflen] = '\0';*/
		/*iSendResult = send(ClientSocket, sendbuf, sendbuflen+1, 0);
		if (iSendResult == SOCKET_ERROR) 
		{
			cout << "Send failed: " << WSAGetLastError() << endl;
			closesocket(ClientSocket);
			WSACleanup();
			return 0;
		}*/
		//printf("Bytes sent: %d\n", iSendResult);
		
	//} 
	//while (sendbuf[0] != 'q');

	freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
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
	case WM_LBUTTONDOWN:
		cout << "Left button clicked!" << endl;
		SendToClient(WM_LBUTTONDOWN);
		break;
	case WM_RBUTTONDOWN:
		cout << "Right button clicked!" << endl;
		SendToClient(WM_RBUTTONDOWN);
		break;
	case WM_MOUSEMOVE:
		//cout << "Mouse is moving!" << endl;
		SendToClient(WM_MOUSEMOVE);
		Sleep(5);
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