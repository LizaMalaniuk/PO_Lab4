#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <cstdint>
#include <random>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

const int PORT = 12345;
const char* SERVER_IP = "127.0.0.1";

enum MsgType : uint32_t {
    MT_CONFIG = 1,
    MT_MATRIX_A,
    MT_MATRIX_B,
    MT_START,
    MT_RESULT
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

vector<vector<int>> generateMatrix(int size) {
    vector<vector<int>> mat(size, vector<int>(size));
    mt19937 gen(static_cast<unsigned int>(time(0)) + GetCurrentProcessId());
    uniform_int_distribution<int> distrib(0, 9);

    for (auto& row : mat)
        for (auto& val : row)
            val = distrib(gen);

    return mat;
}

void sendConfig(SOCKET sock, uint32_t size, uint32_t threads, int32_t k) {
    vector<char> msg(16);
    uint32_t type = htonl(MT_CONFIG);
    memcpy(msg.data(), &type, 4);
    uint32_t netSize = htonl(size);
    uint32_t netThreads = htonl(threads);
    int32_t netK = htonl(k);
    memcpy(msg.data() + 4, &netSize, 4);
    memcpy(msg.data() + 8, &netThreads, 4);
    memcpy(msg.data() + 12, &netK, 4);
    sendMessage(sock, msg);
}

void sendMatrix(SOCKET sock, const vector<vector<int>>& mat, MsgType type) {
    uint32_t size = mat.size();
    vector<char> msg(4 + size * size * 4);
    uint32_t t = htonl(type);
    memcpy(msg.data(), &t, 4);

    for (uint32_t i = 0; i < size; ++i)
        for (uint32_t j = 0; j < size; ++j) {
            uint32_t val = htonl(mat[i][j]);
            memcpy(&msg[4 + (i * size + j) * 4], &val, 4);
        }

    sendMessage(sock, msg);
}

void sendSimpleCommand(SOCKET sock, MsgType type) {
    vector<char> msg(4);
    uint32_t t = htonl(type);
    memcpy(msg.data(), &t, 4);
    sendMessage(sock, msg);
}

void printMatrix(const vector<vector<int>>& mat) {
    for (const auto& row : mat) {
        for (int v : row) cout << v << " ";
        cout << "\n";
    }
}

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "Connection failed\n";
        return 1;
    }

    uint32_t size = 4;
    uint32_t threads = 2;
    int32_t k = 3;

    auto A = generateMatrix(size);
    auto B = generateMatrix(size);

    cout << "[CLIENT] Matrix A:\n";
    printMatrix(A);
    cout << "[CLIENT] Matrix B:\n";
    printMatrix(B);

    sendConfig(sock, size, threads, k);
    sendMatrix(sock, A, MT_MATRIX_A);
    sendMatrix(sock, B, MT_MATRIX_B);
    sendSimpleCommand(sock, MT_START);
    sendSimpleCommand(sock, MT_RESULT);

    vector<char> response;
    if (recvMessage(sock, response) > 0) {
        if (response.size() >= 4) {
            uint32_t type;
            memcpy(&type, response.data(), 4);
            type = ntohl(type);
            if (type == MT_RESULT) {
                vector<vector<int>> C(size, vector<int>(size));
                for (uint32_t i = 0; i < size; ++i)
                    for (uint32_t j = 0; j < size; ++j) {
                        uint32_t val;
                        memcpy(&val, &response[4 + (i * size + j) * 4], 4);
                        C[i][j] = ntohl(val);
                    }

                cout << "[CLIENT] Result matrix (C = A + k * B):\n";
                printMatrix(C);
            }
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}