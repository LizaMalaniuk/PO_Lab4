cmake_minimum_required(VERSION 3.16)
project(PO_lab4)

set(CMAKE_CXX_STANDARD 20)

add_executable(server server.cpp)

add_executable(client client.cpp)

target_link_libraries(server ws2_32)
target_link_libraries(client ws2_32)


