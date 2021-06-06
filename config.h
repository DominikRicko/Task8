#pragma once

#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <thread>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

#define MESSAGE_BUFFER_LENGTH 1024
#define SERVER_ARGUMENT "-s"
#define CLIENT_ARGUMENT "-c"
#define DEFAULT_PORT 1337
#define DEFAULT_WINSOCK_VERSION MAKEWORD(2, 2)
#define DEFAULT_NAME "Default"
volatile bool exitSignalReceived = false;
std::vector<SOCKET> connectedSockets;

#pragma comment(lib, "Ws2_32.lib")