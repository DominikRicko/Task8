#pragma once

#include <WinSock2.h>
#include <ws2tcpip.h>
#include "Console.h"
#include "ClientNetwork.h"
#include "ServerNetwork.h"

void mainNetworking(std::string port, bool isServer);
addrinfo* GetAddressInfo(char* port, bool isServer);
SOCKET CreateSocket(const addrinfo& availableInfo);
void ReceiveMessage(SOCKET client);
void SendMessageTo(SOCKET client, const char* message, int length);
void Disconnect(SOCKET socket);

volatile bool exitSignalReceived = false;

#define MESSAGE_BUFFER_LENGTH 1024