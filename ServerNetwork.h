#pragma once

#include "config.h"

void StartServer(SOCKET socket, addrinfo& addressInfo);
void ListenForIncomingConnections(SOCKET hostSocket);
void SendMessageTo(SOCKET client, const char* message, int length);