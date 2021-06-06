#pragma once

#include "config.h"

void mainNetworking(std::string port, bool isServer);
addrinfo* GetAddressInfo(char* port, bool isServer);
SOCKET CreateSocket(const addrinfo& availableInfo);
void ReceiveMessage(SOCKET client);
void SendMessageTo(SOCKET client, const char* message, int length);
int GetSocketIndex(SOCKET socket);
void Disconnect(SOCKET socket);
void ResolveMessage(SOCKET client, char(&buffer)[MESSAGE_BUFFER_LENGTH], int messageLength);