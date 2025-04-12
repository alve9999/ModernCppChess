cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcf-protection=none -fno-stack-protector -fno-omit-frame-pointer -foptimize-sibling-calls -O3 -Wno-narrowing -std=c++23 -ftemplate-depth=100000 -fconstexpr-ops-limit=1000000000 -ggdb")
project(chess2000)
set(CMAKE_CXX_STANDARD 23)


add_executable(chess2000 main.cpp board.hpp check.hpp sliding.cpp pawns.hpp constants.hpp movegen.hpp move.hpp MoveList.hpp uci.cpp eval.hpp ai.hpp)


target_precompile_headers(chess2000 PRIVATE pch.h)
