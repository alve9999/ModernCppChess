cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-narrowing -std=c++20 -ggdb -gdwarf-4 -mavx2 -mavx512f -mavx512bw -march=native")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -ggdb")
project(chess2000)
set(CMAKE_CXX_STANDARD 23)



add_executable(chess2000 main.cpp board.hpp check.hpp sliding.cpp pawns.hpp constants.hpp movegen.hpp move.hpp uci.cpp eval.hpp ai.hpp hash.cpp SEE.cpp minimax_info.hpp parameter.cpp nnue.cpp)


target_precompile_headers(chess2000 PRIVATE pch.h)
