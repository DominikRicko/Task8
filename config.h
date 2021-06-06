#pragma once

#include <iostream>
#include <WinSock2.h>
#include <string>
#include <vector>
#include <regex>
#include <thread>
#include <ws2tcpip.h>
#include <mutex>

void mainNetworking(std::string port, bool isServer);
addrinfo* GetAddressInfo(char* port, bool isServer);
SOCKET CreateSocket(const addrinfo& availableInfo);
void ReceiveMessage(SOCKET client);
void SendMessageTo(SOCKET client, const char* message, int length);
void Disconnect(SOCKET socket);

void StartClient(SOCKET socket, addrinfo& addressInfo);
SOCKET outgoingSocket = INVALID_SOCKET;

void StartServer(SOCKET socket, addrinfo& addressInfo);
SOCKET incomingSocket = INVALID_SOCKET;
SOCKET listeningSocket = INVALID_SOCKET;


void PrintToConsole(std::string message);
volatile bool exitSignalReceived = false;

using std::string;
using std::to_string;

#define MESSAGE_BUFFER_LENGTH 1024
#define DEFAULT_WINSOCK_VERSION MAKEWORD(2, 2)
#pragma comment(lib, "Ws2_32.lib")