#include "ClientNetwork.h"

void StartClient(SOCKET socket, addrinfo& addressInfo) {

    if (connect(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen) == SOCKET_ERROR) {
        closesocket(socket);
        socket = INVALID_SOCKET;
    }

    PrintToConsole("Connected to socket.");
    connectedSocket = socket;

    ReceiveMessage(socket);
}