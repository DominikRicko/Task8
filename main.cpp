#include "config.h"


void PrintToConsole(std::string message) {
    //Tu treba kritični odsječak
    std::cout << message << std::endl;
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
        command = command.substr(startIndex, command.length() - startIndex);

        splitCommand.push_back(commandPiece);

    }

    return splitCommand;
}

bool ResolveCommand(std::string command) {

    if (std::regex_match(command, std::regex("-startListening +[0-9]*"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');

        std::thread networkThread(mainNetworking, commandPieces.at(commandPieces.size() - 1), true);
        networkThread.detach();

        return true;
    }
    
    if (std::regex_match(command, std::regex("-connect +[0-9]+"))) {

        std::vector<std::string> commandPieces = commandSplit(command, ' ');

        std::thread networkThread(mainNetworking, commandPieces.at(commandPieces.size() - 1), false);
        networkThread.detach();

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
            PrintToConsole("Program terminated.");
            break;
        }

        if (userInput.length() == 0) {
            continue;
        }

        if (ResolveCommand(userInput)) {
            continue;
        }
        
        if (listeningSocket != NULL) SendMessageTo(listeningSocket, userInput.c_str(), userInput.size());
        if (connectedSocket != NULL) SendMessageTo(connectedSocket, userInput.c_str(), userInput.size());

    }

    WSACleanup();
    return 0;
}
