#include "Network.h"
#include "ClientNetwork.h"
#include "ServerNetwork.h"

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
        std::cerr << "Failure at getaddrinfo: " << addrResultCode << std::endl;
        exit(3);
    }

    return result;
}

SOCKET CreateSocket(const addrinfo& availableInfo) {

    SOCKET newSocket = socket(availableInfo.ai_family, availableInfo.ai_socktype, availableInfo.ai_protocol);
    if (newSocket == INVALID_SOCKET) {
        std::cerr << "Error occured: " << WSAGetLastError() << std::endl;
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
            std::cerr << "Error occured at receiving message: " << WSAGetLastError() << std::endl;
            closesocket(client);
            return;
        }
        else if (receiveStatus > 0) {
            ResolveMessage(client, buffer, receiveStatus);
        }
        else if (receiveStatus == 0) {

            if (shutdown(client, SD_SEND) == SOCKET_ERROR) {
                std::cerr << "Shutdown failed: " << WSAGetLastError() << std::endl;
                std::cout << "Closing the socket anyways." << std::endl;
            }
            closesocket(client);

            int index = GetSocketIndex(client);
            if (index > -1) connectedSockets.erase(connectedSockets.begin() + index);
            std::cout << "Connection closed." << std::endl;
            return;
        }
        else {
            std::cerr << "Unknown scenario." << std::endl;
            std::cerr << "Receive status: " << receiveStatus << std::endl;
        }

    }

}

void SendMessageTo(SOCKET client, const char* message, int length) {
    if (length > MESSAGE_BUFFER_LENGTH) length = MESSAGE_BUFFER_LENGTH;
    if (send(client, message, length, 0) == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        return;
    }

}

int GetSocketIndex(SOCKET socket) {
    int index = -1;
    for (int i = 0; i < connectedSockets.size(); i++)
        if (connectedSockets.at(i) == socket) index = i;

    return index;
}

void Disconnect(SOCKET socket) {
    exitSignalReceived = true;
    SendMessageTo(socket, "", 0);
    shutdown(socket, SD_SEND);
    closesocket(socket);
    return;
}

void ResolveMessage(SOCKET client, char(&buffer)[MESSAGE_BUFFER_LENGTH], int messageLength) {

    int index = GetSocketIndex(client);
    if (index < 0) {
        std::cerr << "Could not find index of socket." << std::endl;
        return;
    }

    switch (buffer[0]) {
    case 'N': SetSocketName(index, buffer); break;
    case 'P': SendOK(client); break;
    }

    std::cout << socketNames[index] << ": " << buffer << std::endl;
}