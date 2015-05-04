#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define IP_SERVER "192.168.43.16"

void StringToCursorPos(char* recvbuf);

int main() 
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;
	SOCKET ConnectSocket;
	

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
		WSACleanup();
		return 0;
	}

	cout << "Connection to server was succesful." << endl;

	char recvbuf[128];							//
	int iSendResult;
	int recvbuflen = 128;						//

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
	cout << "end of all";
	_getch();
	return 0;
}

void StringToCursorPos(char* recvbuf)
{
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
	while(recvbuf[i] != '\0')
	{
		pt.y *= 10;
		pt.y += recvbuf[i++] - '0';
	}
	SetCursorPos(pt.x, pt.y);
}