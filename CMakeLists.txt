cmake_minimum_required(VERSION 3.16)
project(PO_lab4)

set(CMAKE_CXX_STANDARD 20)

# Сервер
add_executable(server server.cpp)
target_link_libraries(server Ws2_32)

# Перший клієнт
add_executable(client client.cpp)
target_link_libraries(client Ws2_32)

# Другий клієнт
add_executable(client2 client2.cpp)
target_link_libraries(client2 Ws2_32)
