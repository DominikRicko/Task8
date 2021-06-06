#include "ServerNetwork.h"

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

	listeningSocket = client;

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

	ListenForIncomingConnections(socket);

}
