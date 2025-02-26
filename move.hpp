#pragma once
#include "MoveList.hpp"
#include "board.hpp"
#include "constants.hpp"
#include "move.hpp"
#include <cstdint>
#include <sys/types.h>
#include <utility>
#include <functional>

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

struct MoveCallbacks {
    std::function<Board()> boardCallback;
    std::function<BoardState()> stateCallback;
};

template <bool IsWhite>
_fast MoveCallbacks algebraicToMove(std::string &alg, const Board &brd,
                         const BoardState state, int ep) {
    uint8_t from = algToCoord(alg.substr(0, 2));
    uint8_t to = algToCoord(alg.substr(2, 4));
    BoardPiece type = BoardPiece::Pawn;
    bool capture = ISSET(brd.Occ, to);
    int special = 0;

    if constexpr (IsWhite) {
        if (ISSET(brd.WPawn, from)) {
            type = BoardPiece::Pawn;
            if (to == ep) {
                special = 3; // en passant
            }
            if (abs(from - to) == 16) {
                special = 1; // pawn double move
            }
            if (to / 8 == 7) { 
                std::string s = alg.length() > 4 ? alg.substr(4, 5) : "";
                if (s == "n") {
                    special += 8; // promote to knight
                }
                else if (s == "b") {
                    special += 9; // promote to bishop
                }
                else if (s == "r") {
                    special += 10; // promote to rook
                }
                else if (s == "q" || s.empty()) {
                    special += 11; // promote to queen 
                }
            }
        }
        else if (ISSET(brd.WKnight, from)) {
            type = BoardPiece::Knight;
        }
        else if (ISSET(brd.WBishop, from)) {
            type = BoardPiece::Bishop;
        }
        else if (ISSET(brd.WRook, from)) {
            type = BoardPiece::Rook;
            if (from == 0) { // a1 - queenside rook
                special = 4; // track left rook move
            }
            else if (from == 7) { // h1 - kingside rook
                special = 5; // track right rook move
            }
        }
        else if (ISSET(brd.WQueen, from)) {
            type = BoardPiece::Queen;
        }
        else if (ISSET(brd.WKing, from)) {
            type = BoardPiece::King;
            if (abs(from - to) == 2) {
                special = 2; // castling
            }
        }
    } 
    else { // Black pieces
        if (ISSET(brd.BPawn, from)) {
            type = BoardPiece::Pawn;
            if (to == ep) {
                special = 3; // en passant
            }
            if (abs(from - to) == 16) {
                special = 1; // pawn double move
            }
            if (to / 8 == 0) { // promotion (reaching the 1st rank)
                std::string s = alg.length() > 4 ? alg.substr(4, 5) : "";
                if (s == "n") {
                    special += 8; // promote to knight
                }
                else if (s == "b") {
                    special += 9; // promote to bishop
                }
                else if (s == "r") {
                    special += 10; // promote to rook
                }
                else if (s == "q" || s.empty()) {
                    special += 11; // promote to queen (default)
                }
            }
        }
        else if (ISSET(brd.BKnight, from)) {
            type = BoardPiece::Knight;
        }
        else if (ISSET(brd.BBishop, from)) {
            type = BoardPiece::Bishop;
        }
        else if (ISSET(brd.BRook, from)) {
            type = BoardPiece::Rook;
            if (from == 56) { // a8 - queenside rook
                special = 4; // track left rook move
            }
            else if (from == 63) { // h8 - kingside rook
                special = 5; // track right rook move
            }
        }
        else if (ISSET(brd.BQueen, from)) {
            type = BoardPiece::Queen;
        }
        else if (ISSET(brd.BKing, from)) {
            type = BoardPiece::King;
            if (abs(from - to) == 2) {
                special = 2; // castling
            }
        }
    }
    
    std::function<Board()> boardCallback;
    std::function<BoardState()> stateCallback;

    if (special == 2) { // Castling
        if constexpr (IsWhite) {
            if (to > from) { // King-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, true, false, true, false, false>();
                };
            } else { // Queen-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, true, true, false, false, false>();
                };
            }
        } else {
            if (to > from) { // King-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, false, false, false, false, true>();
                };
            } else { // Queen-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, false, false, false, true, false>();
                };
            }
        }
    } else if (special == 3) { // En Passant
        boardCallback = [&brd, from, to]() {
            if constexpr (IsWhite) {
                return brd.template EP<BoardPiece::Pawn, true, false, false, false, false>(from, to);
            } else {
                return brd.template EP<BoardPiece::Pawn, false, false, false, false, false>(from, to);
            }
        };
    }
    else if (special >= 8 && special <= 11) { // Promotion
        BoardPiece promotePiece;
        switch (special) {
            case 8: promotePiece = BoardPiece::Knight; break;
            case 9: promotePiece = BoardPiece::Bishop; break;
            case 10: promotePiece = BoardPiece::Rook; break;
            default: promotePiece = BoardPiece::Queen; break;
        }

        if (capture) {
            boardCallback = [&brd, promotePiece, from, to]() {
                return brd.template promoteCapture<promotePiece,IsWhite,false,false,false,false>(from, to);
            };
        } 
        else {
            boardCallback = [&brd, promotePiece, from, to]() {
                return brd.template promote<promotePiece,IsWhite,false,false,false,false>(from, to);
            };
        }
    }
    else if (capture) {
        boardCallback = [&brd, type, from, to]() {
            switch (type) {
                case BoardPiece::Pawn:
                    return brd.template capture<BoardPiece::Pawn, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Knight:
                    return brd.template capture<BoardPiece::Knight, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Bishop:
                    return brd.template capture<BoardPiece::Bishop, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Rook:
                    return brd.template capture<BoardPiece::Rook, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Queen:
                    return brd.template capture<BoardPiece::Queen, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::King:
                    return brd.template capture<BoardPiece::King, IsWhite, false, false, false, false>(from, to);
                default:
                    return brd; 
            }
        };
    } 
    else { 
        boardCallback = [&brd, type, from, to]() {
            switch (type) {
                case BoardPiece::Pawn:
                    return brd.template move<BoardPiece::Pawn, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Knight:
                    return brd.template move<BoardPiece::Knight, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Bishop:
                    return brd.template move<BoardPiece::Bishop, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Rook:
                    return brd.template move<BoardPiece::Rook, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::Queen:
                    return brd.template move<BoardPiece::Queen, IsWhite, false, false, false, false>(from, to);
                case BoardPiece::King:
                    return brd.template move<BoardPiece::King, IsWhite, false, false, false, false>(from, to);
                default:
                    return brd;
            }
        };
    }
    if (special == 1) { 
        stateCallback = [&state]() {
            return state.pawn();
        };
    } else if (special == 2) { 
        stateCallback = [&state]() {
            return state.king();
        };
    } else if (special == 4) { 
        stateCallback = [&state]() {
            return state.rookMoveLeft();
        };
    } else if (special == 5) { 
        stateCallback = [&state]() {
            return state.rookMoveRight();
        };
    } else if (type == BoardPiece::King) { 
        stateCallback = [&state]() {
            return state.king();
        };
    } else {
        stateCallback = [&state]() {
            return state.normal();
        };
    }

    return MoveCallbacks{boardCallback, stateCallback};
}

static long long c = 0;

using MakeMoveFunc = void (*)(const Board &, int, int);

struct Callback {
    MakeMoveFunc move;
    uint8_t from;
    uint8_t to;
};
constexpr const int gd = 7;
constexpr const bool count_succ = false;
template <class BoardState status, int depth>
constexpr inline void perft(const Board &brd, int ep) noexcept {
    if constexpr (depth == 0) {
        c++;
        return;
    } else {
        Callback ml[100];
        int count = 0;
        genMoves<status, depth>(brd, ep, ml, count);
        int cur = c;
        for (int i = 0; i < count; i++) {
            if(depth == gd && count_succ) {
                std::cout << (int)ml[i].from << " " << (int)ml[i].to << std::endl;
            }
            ml[i].move(brd, ml[i].from, ml[i].to);
        }
        if constexpr (depth == gd-1 && count_succ) {
            std::cout << c-cur << std::endl;
        }
        if constexpr (depth == gd) {
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
    Board newBoard = brd.EP<BoardPiece::Pawn, status.IsWhite, status.WLC,
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
