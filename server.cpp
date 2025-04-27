#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 12345;

enum MsgType : uint32_t {
    MT_CONFIG = 1,
    MT_MATRIX_A,
    MT_MATRIX_B,
    MT_START,
    MT_RESULT
};

struct Config {
    uint32_t size;
    uint32_t threads;
    int32_t k;
};

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

void computeMatrix(const vector<vector<int>>& A, const vector<vector<int>>& B, vector<vector<int>>& C, int k) {
    for (size_t i = 0; i < A.size(); ++i)
        for (size_t j = 0; j < A[i].size(); ++j)
            C[i][j] = A[i][j] + k * B[i][j];
}

void handleClient(SOCKET clientSock) {
    Config config;
    vector<vector<int>> A, B, C;

    while (true) {
        vector<char> message;
        if (recvMessage(clientSock, message) <= 0) break;

        uint32_t msgType;
        memcpy(&msgType, message.data(), 4);
        msgType = ntohl(msgType);

        if (msgType == MT_CONFIG) {
            memcpy(&config.size, &message[4], 4);
            memcpy(&config.threads, &message[8], 4);
            memcpy(&config.k, &message[12], 4);

            config.size = ntohl(config.size);
            config.threads = ntohl(config.threads);
            config.k = ntohl(config.k);

            A.resize(config.size, vector<int>(config.size));
            B.resize(config.size, vector<int>(config.size));
            C.resize(config.size, vector<int>(config.size));

            cout << "[SERVER] CONFIG received. Size=" << config.size
                 << " Threads=" << config.threads << " k=" << config.k << "\n";

        } else if (msgType == MT_MATRIX_A || msgType == MT_MATRIX_B) {
            auto& target = (msgType == MT_MATRIX_A) ? A : B;
            for (uint32_t i = 0; i < config.size; ++i)
                for (uint32_t j = 0; j < config.size; ++j) {
                    uint32_t val;
                    memcpy(&val, &message[4 + (i * config.size + j) * 4], 4);
                    target[i][j] = ntohl(val);
                }

            cout << "[SERVER] Matrix " << (msgType == MT_MATRIX_A ? "A" : "B") << " received.\n";

        } else if (msgType == MT_START) {
            computeMatrix(A, B, C, config.k);
            cout << "[SERVER] Computation completed.\n";

        } else if (msgType == MT_RESULT) {
            vector<char> response(4 + config.size * config.size * 4);
            uint32_t mt = htonl(MT_RESULT);
            memcpy(response.data(), &mt, 4);

            for (uint32_t i = 0; i < config.size; ++i)
                for (uint32_t j = 0; j < config.size; ++j) {
                    uint32_t val = htonl(C[i][j]);
                    memcpy(&response[4 + (i * config.size + j) * 4], &val, 4);
                }

            sendMessage(clientSock, response);
            cout << "[SERVER] Result sent.\n";
        }
    }

    closesocket(clientSock);
    cout << "[SERVER] Client disconnected.\n";
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
    if (clientSock != INVALID_SOCKET) {
        handleClient(clientSock);
    }

    closesocket(serverSock);
    WSACleanup();
    return 0;
}