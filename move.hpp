#pragma once
#include "board.hpp"
#include "constants.hpp"
#include <cstdint>
#include <utility>
struct Move {
    uint8_t from;
    uint8_t to;
    BoardPiece piece;
    uint8_t special;
    std::string toAlgebraic() const {
        char file_from = 'h' - (this->from % 8);
        char rank_from = '1' + (this->from / 8);
        char file_to = 'h' - (this->to % 8);
        char rank_to = '1' + (this->to / 8);
        return std::string{file_from, rank_from, file_to, rank_to};
    }
};

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
_fast Move algebraicToMove(std::string &alg, const Board &brd) {
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
            if (to == brd.EPSquare) {
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
            if (to == brd.EPSquare) {
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

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast Board normalMove(const Board &brd, Move move, int EPSquare) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    const uint64_t mov = 1ULL << move.from | 1ULL << move.to;
    if constexpr (IsWhite) {
        const BoardState defState =
            BoardState(false, false, WLC, WRC, BLC, BRC);
        if (BoardPiece::Pawn == move.piece) {
            if (EPSquare != -1) {
                return Board(bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq,
                             wk, EPSquare,
                             BoardState(false, true, WLC, WRC, BLC, BRC));
            }
            return Board(bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Knight == move.piece) {
            return Board(bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Bishop == move.piece) {
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Rook == move.piece) {
            if constexpr (WLC) {
                if (move.from == 0) {
                    return Board(
                        bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk,
                        EPSquare,
                        BoardState(false, false, false, WRC, BLC, BRC));
                }
            }
            if constexpr (WRC) {
                if (move.from == 7) {
                    return Board(
                        bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk,
                        EPSquare,
                        BoardState(false, false, WLC, false, BLC, BRC));
                }
            }
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Queen == move.piece) {
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::King == move.piece) {
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov,
                         EPSquare,
                         BoardState(false, false, false, false, BLC, BRC));
        }
    } else {
        BoardState defState = BoardState(true, false, WLC, WRC, BLC, BRC);
        if (BoardPiece::Pawn == move.piece) {
            if (EPSquare != -1) {
                return Board(bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq,
                             wk, EPSquare,
                             BoardState(true, true, WLC, WRC, BLC, BRC));
            }
            return Board(bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Knight == move.piece) {
            return Board(bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Bishop == move.piece) {
            return Board(bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Rook == move.piece) {
            if constexpr (BLC) {
                if (move.from == 63) {
                    return Board(bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr,
                                 wq, wk, EPSquare,
                                 BoardState(true, false, WLC, WRC, false, BRC));
                }
            }
            if constexpr (BRC) {
                if (move.from == 56) {
                    return Board(bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr,
                                 wq, wk, EPSquare,
                                 BoardState(true, false, WLC, WRC, BLC, false));
                }
            }
            return Board(bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::Queen == move.piece) {
            return Board(bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk,
                         EPSquare, defState);
        }
        if (BoardPiece::King == move.piece) {
            return Board(bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk,
                         EPSquare,
                         BoardState(true, false, WLC, WRC, false, false));
        }
    }
    return Board();
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast Board capture(const Board &brd, Move move) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    const uint64_t mov = 1ULL << move.from | 1ULL << move.to;
    const uint64_t remove = ~(1ULL << move.to);
    if constexpr (IsWhite) {
        const BoardState defState =
            BoardState(false, false, WLC, WRC, BLC, BRC);
        if (BoardPiece::Pawn == move.piece) {
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp ^ mov, wn, wb, wr, wq, wk, 0,
                         defState);
        }
        if (BoardPiece::Knight == move.piece) {
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp, wn ^ mov, wb, wr, wq, wk, 0,
                         defState);
        }
        if (BoardPiece::Bishop == move.piece) {
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp, wn, wb ^ mov, wr, wq, wk, 0,
                         defState);
        }
        if (BoardPiece::Rook == move.piece) {
            if constexpr (WLC) {
                if (move.from == 0) {
                    return Board(
                        bp & remove, bn & remove, bb & remove, br & remove,
                        bq & remove, bk, wp, wn, wb, wr ^ mov, wq, wk, 0,
                        BoardState(false, false, false, WRC, BLC, BRC));
                }
            }
            if constexpr (WRC) {
                if (move.from == 7) {
                    return Board(
                        bp & remove, bn & remove, bb & remove, br & remove,
                        bq & remove, bk, wp, wn, wb, wr ^ mov, wq, wk, 0,
                        BoardState(false, false, WLC, false, BLC, BRC));
                }
            }
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp, wn, wb, wr ^ mov, wq, wk, 0,
                         defState);
        }
        if (BoardPiece::Queen == move.piece) {
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp, wn, wb, wr, wq ^ mov, wk, 0,
                         defState);
        }
        if (BoardPiece::King == move.piece) {
            return Board(bp & remove, bn & remove, bb & remove, br & remove,
                         bq & remove, bk, wp, wn, wb, wr, wq, wk ^ mov, 0,
                         BoardState(false, false, false, false, BLC, BRC));
        }
    } else {
        BoardState defState = BoardState(true, false, WLC, WRC, BLC, BRC);
        if (BoardPiece::Pawn == move.piece) {
            return Board(bp ^ mov, bn, bb, br, bq, bk, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         defState);
        }
        if (BoardPiece::Knight == move.piece) {
            return Board(bp, bn ^ mov, bb, br, bq, bk, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         defState);
        }
        if (BoardPiece::Bishop == move.piece) {
            return Board(bp, bn, bb ^ mov, br, bq, bk, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         defState);
        }
        if (BoardPiece::Rook == move.piece) {
            if constexpr (BLC) {
                if (move.from == 56) {
                    return Board(bp, bn, bb, br ^ mov, bq, bk, wp & remove,
                                 wn & remove, wb & remove, wr & remove,
                                 wq & remove, wk, 0,
                                 BoardState(true, false, WLC, WRC, false, BRC));
                }
            }
            if constexpr (BRC) {
                if (move.from == 63) {
                    return Board(bp, bn, bb, br ^ mov, bq, bk, wp & remove,
                                 wn & remove, wb & remove, wr & remove,
                                 wq & remove, wk, 0,
                                 BoardState(true, false, WLC, WRC, BLC, false));
                }
            }
            return Board(bp, bn, bb, br ^ mov, bq, bk, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         defState);
        }
        if (BoardPiece::Queen == move.piece) {
            return Board(bp, bn, bb, br, bq ^ mov, bk, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         defState);
        }
        if (BoardPiece::King == move.piece) {
            return Board(bp, bn, bb, br, bq, bk ^ mov, wp & remove, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         BoardState(true, false, WLC, WRC, false, false));
        }
    }
    return Board();
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast Board castles(const Board &brd, Move move) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    if constexpr (IsWhite) {
        if constexpr (WLC) {
            if (move.to == 2) {
                return Board(bp, bn, bb, br, bq, bk, wp, wn, wb,
                             wr ^ WRookLChange, wq, wk ^ WKingLChange, 0,
                             BoardState(false, false, false, false, BLC, BRC));
            }
        }
        if constexpr (WRC) {
            if (move.to == 6) {
                return Board(bp, bn, bb, br, bq, bk, wp, wn, wb,
                             wr ^ WRookRChange, wq, wk ^ WKingRChange, 0,
                             BoardState(false, false, false, false, BLC, BRC));
            }
        }
    } else {
        if constexpr (BLC) {
            if (move.to == 58) {
                return Board(bp, bn, bb, br ^ BRookLChange, bq,
                             bk ^ BKingLChange, wp, wn, wb, wr, wq, wk, 0,
                             BoardState(true, false, WLC, WRC, false, false));
            }
        }
        if constexpr (BRC) {
            if (move.to == 62) {
                return Board(bp, bn, bb, br ^ BRookRChange, bq,
                             bk ^ BKingRChange, wp, wn, wb, wr, wq, wk, 0,
                             BoardState(true, false, WLC, WRC, false, false));
            }
        }
    }
    return Board();
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC,
          BoardPiece piece>
constexpr Board promote(const Board &brd, Move move) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    const uint64_t from = 1ULL << move.from;
    const uint64_t to = 1ULL << move.to;
    if constexpr (BoardPiece::Queen == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr, wq ^ to,
                         wk, 0, BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb, br, bq ^ to, bk, wp, wn, wb, wr, wq,
                         wk, 0, BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Rook == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr ^ to, wq,
                         wk, 0, BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb, br ^ to, bq, bk, wp, wn, wb, wr, wq,
                         wk, 0, BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Bishop == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb ^ to, wr, wq,
                         wk, 0, BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb ^ to, br, bq, bk, wp, wn, wb, wr, wq,
                         wk, 0, BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Knight == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn ^ to, wb, wr, wq,
                         wk, 0, BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn ^ to, bb, br, bq, bk, wp, wn, wb, wr, wq,
                         wk, 0, BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    return Board();
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC,
          BoardPiece piece>
_fast Board promoteCapture(const Board &brd, Move move) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    const uint64_t from = 1ULL << move.from;
    const uint64_t to = 1ULL << move.to;
    const uint64_t remove = ~(1ULL << move.to);

    if constexpr (BoardPiece::Queen == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn & remove, bb & remove, br & remove, bq & remove,
                         bk, wp ^ from, wn, wb, wr, wq ^ to, wk, 0,
                         BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb, br, bq ^ to, bk, wp, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Rook == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn & remove, bb & remove, br & remove, bq & remove,
                         bk, wp ^ from, wn, wb, wr ^ to, wq, wk, 0,
                         BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb, br ^ to, bq, bk, wp, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Bishop == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn & remove, bb & remove, br & remove, bq & remove,
                         bk, wp ^ from, wn, wb ^ to, wr, wq, wk, 0,
                         BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn, bb ^ to, br, bq, bk, wp, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    if constexpr (BoardPiece::Knight == piece) {
        if constexpr (IsWhite) {
            return Board(bp, bn & remove, bb & remove, br & remove, bq & remove,
                         bk, wp ^ from, wn ^ to, wb, wr, wq, wk, 0,
                         BoardState(false, false, WLC, WRC, BLC, BRC));
        } else {
            return Board(bp ^ from, bn ^ to, bb, br, bq, bk, wp, wn & remove,
                         wb & remove, wr & remove, wq & remove, wk, 0,
                         BoardState(true, false, WLC, WRC, BLC, BRC));
        }
    }
    return Board();
}
template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast Board EPMove(const Board &brd, Move move) noexcept {
    const uint64_t bp = brd.BPawn;
    const uint64_t bn = brd.BKnight;
    const uint64_t bb = brd.BBishop;
    const uint64_t br = brd.BRook;
    const uint64_t bq = brd.BQueen;
    const uint64_t bk = brd.BKing;
    const uint64_t wp = brd.WPawn;
    const uint64_t wn = brd.WKnight;
    const uint64_t wb = brd.WBishop;
    const uint64_t wr = brd.WRook;
    const uint64_t wq = brd.WQueen;
    const uint64_t wk = brd.WKing;
    const uint64_t from = 1ULL << move.from;
    const uint64_t to = 1ULL << move.to;
    const uint64_t mov = from | to;
    if constexpr (IsWhite) {
        const uint64_t remove = ~(1ULL << (move.to - 8));
        return Board(bp & remove, bn & remove, bb & remove, br & remove,
                     bq & remove, bk & remove, wp ^ mov, wn, wb, wr, wq, wk, 0,
                     BoardState(false, false, WLC, WRC, BLC, BRC));
    } else {
        const uint64_t remove = ~(1ULL << (move.to + 8));
        return Board(bp ^ mov, bn, bb, br, bq, bk, wp & remove, wn & remove,
                     wb & remove, wr & remove, wq & remove, wk & remove, 0,
                     BoardState(true, false, WLC, WRC, BLC, BRC));
    }
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast Board makeMove(const Board &brd, Move move) noexcept {
    if (move.special == 0) {
        return normalMove<IsWhite, EP, WLC, WRC, BLC, BRC>(brd, move, -1);
    }
    if (move.special == 1) {
        constexpr int offset = IsWhite ? -8 : 8;
        return normalMove<IsWhite, EP, WLC, WRC, BLC, BRC>(brd, move,
                                                           move.to - offset);
    }
    if (move.special == 2) {
        return castles<IsWhite, EP, WLC, WRC, BLC, BRC>(brd, move);
    }
    if (move.special == 3) {
        return EPMove<IsWhite, EP, WLC, WRC, BLC, BRC>(brd, move);
    }
    if (move.special == 4) {
        return capture<IsWhite, EP, WLC, WRC, BLC, BRC>(brd, move);
    }
    if (move.special == 8) {
        return promote<IsWhite, EP, WLC, WRC, BLC, BRC, BoardPiece::Knight>(
            brd, move);
    }
    if (move.special == 9) {
        return promote<IsWhite, EP, WLC, WRC, BLC, BRC, BoardPiece::Bishop>(
            brd, move);
    }
    if (move.special == 10) {
        return promote<IsWhite, EP, WLC, WRC, BLC, BRC, BoardPiece::Rook>(brd,
                                                                          move);
    }
    if (move.special == 11) {
        return promote<IsWhite, EP, WLC, WRC, BLC, BRC, BoardPiece::Queen>(
            brd, move);
    }
    if (move.special == 12) {
        return promoteCapture<IsWhite, EP, WLC, WRC, BLC, BRC,
                              BoardPiece::Knight>(brd, move);
    }
    if (move.special == 13) {
        return promoteCapture<IsWhite, EP, WLC, WRC, BLC, BRC,
                              BoardPiece::Bishop>(brd, move);
    }
    if (move.special == 14) {
        return promoteCapture<IsWhite, EP, WLC, WRC, BLC, BRC,
                              BoardPiece::Rook>(brd, move);
    }
    if (move.special == 15) {
        return promoteCapture<IsWhite, EP, WLC, WRC, BLC, BRC,
                              BoardPiece::Queen>(brd, move);
    }
    return Board();
}
