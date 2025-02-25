#pragma once
#include <functional>

#include "board.hpp"
#include <utility>
#include <vector>

using MakeMoveFunc = void (*)(const Board&,int,int);

struct callback{
    MakeMoveFunc move;
    int from;
    int to;
};


class MoveList {
public:
    std::vector<callback> Moves;

    MoveList() { Moves.reserve(50); }


    void addMove(MakeMoveFunc move,int from,int to) noexcept {
        Moves.emplace_back(move,from,to);
    }
};

