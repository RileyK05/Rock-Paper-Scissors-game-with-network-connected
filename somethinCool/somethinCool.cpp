#pragma comment(lib, "ws2_32.lib")

#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <string_view>

const int totUsr = 2;
const int bufferSize = 4096;

enum Choice {
    Rock,
    Paper,
    Scissors
};

const char* choices[] = { "rock", "paper", "scissors" };

void sendToClient(SOCKET clientSocket, const char* message) {
    send(clientSocket, message, strlen(message) + 1, 0);
}

void handleGame(SOCKET client1, SOCKET client2) {
    while (true) {
        char buffer1[bufferSize], buffer2[bufferSize];
        int bytesRec1, bytesRec2;

        bytesRec1 = recv(client1, buffer1, bufferSize, 0);
        bytesRec2 = recv(client2, buffer2, bufferSize, 0);

        if (bytesRec1 <= 0 || bytesRec2 <= 0) {
            std::cerr << "Client disconnected; game over\n";
            closesocket(client1);
            closesocket(client2);
            return;
        }

        buffer1[bytesRec1] = '\0';
        buffer2[bytesRec2] = '\0';

        if (std::string_view(buffer1) == "CNL" || std::string_view(buffer2) == "CNL") {
            std::cout << "Game terminated by a client.\n";
            closesocket(client1);
            closesocket(client2);
            return;
        }

        Choice choice1, choice2;

        auto getChoiceEnum = [](const char* choiceStr) {
            if (std::string_view(choiceStr) == "rock" || std::string_view(choiceStr) == "Rock") {
                return Rock;
            }
            else if (std::string_view(choiceStr) == "paper" || std::string_view(choiceStr) == "Paper") {
                return Paper;
            }
            else if (std::string_view(choiceStr) == "scissors" || std::string_view(choiceStr) == "Scissors") {
                return Scissors;
            }
            else {
                return static_cast<Choice>(-1);
            }
            };

        choice1 = getChoiceEnum(buffer1);
        choice2 = getChoiceEnum(buffer2);

        int result;
        if (choice1 == choice2) {
            result = 0;
        }
        else if ((choice1 == Rock && choice2 == Scissors) ||
            (choice1 == Paper && choice2 == Rock) ||
            (choice1 == Scissors && choice2 == Paper)) {
            result = 1;
        }
        else {
            result = 2;
        }

        std::string resultMsg;
        if (result == 0) {
            resultMsg = "Tie";
        }
        else if (result == 1) {
            resultMsg = "Player 1 Wins!";
        }
        else if (result == 2) {
            resultMsg = "Player 2 Wins!";
        }

        sendToClient(client1, resultMsg.c_str());
        sendToClient(client2, resultMsg.c_str());
    }
}

int main() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock failed to initialize. Error: " << WSAGetLastError() << std::endl;
        return -1;
    }

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCKET) {
        std::cerr << "Invalid socket. Error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Error binding socket. Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return -1;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Error listening. Error: " << WSAGetLastError() << std::endl;
        closesocket(serverSock);
        WSACleanup();
        return -1;
    }

    std::cout << "Listening on 8080\n";

    SOCKET clientSock[totUsr];
    std::fill(std::begin(clientSock), std::end(clientSock), INVALID_SOCKET);

    while (true) {
        SOCKET newClientSocket = accept(serverSock, NULL, NULL);
        if (newClientSocket == INVALID_SOCKET) {
            std::cerr << "Error accepting connection: " << WSAGetLastError() << std::endl;
            closesocket(serverSock);
            WSACleanup();
            return -1;
        }

        int clientIndex = -1;
        for (int i = 0; i < totUsr; ++i) {
            if (clientSock[i] == INVALID_SOCKET) {
                clientIndex = i;
                break;
            }
        }

        std::cout << "client index: " << clientIndex << std::endl;

        if (clientIndex == -1) {
            std::cerr << "Maximum number of clients reached. Connection rejected." << std::endl;
            closesocket(newClientSocket);
            continue;
        }
        else {
            clientSock[clientIndex] = newClientSocket;
            std::cout << "New client connected. Slot: " << clientIndex << std::endl;
        }

        if (clientSock[0] != INVALID_SOCKET && clientSock[1] != INVALID_SOCKET) {
            std::thread(handleGame, clientSock[0], clientSock[1]).detach();
        }
    }

    return 0;
}

