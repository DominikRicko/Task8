#include "config.h"

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
            if (client == incomingSocket) SendMessageTo(outgoingSocket, buffer, receiveStatus);
            else SendMessageTo(incomingSocket, buffer, receiveStatus);
        }
        else if (receiveStatus == 0) {

            if (shutdown(client, SD_SEND) == SOCKET_ERROR) {
                PrintToConsole("Shutdown failed: " + WSAGetLastError());
                PrintToConsole("Closing the socket anyways.");
            }
            closesocket(client);

            if (client == incomingSocket) incomingSocket = NULL;
            if (client == outgoingSocket) outgoingSocket = NULL;
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
    SendMessageTo(socket, "", 0);
    shutdown(socket, SD_SEND);
    closesocket(socket);
    return;
}

void StartClient(SOCKET socket, addrinfo& addressInfo) {

    if (connect(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen) == SOCKET_ERROR) {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    PrintToConsole("Connected to socket.");
    outgoingSocket = socket;

    ReceiveMessage(socket);
}

void ListenForIncomingConnections(SOCKET hostSocket) {

    u_long iMode = 0;

    if (listen(hostSocket, 1) == SOCKET_ERROR) {
        PrintToConsole("Listen failed with error: " + WSAGetLastError());
        return;
    }

    PrintToConsole("Listening for connections.");

    SOCKET client = accept(hostSocket, NULL, NULL);
    if (client == INVALID_SOCKET) {
        PrintToConsole("Accept failed: " + WSAGetLastError());
        closesocket(client);
        return;
    }

    incomingSocket = client;

    if (int result = ioctlsocket(client, FIONBIO, &iMode) != NO_ERROR) {
        PrintToConsole("Unable to set socket as blocking: " + result);
        return;
    }

    ReceiveMessage(client);

}

void StartServer(SOCKET socket, addrinfo& addressInfo) {

    int socketResult = bind(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen);
    if (socketResult == SOCKET_ERROR) {
        PrintToConsole("Bind failed with error: " + WSAGetLastError());
        exit(4);
    }

    listeningSocket = socket;
    ListenForIncomingConnections(socket);

}


std::mutex consoleMutex;
void PrintToConsole(std::string message) {
    std::unique_lock<std::mutex> lck(consoleMutex, std::defer_lock);
    lck.lock();
    std::cout << message << std::endl;
    lck.unlock();
}

std::vector<std::string> commandSplit(std::string command, char splitter) {
    std::vector<std::string> splitCommand;

    while (command.length() > 0) {

        std::size_t startIndex = command.find(splitter);
        if (startIndex == std::string::npos) {
            std::size_t endIndex = command.find('\n');
            if (endIndex != std::string::npos) 
                splitCommand.push_back(command.substr(0, endIndex - 1));
            else splitCommand.push_back(command);
            return splitCommand;
        }
        std::string commandPiece = command.substr(0, startIndex);
        command = command.substr(startIndex + 1, command.length() - startIndex);

        splitCommand.push_back(commandPiece);

    }

    return splitCommand;
}

bool ResolveCommand(std::string command) {

    if (std::regex_match(command, std::regex("-startListening +[0-9]*"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');
        closesocket(listeningSocket);
        PrintToConsole("Starting to listen on port: " + commandPieces.size() - 1);
        std::thread networkThread(mainNetworking, commandPieces.at(commandPieces.size() - 1), true);
        networkThread.detach();

        return true;
    }
    
    if (std::regex_match(command, std::regex("-connect +[0-9]+"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');
        PrintToConsole(std::string("Trying to connect to port: ") + commandPieces.at( commandPieces.size() - 1));
        std::thread networkThread(mainNetworking, commandPieces.at(commandPieces.size() - 1), false);
        networkThread.detach();

        return true;
    }

    if (std::regex_match(command, std::regex("-disconnectIncoming"))) {

        Disconnect(incomingSocket);
        PrintToConsole("Disconnected incoming socket.");
        return true;

    }

    if (std::regex_match(command, std::regex("-disconnectOutgoing"))) {

        Disconnect(outgoingSocket);
        PrintToConsole("Disconnected outgoing socket.");
        return true;

    }

    if (std::regex_match(command, std::regex("-stopListening"))) {

        closesocket(listeningSocket);
        PrintToConsole("Stopped Listening.");
        return true;

    }

    if (std::regex_match(command, std::regex("-.*"))) {
        PrintToConsole("Unknown command.");
    }

    return false;
}

int main(int argc, char* argv[])
{

    WSADATA wsaData;
    if (int err = WSAStartup(DEFAULT_WINSOCK_VERSION, &wsaData)) {
        std::cerr << "WSAStartup failed with error: " << err << std::endl;
        exit(1);
    }

    while (!exitSignalReceived) {
        std::string userInput;
        std::getline(std::cin, userInput);
        if (exitSignalReceived) {
            PrintToConsole("Program terminated.");
            break;
        }

        if (userInput.length() == 0) {
            continue;
        }

        if (ResolveCommand(userInput)) {
            continue;
        }
        
        if (incomingSocket != NULL) SendMessageTo(incomingSocket, userInput.c_str(), userInput.size());
        if (outgoingSocket != NULL) SendMessageTo(outgoingSocket, userInput.c_str(), userInput.size());

    }

    WSACleanup();
    return 0;
}
