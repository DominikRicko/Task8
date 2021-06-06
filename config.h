#pragma once

#include <iostream>
#include <WinSock2.h>
#include <string>
#include <vector>
#include <regex>
#include <thread>

#include "Network.h"
#define DEFAULT_WINSOCK_VERSION MAKEWORD(2, 2)
#pragma comment(lib, "Ws2_32.lib")