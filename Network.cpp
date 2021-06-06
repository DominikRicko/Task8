#include "Network.h"

void mainNetworking(std::string port, bool isServer) {

    // Common intialization for both client and server

    addrinfo* result = GetAddressInfo((char*)port.c_str(), isServer);

    SOCKET socket = CreateSocket(*result);

    if (isServer) StartServer(socket, *result);
    else StartClient(socket, *result);

    closesocket(socket);
    freeaddrinfo(result);
}

addrinfo* GetAddressInfo(char* port, bool isServer) {
    addrinfo service;
    addrinfo* result = NULL;
    memset(&service, 0, sizeof(service));

    service.ai_family = AF_INET;
    service.ai_socktype = SOCK_STREAM;
    service.ai_protocol = IPPROTO_TCP;
    if (isServer) service.ai_flags = AI_PASSIVE;

    int addrResultCode = getaddrinfo(NULL, port, &service, &result);
    if (addrResultCode != 0) {
        PrintToConsole("Failure at getaddrinfo: " + addrResultCode);
        exit(3);
    }

    return result;
}

SOCKET CreateSocket(const addrinfo& availableInfo) {

    SOCKET newSocket = socket(availableInfo.ai_family, availableInfo.ai_socktype, availableInfo.ai_protocol);
    if (newSocket == INVALID_SOCKET) {
        PrintToConsole("Error occured: " + WSAGetLastError());
        exit(2);
    }

    return newSocket;
}

void ReceiveMessage(SOCKET client) {

    char buffer[MESSAGE_BUFFER_LENGTH];

    while (!exitSignalReceived) {

        memset(buffer, 0, sizeof(buffer));
        int receiveStatus = recv(client, buffer, MESSAGE_BUFFER_LENGTH, 0);
        if (receiveStatus == SOCKET_ERROR) {
            PrintToConsole("Error occured at receiving message: " + WSAGetLastError());
            closesocket(client);
            return;
        }
        else if (receiveStatus > 0) {
            PrintToConsole(buffer);
            if (client == listeningSocket) SendMessageTo(connectedSocket, buffer, receiveStatus);
            else SendMessageTo(listeningSocket, buffer, receiveStatus);
        }
        else if (receiveStatus == 0) {

            if (shutdown(client, SD_SEND) == SOCKET_ERROR) {
                PrintToConsole("Shutdown failed: " + WSAGetLastError());
                PrintToConsole("Closing the socket anyways.");
            }
            closesocket(client);

            if (client == listeningSocket) listeningSocket = NULL;
            if (client == connectedSocket) connectedSocket = NULL;
            PrintToConsole("Connection closed.");
            return;
        }
        else {
            PrintToConsole("Unknown scenario.");
            PrintToConsole("Receive status: " + receiveStatus);
        }

    }

}

void SendMessageTo(SOCKET client, const char* message, int length) {
    if (client == NULL) return;
    if (length > MESSAGE_BUFFER_LENGTH) length = MESSAGE_BUFFER_LENGTH;
    if (send(client, message, length, 0) == SOCKET_ERROR) {
        PrintToConsole("Send failed: " + WSAGetLastError());
        return;
    }

}

void Disconnect(SOCKET socket) {
    exitSignalReceived = true;
    SendMessageTo(socket, "", 0);
    shutdown(socket, SD_SEND);
    closesocket(socket);
    return;
}