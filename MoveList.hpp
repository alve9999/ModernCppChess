#pragma once
#include "board.hpp"
#include "move.hpp"
#include <vector>

class MoveList {
public:
    std::vector<Move> Moves;

    MoveList(){
        Moves.reserve(50);
    }

    void addMove(const uint8_t from,const uint8_t to, const BoardPiece piece,const uint8_t special) noexcept {
        Moves.emplace_back(from,to,piece,special);
    }


};

