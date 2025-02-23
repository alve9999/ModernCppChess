cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing -O3 -std=c++23 -fconstexpr-ops-limit=1000000000 -ggdb")
project(chess2000)

set(CMAKE_CXX_STANDARD 23)

add_executable(chess2000 main.cpp board.hpp check.hpp sliding.cpp pawns.hpp constants.hpp movegen.hpp move.hpp MoveList.hpp uci.cpp)


