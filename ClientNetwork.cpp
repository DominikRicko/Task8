#include "Network.h"
#include "ClientNetwork.h"

void StartClient(SOCKET socket, addrinfo& addressInfo) {

    if (connect(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen) == SOCKET_ERROR) {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    std::string initialMessage = "N";

    std::cout << "Connected to server." << std::endl;
    connectedSockets.push_back(socket);
    SendMessageTo(socket, initialMessage.c_str(), sizeof(char) * initialMessage.size());

    ReceiveMessage(socket);
}