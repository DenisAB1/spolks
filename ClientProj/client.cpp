#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

#include "ScreenShotFunction.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
char* IP_SERVER = { "192.168.0.101"};

SOCKET ConnectSocket;
HANDLE hNamedPipe;

void StringToCursorPos(char* recvbuf);
DWORD WINAPI ThreadSendImageFunction (LPVOID lpParameters);

int main() 
{
	//char IP_SERVER[15];
	//cout << "Enter IP of computer-server: ";
	//cin >> IP_SERVER;
	char ServerName[20];
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) 
	{
		cout << "WSAStartup failed: " << iResult << endl;
		_getch();
		return 0;
	}

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_INET;				// изменение
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	iResult = getaddrinfo(IP_SERVER, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) 
	{
		cout << "Getaddrinfo failed: " << iResult << endl;
		WSACleanup();
		_getch();
		return 0;
	}
	ptr=result;

		// создаем сокет для подключения к серверу
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) 
	{
		cout << "Error at socket(): " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		_getch();
		return 0;
	}
			// пытаемся соединиться с сервером
	iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);	
	if (iResult == SOCKET_ERROR) 
	{
		closesocket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) 
	{
		cout << "Unable to connect to server!" << endl;
		_getch();
		return 0;
	}

	cout << "Connection to server was succesful." << endl;

	
	DWORD sizeOfServerName;
	//Sleep(1000);
	iResult = recv(ConnectSocket, ServerName, 7, 0);
	if (iResult == 0)
	{
		cout << "Connection closed." << endl;
		_getch();
		return 0;
	}
	if (iResult < 0)
	{
		cout << "Getting server name failed: " << WSAGetLastError() << endl;
		_getch;
		return 0;
	}

	char synchPipeName[40];
	sprintf(synchPipeName, "%s%s%s", "\\\\", ServerName, "\\pipe\\synchroPipe");
	cout << synchPipeName << endl;
	if(!WaitNamedPipeA(synchPipeName, 1000))
	{
		cout << "Pipe waiting error." << GetLastError() << endl;
		_getch();
		return 0;
	}
	
	hNamedPipe = CreateFileA(synchPipeName, GENERIC_READ | GENERIC_WRITE, 0 , NULL, OPEN_EXISTING, 0, NULL);
	if(hNamedPipe == INVALID_HANDLE_VALUE)
	{
		
		cout << "Pipe creating error." << GetLastError() << endl;
		_getch();
		return 0;
	}

	char recvbuf[128];							//
	int iSendResult;
	int recvbuflen = 128;						//

	HANDLE hThread;
	DWORD IDThread;

	hThread = CreateThread(NULL, NULL, ThreadSendImageFunction, (void*)ConnectSocket, 0, &IDThread);

	do 
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			StringToCursorPos(recvbuf);		//переводит в точку и устанавливает курсор
		else if (iResult == 0)
		{
			cout << "Connection closed." << endl;
			_getch();
			return 0;
		}
		else
		{
			cout << "Recv failed: " << WSAGetLastError() << endl;
			_getch();
			return 0;
		}
	
	} 
	while (iResult > 0);
	//freeaddrinfo(result);
	//WSACleanup();
	//remove("TempImageClient.bmp");
	//remove("TempImageClient.jpeg");
	TerminateThread(hThread, NO_ERROR);
	CloseHandle(hNamedPipe);
	cout << "end of all";
	_getch();
	return 0;
}

void StringToCursorPos(char* recvbuf)
{
	int mouseAction = 0;
	//HWND hwnd;
	POINT pt;
	pt.x = 0;
	pt.y = 0;

	int i = 0;
	while(recvbuf[i] != '_')
	{
		pt.x *= 10;
		pt.x += recvbuf[i++] - '0';
	}
	i++;
	while(recvbuf[i] != '_')
	{
		pt.y *= 10;
		pt.y += recvbuf[i++] - '0';
	}
	i++;
	while(recvbuf[i] != '\0')
	{
		mouseAction *= 10;
		mouseAction += recvbuf[i++] - '0';
	}
	SetCursorPos(pt.x, pt.y);

	
	//hwnd = WindowFromPoint(pt);
	switch(mouseAction)
	{
	case WM_MOUSEMOVE:
		break;
	case WM_LBUTTONDOWN:	
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN, pt.x, pt.y, 0, 0); // нажали
		cout << "left_down" << endl;
		break;
	case WM_LBUTTONUP:	
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP, pt.x, pt.y, 0, 0); //отпустили
		cout << "left_up" << endl;
		break;
	case WM_RBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTDOWN, pt.x, pt.y, 0, 0); // нажали
		cout << "right_down" << endl;
		break;
	case WM_RBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0); //отпустили
		cout << "right_up" << endl;
		break;
	case WM_MBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0); //отпустили
		cout << "Middle button down!" << endl;
		break;
	case WM_MBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0); //отпустили
		cout << "Middle button up!" << endl;
		break;

	}
}

DWORD WINAPI ThreadSendImageFunction (LPVOID lpParameters)
{
	//передадим серверу разрешение экрана
	int iSendResult = 0;
	int iResult = 0;

	int cxScreen = GetSystemMetrics (SM_CXSCREEN);
    int cyScreen = GetSystemMetrics (SM_CYSCREEN);
	char bufScreen[9] = {'\0'};
	sprintf(bufScreen, "%d_%d", cxScreen, cyScreen);

	iSendResult = send((SOCKET)lpParameters, bufScreen, strlen(bufScreen), 0);
	if (iSendResult == SOCKET_ERROR)
		{
			cout << "Send failed: " << WSAGetLastError() << endl;
			closesocket((SOCKET)lpParameters);
			WSACleanup();
			_getch();
			return 0;
		}
	static int counter = 0;																	//////тест скорости
	do 
	{
		counter++;																			//////тест скорости
		//lpParameters contain  ConnectSocket
		
		char* bufferForImage = NULL;
		char* bufferForSize = NULL;
		unsigned int sizeOfFile = 0;
		unsigned int numberOfbytes = 0;

		
		GetScreenShot();

		char bufferSynch[1];
		DWORD numberOfReadBytes = 0;
		bufferSynch[0] = '\0';

		HANDLE hFileImageC = CreateFile(L"TempImageClient.jpeg", GENERIC_READ, 0,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_TEMPORARY, NULL);

		sizeOfFile = GetFileSize(hFileImageC, NULL); 

		bufferForSize = new char[10];
		itoa(sizeOfFile, bufferForSize, 10);

		numberOfReadBytes = 0;
		bufferSynch[0] = '\0';
		do{
		ReadFile(hNamedPipe, bufferSynch, 1, &numberOfReadBytes, NULL);
		}while(numberOfReadBytes == 0);
		iSendResult = send((SOCKET)lpParameters, bufferForSize, strlen(bufferForSize), 0);
		

		bufferForImage = new char[sizeOfFile];  
		//передача сначала размера изображения, потом самого изображения

		ReadFile(hFileImageC, bufferForImage, sizeOfFile, &numberOfReadBytes, NULL);
		
		numberOfReadBytes = 0;
		bufferSynch[0] = '\0';
		do{
		ReadFile(hNamedPipe, bufferSynch, 1, &numberOfReadBytes, NULL);
		}while(numberOfReadBytes == 0);
		iSendResult = send((SOCKET)lpParameters, bufferForImage, sizeOfFile, 0);

		cout << counter <<" send: " << iSendResult << endl;													//////тест скорости
		if (iSendResult == SOCKET_ERROR)
		{
			cout << "Send failed: " << WSAGetLastError() << endl;
			closesocket((SOCKET)lpParameters);
			WSACleanup();
			_getch();
			return 0;
		}
		free(bufferForImage);
		free(bufferForSize);
		CloseHandle(hFileImageC);
		//remove("TempImageClient.bmp");
		//remove("TempImageClient.jpeg");
	} 
	while (1);
}