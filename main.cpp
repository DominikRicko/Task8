﻿#include "config.h"

std::vector<SOCKET> connectedSockets;
std::vector<std::string> socketNames;
volatile bool exitSignalReceived = false;
int coutner;

int GetSocketIndex(SOCKET socket) {
    int index = -1;
    for (int i = 0; i < connectedSockets.size(); i++)
        if (connectedSockets.at(i) == socket) index = i;

    return index;
}

void PrintToConsole(std::string message) {
    //Tu treba kritični odsječak
    std::cout << message << std::endl;
}

void SendMessageTo(SOCKET client, const char* message, int length) {
    if (length > MESSAGE_BUFFER_LENGTH) length = MESSAGE_BUFFER_LENGTH;
    if (send(client, message, length, 0) == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
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

void SendOK(SOCKET socket) {
    SendMessageTo(socket, "OK", sizeof("OK"));
    Disconnect(socket);
}

void SendAmountOfUsers(SOCKET client) {
    std::string message = "B";
    char usersConnectedCString[2];
    _itoa_s(connectedSockets.size(), usersConnectedCString, 10);

    message += usersConnectedCString;

    SendMessageTo(client, message.c_str(), message.size());
    if (connectedSockets.size() >= 4) {
        for (auto socket : connectedSockets) {
            SendMessageTo(socket, "P", sizeof("P"));
        }
    }
}

void SetSocketName(int socketIndex, char(&buffer)[MESSAGE_BUFFER_LENGTH]) {

    socketNames[socketIndex] = buffer + 1;
    SendAmountOfUsers(connectedSockets[socketIndex]);
}

void ResolveMessage(SOCKET client, char (&buffer)[MESSAGE_BUFFER_LENGTH], int messageLength) {

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
        else if(receiveStatus > 0){
            ResolveMessage(client, buffer, receiveStatus);
        }
        else if(receiveStatus == 0){
            
            if (shutdown(client, SD_SEND) == SOCKET_ERROR) {
                std::cerr << "Shutdown failed: " << WSAGetLastError() << std::endl;
                std::cout << "Closing the socket anyways." << std::endl;
            }
            closesocket(client);

            int index = GetSocketIndex(client);
            if(index > -1) connectedSockets.erase(connectedSockets.begin() + index);
            std::cout << "Connection closed." << std::endl;
            return;
        }
        else {
            std::cerr << "Unknown scenario." << std::endl;
            std::cerr << "Receive status: " << receiveStatus << std::endl;
        }

    }

}

void ListenForIncomingConnections(SOCKET hostSocket) {

    u_long iMode = 0;

    if (listen(hostSocket, 4) == SOCKET_ERROR) {
        std::cerr << "Listen failed with error: " << WSAGetLastError() << std::endl;
        exit(10);
    }

    std::cout << "Listening for connections." << std::endl;

    while (!exitSignalReceived) {

        SOCKET client = accept(hostSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(client);
            continue;
        }

        if (connectedSockets.size() > 4) {
            std::cerr << "New connection rejected." << std::endl;
            closesocket(client);
            continue;
        }

        if (int result = ioctlsocket(client, FIONBIO, &iMode) != NO_ERROR) {
            std::cerr << "Unable to set socket as blocking: " << result << std::endl;
            exit(11);
        }

        connectedSockets.push_back(client);
        socketNames.push_back("Client" + socketNames.size());
        std::thread clientThread(ReceiveMessage, client);
        clientThread.detach();

    }
    
}

void StartServer(SOCKET socket, addrinfo& addressInfo) {

    int socketResult = bind(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen);
    if (socketResult == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        exit(4);
    }

    ListenForIncomingConnections(socket);
    
}

void StartClient(std::string name, SOCKET socket, addrinfo& addressInfo) {

    if (connect(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen) == SOCKET_ERROR) {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    std::string initialMessage = "N" + name;

    std::cout << "Connected to server." << std::endl;
    connectedSockets.push_back(socket);
    socketNames.push_back("Server");
    SendMessageTo(socket, initialMessage.c_str(), sizeof(char)* initialMessage.size());

    ReceiveMessage(socket);
}

SOCKET CreateSocket(const addrinfo& availableInfo) {

    SOCKET newSocket = socket(availableInfo.ai_family, availableInfo.ai_socktype, availableInfo.ai_protocol);
    if (newSocket == INVALID_SOCKET) {
        std::cerr << "Error occured: " << WSAGetLastError() << std::endl;
        exit(2);
    }

    return newSocket;
}

addrinfo* GetAddressInfo(char *port, bool isServer) {
    addrinfo service;
    addrinfo* result = NULL;
    memset(&service, 0, sizeof(service));

    service.ai_family = AF_INET;
    service.ai_socktype = SOCK_STREAM;
    service.ai_protocol = IPPROTO_TCP;
    if(isServer) service.ai_flags = AI_PASSIVE;

    int addrResultCode = getaddrinfo(NULL, port, &service, &result);
    if (addrResultCode != 0) {
        std::cerr << "Failure at getaddrinfo: " << addrResultCode << std::endl;
        exit(3);
    }

    return result;
}

bool ResolveCommand(std::string command) {

    PrintToConsole(command);

    if (std::regex_match(command, std::regex("-startListening *[0-9]+"))) {



        return true;
    }
    if (std::regex_match(command, std::regex("-connect *[0-9]+"))) {



        return true;
    }

    return false;
}

void mainNetworking(std::string name, int port, bool isServer) {

    // Common intialization for both client and server
    
    char portString[5];
    _itoa_s(port, portString, 10);

    addrinfo* result = GetAddressInfo(portString, isServer);

    SOCKET socket = CreateSocket(*result);

    if (isServer) StartServer(socket, *result);
    else StartClient(name, socket, *result);

    closesocket(socket);
    freeaddrinfo(result);
}

int main(int argc, char* argv[])
{

    WSADATA wsaData;
    if (int err = WSAStartup(DEFAULT_WINSOCK_VERSION, &wsaData)) {
        std::cerr << "WSAStartup failed with error: " << err << std::endl;
        exit(1);
    }

    //std::thread networkThread(mainNetworking, name, port, isServer);

    while (!exitSignalReceived) {
        std::string userInput;
        std::getline(std::cin, userInput);
        if (exitSignalReceived) {
            std::cout << "Program terminated." << std::endl;
            break;
        }

        if (ResolveCommand(userInput)) {
            continue;
        }

        if (userInput.length() == 0) {
            continue;
        }
        
        for (auto client : connectedSockets) {
            SendMessageTo(client, userInput.c_str(), userInput.size());
        }

    }

    WSACleanup();
    return 0;
}
