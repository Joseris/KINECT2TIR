#include "UDPSender.h"

#include <winsock2.h>
#include <Ws2tcpip.h>

// Link with ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

SOCKET SendSocket = INVALID_SOCKET;
sockaddr_in RecvAddr;

int BufLen = sizeof(T6DOF);

WSADATA wsaData;

int InitUDPSender(unsigned long ulIP, int iPort)
{
	int iResult;
	//----------------------
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR) {
		return 1;
	}

	//---------------------------------------------
	// Create a socket for sending data
	SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (SendSocket == INVALID_SOCKET) {
		WSACleanup();
		return 1;
	}

	//---------------------------------------------
	// Set up the RecvAddr structure with the IP address of
	// the receiver (in this example case "192.168.1.1")
	// and the specified port number.
	RecvAddr.sin_family = AF_INET;
	RecvAddr.sin_port = htons(iPort);
	RecvAddr.sin_addr.s_addr = ulIP;
}

int SendUDP(T6DOF *headpose)
{
	int iResult;

	//---------------------------------------------
	// Send a datagram to the receiver
	iResult = sendto(SendSocket,
		(const char *) headpose, BufLen, 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
	if (iResult == SOCKET_ERROR) {
		closesocket(SendSocket);
		WSACleanup();
		return 1;
	}
	return 0;
}

int CloseUDPSender()
{
	int iResult;

	//---------------------------------------------
	// When the application is finished sending, close the socket.
	iResult = closesocket(SendSocket);
	if (iResult == SOCKET_ERROR) {
		WSACleanup();
		return 1;
	}
	//---------------------------------------------
	// Clean up.
	WSACleanup();
}