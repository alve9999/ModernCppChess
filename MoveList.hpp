#pragma once
#include <functional>

#include "board.hpp"
#include "move.hpp"
#include <utility>
#include <vector>

using StateMoveFunc = BoardState (BoardState::*)(int) const noexcept;
using MakeMoveFunc = Board (Board::*)(const Move&) const noexcept;

struct moveCallback {
    Move move;
    MakeMoveFunc moveFunc;
    StateMoveFunc state;
    int ep;
};

class MoveList {
public:
    std::vector<moveCallback> Moves;

    MoveList() { Moves.reserve(50); }


    void addMove(const MakeMoveFunc& move, StateMoveFunc state,
                 const uint8_t from, const uint8_t to, const BoardPiece pieceType, const uint8_t special) noexcept {
        Moves.emplace_back((Move){from, to, pieceType, special}, move, state, 0);
    }

    void addMove(const MakeMoveFunc& move, StateMoveFunc state,
                 const uint8_t from, const uint8_t to, const BoardPiece pieceType, const uint8_t special, int ep) noexcept {
        Moves.emplace_back((Move){from, to, pieceType, special}, move, state, ep);
    }
};

