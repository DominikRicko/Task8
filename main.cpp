#include "config.h"

void mainNetworking(std::string ip, std::string port, bool isServer) {

    // Common intialization for both client and server

    addrinfo* result = GetAddressInfo(ip.c_str(), port.c_str(), isServer);

    SOCKET socket = CreateSocket(*result);

    if (isServer) StartServer(socket, *result);
    else StartClient(socket, *result);

    closesocket(socket);
    freeaddrinfo(result);
}

addrinfo* GetAddressInfo(const char* ip, const char* port, bool isServer) {
    addrinfo service;
    addrinfo* result = NULL;
    memset(&service, 0, sizeof(service));

    service.ai_family = AF_INET;
    service.ai_socktype = SOCK_STREAM;
    service.ai_protocol = IPPROTO_TCP;
    if (isServer) service.ai_flags = AI_PASSIVE;

    int addrResultCode = getaddrinfo(ip, port, &service, &result);
    if (addrResultCode != 0) {
        PrintToConsole(std::string("Failure at getaddrinfo: ") + std::to_string(addrResultCode));
        exit(3);
    }

    return result;
}

SOCKET CreateSocket(const addrinfo& availableInfo) {

    SOCKET newSocket = socket(availableInfo.ai_family, availableInfo.ai_socktype, availableInfo.ai_protocol);
    if (newSocket == INVALID_SOCKET) {
        PrintToConsole(std::string("Error occured: ") + std::to_string(WSAGetLastError()));
        exit(2);
    }

    return newSocket;
}

void ReceiveMessage(SOCKET client) {

    char buffer[MESSAGE_BUFFER_LENGTH];

    while (true) {

        memset(buffer, 0, sizeof(buffer));
        int receiveStatus = recv(client, buffer, MESSAGE_BUFFER_LENGTH, 0);
        if (receiveStatus == SOCKET_ERROR) {
            PrintToConsole(std::string("Error occured at receiving message: ") + std::to_string(WSAGetLastError()));
            closesocket(client);
            return;
        }
        else if (receiveStatus > 0) {
            PrintToConsole(string(buffer));
            if (client == incomingSocket) SendMessageTo(outgoingSocket, buffer, receiveStatus);
        }
        else if (receiveStatus == 0) {

            if (shutdown(client, SD_SEND) == SOCKET_ERROR) {
                PrintToConsole(std::string("Shutdown failed: ") + std::to_string(WSAGetLastError()));
                PrintToConsole(std::string("Closing the socket anyways."));
            }
            closesocket(client);
            if (client == incomingSocket) incomingSocket = INVALID_SOCKET;
            if (client == outgoingSocket) outgoingSocket = INVALID_SOCKET;
            PrintToConsole("Connection closed.");

            return;
        }
        else {
            PrintToConsole(std::string("Unknown scenario."));
            PrintToConsole(std::string("Receive status: " + receiveStatus));
        }

    }

}

void SendMessageTo(SOCKET client, const char* message, size_t length) {
    if (client == INVALID_SOCKET) return;
    if (length > MESSAGE_BUFFER_LENGTH) length = MESSAGE_BUFFER_LENGTH;
    if (send(client, message, length, 0) == SOCKET_ERROR) {
        PrintToConsole(std::string("Send failed: ") + std::to_string(WSAGetLastError()));
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
        PrintToConsole(std::string("Listen failed with error: ") + std::to_string(WSAGetLastError()));
        return;
    }
    PrintToConsole("Listening for connections.");

    while (true) {
        SOCKET client = accept(hostSocket, NULL, NULL);
        if (client == INVALID_SOCKET) {
            PrintToConsole(string("Accept failed: ") + to_string(WSAGetLastError()));
            closesocket(client);
            return;
        }
        if (incomingSocket != INVALID_SOCKET) {
            Disconnect(client);
            continue;
        }

        incomingSocket = client;

        if (int result = ioctlsocket(client, FIONBIO, &iMode) != NO_ERROR) {
            PrintToConsole(string("Unable to set socket as blocking: ") + to_string(result));
            return;
        }

        std::thread receive(ReceiveMessage, client);
        receive.detach();
    }
}

void StartServer(SOCKET socket, addrinfo& addressInfo) {

    int socketResult = bind(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen);
    if (socketResult == SOCKET_ERROR) {
        PrintToConsole(string("Bind failed with error: ") + to_string(WSAGetLastError()));
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

    if (std::regex_match(command, std::regex("-listen +[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+:[0-9]+"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');
        std::vector<std::string> commandPieces2 = commandSplit(commandPieces.at(commandPieces.size() - 1), ':');
        PrintToConsole(std::string("Starting to listen on address: ") + commandPieces.at(commandPieces.size() - 1));
        closesocket(listeningSocket);
        listeningSocket = INVALID_SOCKET;
        std::thread networkThread(mainNetworking, commandPieces2.at(0), commandPieces2.at(commandPieces2.size() - 1), true);
        networkThread.detach();

        return true;
    }
    
    if (std::regex_match(command, std::regex("-connect +[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+:[0-9]+"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');
        std::vector<std::string> commandPieces2 = commandSplit(commandPieces.at(commandPieces.size() - 1), ':');
        PrintToConsole(std::string("Trying to connect to application at: ") + commandPieces.at( commandPieces.size() - 1));
        if(outgoingSocket != INVALID_SOCKET) Disconnect(outgoingSocket);
        outgoingSocket = INVALID_SOCKET;
        std::thread networkThread(mainNetworking, commandPieces2.at(0), commandPieces2.at(commandPieces2.size() - 1), false);
        networkThread.detach();

        return true;
    }

    if (std::regex_match(command, std::regex("-disconnectIncoming"))) {

        Disconnect(incomingSocket);

        incomingSocket = INVALID_SOCKET;
        PrintToConsole("Disconnected incoming socket.");
        return true;

    }

    if (std::regex_match(command, std::regex("-disconnectOutgoing"))) {

        Disconnect(outgoingSocket);
      
        outgoingSocket = INVALID_SOCKET;
        PrintToConsole("Disconnected outgoing socket.");
        return true;

    }

    if (std::regex_match(command, std::regex("-stopListen"))) {

        closesocket(listeningSocket);
      
        listeningSocket = INVALID_SOCKET;
        PrintToConsole("Stopped Listening.");
        return true;

    }

    if (std::regex_match(command, std::regex("-help"))) {

        PrintToConsole("Commands: ");
        PrintToConsole("\t-listen [ip_address]:[portNumber] - start listening on given ip address and port.");
        PrintToConsole("\t-stopListen - stops listening.");
        PrintToConsole("\t-connect [ip_address]:[portNumber] - attempts connecting to given ip address and port.");
        PrintToConsole("\t-disconnectOutgoing - disconnects the socket obtained with -connect command.");
        PrintToConsole("\t-disconnectIncoming - disconnects the socket that has connected to listening socket.");
        PrintToConsole("\t-help - displays these messages.");
        PrintToConsole("");
        PrintToConsole("Typing anything without \'-\' is treated as a message to be sent to other clients.");

        return true;
    }

    if (std::regex_match(command, std::regex("-.*"))) {
        PrintToConsole(string("Unknown command."));
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

    PrintToConsole("Task 8: Chain Connection App v1.0");
    PrintToConsole("Made by Stjepan Mrganic & Dominik Ricko");
    PrintToConsole("Type -help for commands.");
    PrintToConsole("");

    while (true) {
        std::string userInput;
        std::getline(std::cin, userInput);

        if (userInput.length() == 0) {
            continue;
        }

        if (ResolveCommand(userInput)) {
            continue;
        }
        
        SendMessageTo(incomingSocket, userInput.c_str(), userInput.size());
        SendMessageTo(outgoingSocket, userInput.c_str(), userInput.size());

    }

    WSACleanup();
    return 0;
}
