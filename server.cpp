#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 12345;

int recvAll(SOCKET sock, char* buffer, int length) {
    int received = 0;
    while (received < length) {
        int r = recv(sock, buffer + received, length - received, 0);
        if (r <= 0) return -1;
        received += r;
    }
    return received;
}

int recvMessage(SOCKET sock, vector<char>& data) {
    uint32_t length_be;
    if (recvAll(sock, reinterpret_cast<char*>(&length_be), 4) != 4) return -1;

    uint32_t length = ntohl(length_be);
    if (length > 1e7) return -1;

    data.resize(length);
    return recvAll(sock, data.data(), length);
}

bool sendMessage(SOCKET sock, const vector<char>& data) {
    uint32_t len = htonl(data.size());
    if (send(sock, reinterpret_cast<const char*>(&len), 4, 0) != 4) return false;
    return send(sock, data.data(), data.size(), 0) == data.size();
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSock, SOMAXCONN);

    cout << "[SERVER] Listening on port " << PORT << "...\n";

    SOCKET clientSock = accept(serverSock, nullptr, nullptr);
    cout << "[SERVER] Client accepted.\n";

    vector<char> message{'H', 'e', 'l', 'l', 'o'};
    sendMessage(clientSock, message);


    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}