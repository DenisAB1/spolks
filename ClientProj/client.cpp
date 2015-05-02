#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <conio.h>
#include <stdio.h>

using namespace std;

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "27015"
#define IP_SERVER "192.168.0.101"

int main() 
{
	WSADATA wsaData;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	int iResult;
	SOCKET ConnectSocket;
	

	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) 
	{
		printf("WSAStartup failed: %d\n", iResult);
		return 0;
	}

	ZeroMemory( &hints, sizeof(hints) );
	hints.ai_family = AF_INET;				// изменение
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	iResult = getaddrinfo(IP_SERVER, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) 
	{
		printf("Getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 0;
	}

	ptr=result;

		// создаем сокет для подключения к серверу
	ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (ConnectSocket == INVALID_SOCKET) 
	{
		printf("Error at socket(): %ld\n", WSAGetLastError());
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
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 0;
	}

	//freeaddrinfo(result);
	//WSACleanup();
	cout << "end of all";
	_getch();
	return 0;
}