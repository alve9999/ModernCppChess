#pragma once
#include "MoveList.hpp"
#include "board.hpp"
#include "constants.hpp"
#include "move.hpp"
#include <cstdint>
#include <sys/types.h>
#include <utility>

inline std::ostream &operator<<(std::ostream &os, const Move &m) {
    os << "From: " << static_cast<int>(m.from)
       << " To: " << static_cast<int>(m.to)
       << " Piece: " << static_cast<int>(m.piece)
       << " Special: " << static_cast<int>(m.special);
    return os;
}

inline int algToCoord(const std::string &square) {
    int column = square[0] - 'a';
    int row = 8 - (square[1] - '0');
    return (8 * row + column);
}

template <bool IsWhite>
_fast Move algebraicToMove(std::string &alg, const Board &brd,
                           const BoardState state) {
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
            /*if (to == state.ep) {
                special = 3;
            }*/
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
            /*if (to == state.ep) {
                special = 3;
            }*/
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

static long long c = 0;

using MakeMoveFunc = void (*)(const Board &, int, int);

struct Callback {
    MakeMoveFunc move;
    uint8_t from;
    uint8_t to;
};

template <class BoardState status, int depth>
constexpr inline void perft(const Board &brd, int ep) noexcept {
    if constexpr (depth == 0) {
        c++;
        return;
    } else {
        Callback ml[100];
        int count = 0;
        genMoves<status, depth>(brd, ep, ml, count);
        for (int i = 0; i < count; i++) {
            ml[i].move(brd, ml[i].from, ml[i].to);
        }
        if constexpr (depth == 5) {
            std::cout << c << std::endl;
        }
    }
}

template <class BoardState status, int depth>
constexpr inline void pawnMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void pawnDoubleMove(const Board &brd, int from,
                                     int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    if constexpr (status.IsWhite) {
        perft<status.pawn(), depth - 1>(newBoard, to - 8);
    } else {
        perft<status.pawn(), depth - 1>(newBoard, to + 8);
    }
}

template <class BoardState status, int depth>
constexpr inline void pawnCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Pawn, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void promote(const Board &brd, int from, int to) noexcept {
    Board newBoard1 = brd.promote<BoardPiece::Queen, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard1, 0);
    Board newBoard2 = brd.promote<BoardPiece::Rook, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard2, 0);
    Board newBoard3 =
        brd.promote<BoardPiece::Bishop, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard3, 0);
    Board newBoard4 =
        brd.promote<BoardPiece::Knight, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard4, 0);
}

template <class BoardState status, int depth>
constexpr inline void promoteCapture(const Board &brd, int from,
                                     int to) noexcept {
    Board newBoard1 =
        brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard1, 0);
    Board newBoard2 =
        brd.promoteCapture<BoardPiece::Rook, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard2, 0);
    Board newBoard3 =
        brd.promoteCapture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard3, 0);
    Board newBoard4 =
        brd.promoteCapture<BoardPiece::Knight, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard4, 0);
}

template <class BoardState status, int depth>
constexpr inline void EP(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.pawn(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void knightMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Knight, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void knightCapture(const Board &brd, int from,
                                    int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Knight, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void bishopMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Bishop, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void bishopCapture(const Board &brd, int from, int to) {
    Board newBoard = brd.capture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void rookMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Rook, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                perft<status.rookMoveLeft(), depth - 1>(newBoard, 0);
                return;
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                perft<status.rookMoveRight(), depth - 1>(newBoard, 0);
                return;
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                perft<status.rookMoveLeft(), depth - 1>(newBoard, 0);
                return;
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                perft<status.rookMoveRight(), depth - 1>(newBoard, 0);
                return;
            }
        }
    }
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void rookCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Rook, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                perft<status.rookMoveLeft(), depth - 1>(newBoard, 0);
                return;
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                perft<status.rookMoveRight(), depth - 1>(newBoard, 0);
                return;
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                perft<status.rookMoveLeft(), depth - 1>(newBoard, 0);
                return;
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                perft<status.rookMoveRight(), depth - 1>(newBoard, 0);
                return;
            }
        }
    }
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void queenMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Queen, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void queenCapture(const Board &brd, int from,
                                   int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.normal(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void kingMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::King, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.king(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void kingCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::King, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    perft<status.king(), depth - 1>(newBoard, 0);
}

template <class BoardState status, int depth>
constexpr inline void leftCastel(const Board &brd, int from, int to) noexcept {
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, true,
                                    false, false, false>();
        perft<status.normal(), depth - 1>(newBoard, 0);
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, true, false>();
        perft<status.normal(), depth - 1>(newBoard, 0);
    }
}

template <class BoardState status, int depth>
constexpr inline void rightCastel(const Board &brd, int from, int to) noexcept {
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    true, false, false>();
        perft<status.normal(), depth - 1>(newBoard, 0);
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, false, true>();
        perft<status.normal(), depth - 1>(newBoard, 0);
    }
}
