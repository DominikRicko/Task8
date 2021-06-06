#pragma once

#include <WinSock2.h>
#include "Console.h"
#include "Network.h"

void StartClient(SOCKET socket, addrinfo& addressInfo);
SOCKET connectedSocket = NULL;