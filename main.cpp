#include "Network.h"


void PrintToConsole(std::string message) {
    //Tu treba kritični odsječak
    std::cout << message << std::endl;
}


void createServerPort(std::string command) {

    std::string port = command.substr(command.find(" "), command.size());
    std::thread networkThread(mainNetworking, NULL, port, true);

    networkThread.detach();
}

bool ResolveCommand(std::string command) {

    PrintToConsole(command);

    if (std::regex_match(command, std::regex("-startListening +[0-9]*"))) {

        createServerPort(command);

        return true;
    }
    if (std::regex_match(command, std::regex("-connect +[0-9]+"))) {



        return true;
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
