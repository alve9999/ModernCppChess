#pragma once

#include "board.hpp"
#include <utility>
#include <vector>
/*
using MakeMoveFunc = void (*)(const Board &, int, int);

struct callback {
    MakeMoveFunc move;
    int from;
    int to;
};

class MoveList {
  public:
    std::vector<callback> Moves;

    MoveList() { Moves.reserve(100); }

    void addMove(const MakeMoveFunc &move, int from, int to, int& count)
noexcept {

        Moves.emplace_back(move, from, to);
    }
};*/
