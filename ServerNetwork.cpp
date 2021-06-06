#include "Network.h"
#include "ServerNetwork.h"

void StartServer(SOCKET socket, addrinfo& addressInfo) {

    int socketResult = bind(socket, addressInfo.ai_addr, (int)addressInfo.ai_addrlen);
    if (socketResult == SOCKET_ERROR) {
        std::cerr << "Bind failed with error: " << WSAGetLastError() << std::endl;
        exit(4);
    }

    ListenForIncomingConnections(socket);

}

void ListenForIncomingConnections(SOCKET hostSocket) {

    u_long iMode = 0;

    if (listen(hostSocket, 1) == SOCKET_ERROR) {
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

        if (connectedSockets.size() > 1) {
            std::cerr << "New connection rejected." << std::endl;
            closesocket(client);
            continue;
        }

        if (int result = ioctlsocket(client, FIONBIO, &iMode) != NO_ERROR) {
            std::cerr << "Unable to set socket as blocking: " << result << std::endl;
            exit(11);
        }

        connectedSockets.push_back(client);
        std::thread clientThread(ReceiveMessage, client);
        clientThread.detach();

    }

}