#pragma once

#include <WinSock2.h>
#include "Console.h"
#include "Network.h"

void StartServer(SOCKET socket, addrinfo& addressInfo);
SOCKET listeningSocket = NULL;