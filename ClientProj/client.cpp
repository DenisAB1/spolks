#include <Winsock2.h>
#include <Mswsock.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

#include "ScreenShotFunction.h"

using namespace std;

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")

#define DEFAULT_PORT "27015"
#define SIZE_OF_NAME_SERVER 20

SOCKET ConnectSocket;
HANDLE hNamedPipe;
HANDLE hKeyboardPipe;

void DoMouseAction(char* recvbuf);
DWORD WINAPI ThreadSendImageFunction (LPVOID lpParameters);
DWORD WINAPI ThreadGetKeysFunction (LPVOID lpParameters);

int main() 
{
	char IP_SERVER[15];
	cout << "Enter IP of computer-server: ";
	cin >> IP_SERVER;

	char ServerName[20];
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) 
	{
		cout << "WSAStartup failed. Error: " << iResult << endl;
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

	
	iResult = recv(ConnectSocket, ServerName, SIZE_OF_NAME_SERVER, 0);
	if (iResult == 0)
	{
		cout << "Connection closed." << endl;
		_getch();
		return 0;
	}
	if (iResult < 0)
	{
		cout << "Getting server name failed: " << WSAGetLastError() << endl;
		_getch();
		return 0;
	}

	char synchPipeName[40];
	char keyboardPipeName[40];

	sprintf(synchPipeName, "%s%s%s", "\\\\", ServerName, "\\pipe\\synchroPipe");
	sprintf(keyboardPipeName, "%s%s%s", "\\\\", ServerName, "\\pipe\\keyboardPipe");
	

	if(!WaitNamedPipeA(synchPipeName, 1000))
	{
		cout << "Pipe waiting error: " << GetLastError() << endl;
		_getch();
		return 0;
	}
	hNamedPipe = CreateFileA(synchPipeName, GENERIC_READ | GENERIC_WRITE, 0 , NULL, OPEN_EXISTING, 0, NULL);
	if(hNamedPipe == INVALID_HANDLE_VALUE)
	{	
		cout << "Pipe creating error: " << GetLastError() << endl;
		_getch();
		return 0;
	}


	if(!WaitNamedPipeA(keyboardPipeName, 1000))
	{
		cout << "Pipe waiting error: " << GetLastError() << endl;
		_getch();
		return 0;
	}
	hKeyboardPipe = CreateFileA(keyboardPipeName, GENERIC_READ | GENERIC_WRITE, 0 , NULL, OPEN_EXISTING, 0, NULL);
	if(hKeyboardPipe == INVALID_HANDLE_VALUE)
	{
		cout << "Pipe creating error: " << GetLastError() << endl;
		_getch();
		return 0;
	}


	char recvbuf[20];							
	int recvbuflen = 20;						

	HANDLE hThread;
	DWORD IDThread;

	HANDLE hThreadKeyboard;
	DWORD IDThreadKeyboard;

	hThread = CreateThread(NULL, NULL, ThreadSendImageFunction, (void*)ConnectSocket, 0, &IDThread);

	hThreadKeyboard = CreateThread(NULL, NULL, ThreadGetKeysFunction, NULL, 0, &IDThreadKeyboard);

	do 
	{
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
			DoMouseAction(recvbuf);		//переводит в точку и устанавливает курсор
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

	WSACleanup();
	remove("TempImageClient.bmp");
	remove("TempImageClient.jpeg");
	TerminateThread(hThread, NO_ERROR);
	CloseHandle(hNamedPipe);
	cout << "end of all";
	_getch();
	return 0;
}

void DoMouseAction(char* recvbuf)
{
	int mouseAction = 0;
	char sign;
	int delta;
	
	POINT pt;
	pt.x = 0;
	pt.y = 0;

	int i = 0;
	while(recvbuf[i] != '_')				// x
	{
		pt.x *= 10;
		pt.x += recvbuf[i++] - '0';
	}
	i++;
	while(recvbuf[i] != '_')				//y
	{
		pt.y *= 10;
		pt.y += recvbuf[i++] - '0';
	}
	i++;
	while(recvbuf[i] != '_')				//mouseAction
	{
		mouseAction *= 10;
		mouseAction += recvbuf[i++] - '0';
	}
	SetCursorPos(pt.x, pt.y);
	i++;
	sign = recvbuf[i++];					//sign
	delta = recvbuf[i] - '0';				//delta
					
	switch(mouseAction)
	{
	case WM_MOUSEMOVE:
		break;
	case WM_LBUTTONDOWN:	
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTDOWN, pt.x, pt.y, 0, 0);
		break;
	case WM_LBUTTONUP:	
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_LEFTUP, pt.x, pt.y, 0, 0);
		break;
	case WM_RBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTDOWN, pt.x, pt.y, 0, 0);
		break;
	case WM_RBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0);
		break;
	case WM_MBUTTONDOWN:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0);
		break;
	case WM_MBUTTONUP:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_RIGHTUP, pt.x, pt.y, 0, 0);
	case WM_MOUSEWHEEL:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_WHEEL, pt.x, pt.y, 
				(sign == '+') ? WHEEL_DELTA * delta : -WHEEL_DELTA * delta, 0);
		break;
	case WM_MOUSEHWHEEL:
		mouse_event(MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_HWHEEL, pt.x, pt.y, 
				(sign == '+') ? WHEEL_DELTA * delta : -WHEEL_DELTA * delta, 0);
		break;
	default:
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
			remove("TempImageClient.bmp");
			remove("TempImageClient.jpeg");

			_getch();
			return 0;
		}

	int sizeOfPreviousFile = 0;
	do 
	{
		//lpParameters contain  ConnectSocket
		
		char* bufferForImage = NULL;
		char bufferForSize[10] = {'\0'};
		int sizeOfFile = 0;
		
		int numberOfbytes = 0;
		char bufferSynch[1];
		DWORD numberOfReadBytes = 0;
		bufferSynch[0] = '\0';
		HANDLE hFileImageC = NULL;

		do
		{
			CloseHandle(hFileImageC);

			GetScreenShot();
			
			hFileImageC = CreateFile(L"TempImageClient.jpeg", GENERIC_READ, 0,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			while(hFileImageC == INVALID_HANDLE_VALUE)
				hFileImageC = CreateFile(L"TempImageClient.jpeg", GENERIC_READ, 0,NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		
			sizeOfFile = GetFileSize(hFileImageC, NULL); 
			if(sizeOfFile == INVALID_FILE_SIZE)
			{
				cout << "GetFileSize() failed. Error: " << GetLastError() << endl;
				_getch();
				return 0;
			}
		}
		while(sizeOfFile == sizeOfPreviousFile);

		/*int i = 0;
		while(i < 10)
			bufferForSize[i++] = NULL;*/
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

		if (iSendResult == SOCKET_ERROR)
		{
			cout << "Send failed: " << WSAGetLastError() << endl;
			closesocket((SOCKET)lpParameters);
			WSACleanup();
			_getch();
			remove("TempImageClient.bmp");
			remove("TempImageClient.jpeg");
			return 0;
		}
		free(bufferForImage);
		CloseHandle(hFileImageC);
		sizeOfPreviousFile = sizeOfFile;
	} 
	while (1);
}

DWORD WINAPI ThreadGetKeysFunction (LPVOID lpParameters)
{
	do
	{
		int WM_choice = 0;
		int VK_choice = 0;
		char readBuf[10];
		DWORD numberOfReadBytes;
		do
		{
			ReadFile(hKeyboardPipe, readBuf, 10, &numberOfReadBytes, NULL);
		}
		while(numberOfReadBytes == 0);

		int i = 0;
		while(readBuf[i] != '_')				// x
		{
			WM_choice *= 10;
			WM_choice += readBuf[i++] - '0';
		}
		i++;
		while(readBuf[i] != '_')				// y
		{
			VK_choice *= 10;
			VK_choice += readBuf[i++] - '0';
		}

		switch (WM_choice)
		{
			case WM_KEYDOWN:
				keybd_event(VK_choice, NULL, NULL, NULL);
				break;
			case WM_KEYUP:
				keybd_event(VK_choice, NULL, KEYEVENTF_KEYUP, NULL);
				break;
			default:
				break;
		}
	}
	while(1);
}