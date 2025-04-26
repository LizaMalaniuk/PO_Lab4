#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 12345;

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

    closesocket(clientSock);
    closesocket(serverSock);
    WSACleanup();
    return 0;
}
