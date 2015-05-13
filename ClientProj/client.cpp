#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

#include "ScreenShotFunction.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define IP_SERVER "192.168.0.100"

SOCKET ConnectSocket;
HANDLE hNamedPipe;

void StringToCursorPos(char* recvbuf);
DWORD WINAPI ThreadSendImageFunction (LPVOID lpParameters);

int main() 
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) 
	{
		cout << "WSAStartup failed: " << iResult << endl;
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
		
		return 0;
	}

	cout << "Connection to server was succesful." << endl;

	if(!WaitNamedPipe(L"\\\\Denis1\\pipe\\qwe", 1000))
	{
		cout << "Pipe waiting error." << GetLastError() << endl;
		_getch();
		return 0;
	}
	
	hNamedPipe = CreateFile(L"\\\\Denis1\\pipe\\qwe", GENERIC_READ | GENERIC_WRITE, 0 , NULL, OPEN_EXISTING, 0, NULL);
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
		{
			//cout << "Bytes received: " << iResult << endl;
			StringToCursorPos(recvbuf);		//переводит в точку и устанавливает курсор
			//cout << recvbuf;
		}
		else if (iResult == 0)
			cout << "Connection closed." << endl;
		else
			cout << "Recv failed: " << WSAGetLastError() << endl;
	
	} 
	while (iResult > 0);
	//freeaddrinfo(result);
	//WSACleanup();
	TerminateThread(hThread, NO_ERROR);
	CloseHandle(hNamedPipe);
	cout << "end of all";
	_getch();
	return 0;
}

void StringToCursorPos(char* recvbuf)
{
	int mouseAction = 0;
	HWND hwnd;
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

	
	hwnd = WindowFromPoint(pt);
	switch(mouseAction)
	{
	case WM_MOUSEMOVE:
		break;
	case WM_LBUTTONDOWN:
		/*INPUT inputstracture;
		inputstracture.type = INPUT_MOUSE;
		inputstracture.mi.dx = pt.x;
		inputstracture.mi.dy = pt.y;
		inputstracture.mi.mouseData = NULL;
		inputstracture.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		SendInput(1, &inputstracture, sizeof(inputstracture));
		inputstracture.mi.dwFlags = MOUSEEVENTF_LEFTUP;
		SendInput(1, &inputstracture, sizeof(inputstracture));*/
		
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN, pt.x, pt.y, 0, 0); // нажали
		cout << "left_down" << endl;
		break;
	case WM_LBUTTONUP:	
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP, pt.x, pt.y, 0, 0); //отпустили
		cout << "left_up" << endl;
		break;
	case WM_RBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTDOWN, pt.x, pt.y, 0, 0); // нажали
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0); //отпустили
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
			return 0;
		}
	do 
	{
		//lpParameters contain  ConnectSocket
		
		char* bufferForImage = NULL;
		char* bufferForSize = NULL;
		unsigned int sizeOfFile = 0;
		unsigned int numberOfbytes = 0;

		
		GetScreenShot();

		char bufferSynch[1];
		DWORD numberOfReadBytes;
		bufferSynch[0] = '\0';
		//cout << "waiting for read" << endl;
		
		//cout << "posle screenshot " << numberOfReadBytes << endl;

		//WaitForSingleObject(hEventStartScreen, INFINITE);	

		FILE *fImage = fopen("TempImage.jpeg","rb");

		fseek (fImage , 0 , SEEK_END);
		sizeOfFile = ftell (fImage);
		rewind (fImage);

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

		numberOfbytes = fread(bufferForImage, 1, sizeOfFile, fImage);
		
		numberOfReadBytes = 0;
		bufferSynch[0] = '\0';
		do{
		ReadFile(hNamedPipe, bufferSynch, 1, &numberOfReadBytes, NULL);
		}while(numberOfReadBytes == 0);
		iSendResult = send((SOCKET)lpParameters, bufferForImage, sizeOfFile, 0);

		cout << "send: " << iSendResult << endl;
		if (iSendResult == SOCKET_ERROR)
		{
			cout << "Send failed: " << WSAGetLastError() << endl;
			closesocket((SOCKET)lpParameters);
			WSACleanup();
			return 0;
		}
		free(bufferForImage);
		free(bufferForSize);
		fclose(fImage);

		//bufferSynch[0] = '\0';
		
			//ReadFile(hNamedPipe, bufferSynch, 1, &numberOfReadBytes, NULL);
		//	cout << numberOfReadBytes << endl;
		
		//WaitForSingleObject(hEventStartScreen); // ждем готовности принять изображение

		//Sleep(1000);

	} 
	while (1);
}