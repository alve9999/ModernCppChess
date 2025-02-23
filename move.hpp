#pragma once
#include "board.hpp"
#include "constants.hpp"
#include <cstdint>
#include <utility>


inline std::ostream &operator<<(std::ostream &os, const Move &m) {
    os << "From: " << static_cast<int>(m.from)
       << " To: " << static_cast<int>(m.to)
       << " Piece: " << static_cast<int>(m.piece)
       << " Special: " << static_cast<int>(m.special);
    return os;
}
typedef struct Move Move;

inline int algToCoord(const std::string &square) {
    int column = square[0] - 'a';
    int row = 8 - (square[1] - '0');
    return (8 * row + column);
}

template <bool IsWhite>
_fast Move algebraicToMove(std::string &alg, const Board &brd, const BoardState & state) {
    uint8_t from = algToCoord(alg.substr(0, 2));
    uint8_t to = algToCoord(alg.substr(2, 4));
    uint8_t special = 0;
    BoardPiece type;
    bool capture;
    if (ISSET(brd.Occ, to)) {
        special += 4;
    }

    if constexpr (IsWhite) {
        if (ISSET(brd.WPawn, from)) {
            type = BoardPiece::Pawn;
            if (to == state.ep) {
                special = 3;
            }
            if (abs(from - to) == 16) {
                special = 1;
            }
            if (to % 8 == 7) {
                std::string s = alg.substr(4, 5);
                if (s == "n") {
                    special += 8;
                }
                if (s == "b") {
                    special += 9;
                }
                if (s == "r") {
                    special += 10;
                }
                if (s == "q") {
                    special += 11;
                }
            }
        }
        if (ISSET(brd.WKnight, from)) {
            type = BoardPiece::Knight;
        }
        if (ISSET(brd.WBishop, from)) {
            type = BoardPiece::Bishop;
        }
        if (ISSET(brd.WRook, from)) {
            type = BoardPiece::Rook;
        }
        if (ISSET(brd.WQueen, from)) {
            type = BoardPiece::Queen;
        }
        if (ISSET(brd.WKing, from)) {
            if (abs(from - to) == 2) {
                special = 2;
            }
            type = BoardPiece::King;
        }
    } else {
        if (ISSET(brd.BPawn, from)) {
            type = BoardPiece::Pawn;
            if (to == state.ep) {
                special = 3;
            }
            if (abs(from - to) == 16) {
                special = 1;
            }
            if (to % 8 == 0) {
                std::string s = alg.substr(4, 5);
                if (s == "n") {
                    special += 8;
                }
                if (s == "b") {
                    special += 9;
                }
                if (s == "r") {
                    special += 10;
                }
                if (s == "q") {
                    special += 11;
                }
            }
        }
        if (ISSET(brd.BKnight, from)) {
            type = BoardPiece::Knight;
        }
        if (ISSET(brd.BBishop, from)) {
            type = BoardPiece::Bishop;
        }
        if (ISSET(brd.BRook, from)) {
            type = BoardPiece::Rook;
        }
        if (ISSET(brd.BQueen, from)) {
            type = BoardPiece::Queen;
        }
        if (ISSET(brd.BKing, from)) {
            if (abs(from - to) == 2) {
                special = 2;
            }
            type = BoardPiece::King;
        }
    }

    return Move{from, to, type, special};
}

static int c = 0;

