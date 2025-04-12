#pragma once
#include "MoveList.hpp"
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "move.hpp"
#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <utility>




inline int algToCoord(const std::string &square) {
    int file = (square[0] - 'a');
    int rank = square[1] - '1';
    return rank * 8 + file;
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
    std::cout << "icnoming move:"<< std::endl;
    std::cout << "alg: " << alg << std::endl;
    std::cout << (int)from << "  " << (int)to << std::endl;
    std::cout << "isWhite: " << IsWhite << std::endl;
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
                } else if (s == "b") {
                    special += 9; // promote to bishop
                } else if (s == "r") {
                    special += 10; // promote to rook
                } else if (s == "q" || s.empty()) {
                    special += 11; // promote to queen
                }
            }
        } else if (ISSET(brd.WKnight, from)) {
            type = BoardPiece::Knight;
        } else if (ISSET(brd.WBishop, from)) {
            type = BoardPiece::Bishop;
        } else if (ISSET(brd.WRook, from)) {
            type = BoardPiece::Rook;
            if (from == 0) {        // a1 - queenside rook
                special = 4;        // track left rook move
            } else if (from == 7) { // h1 - kingside rook
                special = 5;        // track right rook move
            }
        } else if (ISSET(brd.WQueen, from)) {
            type = BoardPiece::Queen;
        } else if (ISSET(brd.WKing, from)) {
            type = BoardPiece::King;
            if (abs(from - to) == 2) {
                special = 2; // castling
            }
        }
    } else { // Black pieces
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
                } else if (s == "b") {
                    special += 9; // promote to bishop
                } else if (s == "r") {
                    special += 10; // promote to rook
                } else if (s == "q" || s.empty()) {
                    special += 11; // promote to queen (default)
                }
            }
        } else if (ISSET(brd.BKnight, from)) {
            type = BoardPiece::Knight;
        } else if (ISSET(brd.BBishop, from)) {
            type = BoardPiece::Bishop;
        } else if (ISSET(brd.BRook, from)) {
            type = BoardPiece::Rook;
            if (from == 56) {        // a8 - queenside rook
                special = 4;         // track left rook move
            } else if (from == 63) { // h8 - kingside rook
                special = 5;         // track right rook move
            }
        } else if (ISSET(brd.BQueen, from)) {
            type = BoardPiece::Queen;
        } else if (ISSET(brd.BKing, from)) {
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
                    return brd.template castle<BoardPiece::King, true, false,
                                               true, false, false>();
                };
            } else { // Queen-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, true, true,
                                               false, false, false>();
                };
            }
        } else {
            if (to > from) { // King-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, false, false,
                                               false, false, true>();
                };
            } else { // Queen-side castling
                boardCallback = [&brd]() {
                    return brd.template castle<BoardPiece::King, false, false,
                                               false, true, false>();
                };
            }
        }
    } else if (special == 3) { // En Passant
        boardCallback = [&brd, from, to]() {
            if constexpr (IsWhite) {
                return brd.template EP<BoardPiece::Pawn, true, false, false,
                                       false, false>(from, to);
            } else {
                return brd.template EP<BoardPiece::Pawn, false, false, false,
                                       false, false>(from, to);
            }
        };
    } else if (special >= 8 && special <= 11) { // Promotion
        BoardPiece promotePiece;
        switch (special) {
        case 8:
            promotePiece = BoardPiece::Knight;
            break;
        case 9:
            promotePiece = BoardPiece::Bishop;
            break;
        case 10:
            promotePiece = BoardPiece::Rook;
            break;
        default:
            promotePiece = BoardPiece::Queen;
            break;
        }

        if (capture) {
            switch (promotePiece) {
            case BoardPiece::Knight:
                boardCallback = [&brd, from, to]() {
                    return brd
                        .template promoteCapture<BoardPiece::Knight, IsWhite,
                                                 false, false, false, false>(
                            from, to);
                };
                break;
            case BoardPiece::Bishop:
                boardCallback = [&brd, from, to]() {
                    return brd
                        .template promoteCapture<BoardPiece::Bishop, IsWhite,
                                                 false, false, false, false>(
                            from, to);
                };
                break;
            case BoardPiece::Rook:
                boardCallback = [&brd, from, to]() {
                    return brd.template promoteCapture<
                        BoardPiece::Rook, IsWhite, false, false, false, false>(
                        from, to);
                };
                break;
            case BoardPiece::Queen:
            default:
                boardCallback = [&brd, from, to]() {
                    return brd.template promoteCapture<
                        BoardPiece::Queen, IsWhite, false, false, false, false>(
                        from, to);
                };
                break;
            }
        } else {
            switch (promotePiece) {
            case BoardPiece::Knight:
                boardCallback = [&brd, from, to]() {
                    return brd.template promote<BoardPiece::Knight, IsWhite,
                                                false, false, false, false>(
                        from, to);
                };
                break;
            case BoardPiece::Bishop:
                boardCallback = [&brd, from, to]() {
                    return brd.template promote<BoardPiece::Bishop, IsWhite,
                                                false, false, false, false>(
                        from, to);
                };
                break;
            case BoardPiece::Rook:
                boardCallback = [&brd, from, to]() {
                    return brd.template promote<BoardPiece::Rook, IsWhite,
                                                false, false, false, false>(
                        from, to);
                };
                break;
            case BoardPiece::Queen:
            default:
                boardCallback = [&brd, from, to]() {
                    return brd.template promote<BoardPiece::Queen, IsWhite,
                                                false, false, false, false>(
                        from, to);
                };
                break;
            }
        }
    } else if (capture) {
        boardCallback = [&brd, type, from, to]() {
            switch (type) {
            case BoardPiece::Pawn:
                return brd.template capture<BoardPiece::Pawn, IsWhite, false,
                                            false, false, false>(from, to);
            case BoardPiece::Knight:
                return brd.template capture<BoardPiece::Knight, IsWhite, false,
                                            false, false, false>(from, to);
            case BoardPiece::Bishop:
                return brd.template capture<BoardPiece::Bishop, IsWhite, false,
                                            false, false, false>(from, to);
            case BoardPiece::Rook:
                return brd.template capture<BoardPiece::Rook, IsWhite, false,
                                            false, false, false>(from, to);
            case BoardPiece::Queen:
                return brd.template capture<BoardPiece::Queen, IsWhite, false,
                                            false, false, false>(from, to);
            case BoardPiece::King:
                return brd.template capture<BoardPiece::King, IsWhite, false,
                                            false, false, false>(from, to);
            default:
                return brd;
            }
        };
    } else {
        boardCallback = [&brd, type, from, to]() {
            switch (type) {
            case BoardPiece::Pawn:
                return brd.template move<BoardPiece::Pawn, IsWhite, false,
                                         false, false, false>(from, to);
            case BoardPiece::Knight:
                return brd.template move<BoardPiece::Knight, IsWhite, false,
                                         false, false, false>(from, to);
            case BoardPiece::Bishop:
                return brd.template move<BoardPiece::Bishop, IsWhite, false,
                                         false, false, false>(from, to);
            case BoardPiece::Rook:
                return brd.template move<BoardPiece::Rook, IsWhite, false,
                                         false, false, false>(from, to);
            case BoardPiece::Queen:
                return brd.template move<BoardPiece::Queen, IsWhite, false,
                                         false, false, false>(from, to);
            case BoardPiece::King:
                return brd.template move<BoardPiece::King, IsWhite, false,
                                         false, false, false>(from, to);
            default:
                return brd;
            }
        };
    }
    if (special == 1) {
        stateCallback = [&state]() { return state.pawn(); };
    } else if (special == 2) {
        stateCallback = [&state]() { return state.king(); };
    } else if (special == 4) {
        stateCallback = [&state]() { return state.rookMoveLeft(); };
    } else if (special == 5) {
        stateCallback = [&state]() { return state.rookMoveRight(); };
    } else if (type == BoardPiece::King) {
        stateCallback = [&state]() { return state.king(); };
    } else {
        stateCallback = [&state]() { return state.normal(); };
    }

    return MoveCallbacks{boardCallback, stateCallback};
}

static long long c = 0;

struct MoveResult {
    Board board;
    BoardState state;
};

using SearchMoveFunc = int (*)(const Board &, int, int,int,int,int);
using MakeMoveFunc = MoveResult (*)(const Board &, int, int);

struct Callback {
    MakeMoveFunc makeMove;
    SearchMoveFunc move;
    uint8_t from;
    uint8_t to;
};

/*
constexpr const int gd = 8;
constexpr const bool count_succ = false;
template <class BoardState status, int depth>
constexpr inline int perft(const Board &brd, int ep) noexcept {
    if constexpr (depth == 0) {
        volatile int a = eval<status.IsWhite>(brd);
        c++;
        return;
    } else {
        Callback ml[100];
        int count = 0;
        genMoves<status, depth,1>(brd, ep, ml, count);
        int cur = c;
        for (int i = 0; i < count; i++) {
            if (depth == gd && count_succ) {
                std::cout << (int)ml[i].from << " " << (int)ml[i].to
                          << std::endl;
            }
            ml[i].move(brd, ml[i].from, ml[i].to);
        }
        if constexpr (depth == gd - 1 && count_succ) {
            std::cout << c - cur << std::endl;
        }
        if constexpr (depth == gd) {
            std::cout << c << std::endl;
        }
    }
    return;
}*/

template <class BoardState status>
inline MoveResult makePawnMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makePawnDoubleMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.pawn()};
}

template <class BoardState status>
inline MoveResult makePawnCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Pawn, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makePromote(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.promote<BoardPiece::Queen, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makePromoteCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                         status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeEP(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.EP<BoardPiece::Pawn, status.IsWhite, status.WLC,
                            status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.pawn()};
}

template <class BoardState status>
inline MoveResult makeKnightMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Knight, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeKnightCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Knight, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeBishopMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Bishop, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeBishopCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeRookMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Rook, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                BoardState newState = status.rookMoveLeft();
                return {newBoard, newState};
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                BoardState newState = status.rookMoveRight();
                return {newBoard, newState};
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                BoardState newState = status.rookMoveLeft();
                return {newBoard, newState};
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                BoardState newState = status.rookMoveRight();
                return {newBoard, newState};
            }
        }
    }

    BoardState newState = status.normal();
    return {newBoard, newState};
}

template <class BoardState status>
inline MoveResult makeRookCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Rook, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                BoardState newState = status.rookMoveLeft();
                return {newBoard, newState};
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                BoardState newState = status.rookMoveRight();
                return {newBoard, newState};
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                BoardState newState = status.rookMoveLeft();
                return {newBoard, newState};
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                BoardState newState = status.rookMoveRight();
                return {newBoard, newState};
            }
        }
    }
    
    BoardState newState = status.normal();
    return {newBoard, newState};
}

template <class BoardState status>
inline MoveResult makeQueenMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Queen, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeQueenCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makeKingMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::King, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.king()};
}

template <class BoardState status>
inline MoveResult makeKingCapture(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.capture<BoardPiece::King, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.king()};
}

template <class BoardState status>
inline MoveResult makeLeftCastel(const Board &brd, int from, int to) noexcept {
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, true,
                                false, false, false>();
        return {newBoard, status.king()};
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                false, true, false>();
        return {newBoard, status.king()};
    }
}

template <class BoardState status>
inline MoveResult makeRightCastel(const Board &brd, int from, int to) noexcept {
    Board newBoard;
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                true, false, false>();
        return {newBoard, status.king()};
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                false, false, true>();
        return {newBoard, status.king()};
    }
}

//for perft testing
//#define searchFunc perft
//for minimax testing
#define searchFunc minimax

template <bool IsWhite>
constexpr inline int getCapturePiece(const Board &brd, int to) noexcept {
    int capturedPiece;
    if constexpr (IsWhite) {
        if (brd.BPawn & (1ULL << to)) capturedPiece = 0;
        else if (brd.BKnight & (1ULL << to)) capturedPiece = 1;
        else if (brd.BBishop & (1ULL << to)) capturedPiece = 2;
        else if (brd.BRook & (1ULL << to)) capturedPiece = 3;
        else if (brd.BQueen & (1ULL << to)) capturedPiece = 4;
    } else {
        if (brd.WPawn & (1ULL << to)) capturedPiece = 0;
        else if (brd.WKnight & (1ULL << to)) capturedPiece = 1;
        else if (brd.WBishop & (1ULL << to)) capturedPiece = 2;
        else if (brd.WRook & (1ULL << to)) capturedPiece = 3;
        else if (brd.WQueen & (1ULL << to)) capturedPiece = 4;
    }
    return capturedPiece;
}

template <bool IsWhite, BoardPiece Piece>
constexpr inline int calculateMoveScoreDelta(int from, int to) {
    if constexpr (IsWhite) {
        return mg_table[static_cast<int>(Piece)][true][to] - mg_table[static_cast<int>(Piece)][true][from];
    } else {
        return mg_table[static_cast<int>(Piece)][false][to] - mg_table[static_cast<int>(Piece)][false][from];
    }
}

template <bool IsWhite, BoardPiece AttackerPiece>
constexpr inline int calculateCaptureScoreDelta(int victimPiece,int from, int to) {
    int delta = 0;
    
    if constexpr (IsWhite) {
        delta += mg_table[static_cast<int>(AttackerPiece)][true][to] - mg_table[static_cast<int>(AttackerPiece)][true][from];
    } else {
        delta += mg_table[static_cast<int>(AttackerPiece)][false][to] - mg_table[static_cast<int>(AttackerPiece)][false][from];
    }
    
    if constexpr (IsWhite) {
        delta += mg_value[victimPiece];
        delta += mg_table[victimPiece][false][to];
    } else {
        delta += mg_value[victimPiece];
        delta += mg_table[victimPiece][true][to];
    }
    
    return delta;
}

template <class BoardState status, int depth>
constexpr inline int pawnMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int pawnDoubleMove(const Board &brd, int from,
                                     int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);
    if constexpr (status.IsWhite) {
        return searchFunc<status.pawn(), depth - 1>(newBoard, to - 8, alpha, beta, score+delta);
    } else {
        return searchFunc<status.pawn(), depth - 1>(newBoard, to + 8, alpha, beta, score+delta);
    }
}

template <class BoardState status, int depth>
constexpr inline int pawnCapture(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::Pawn, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Pawn>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int promote(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    int delta = 0;
    Board newBoard1 = brd.promote<BoardPiece::Queen, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    int val = searchFunc<status.normal(), depth - 1>(newBoard1, 0, alpha, beta, score+delta);
    return val; 
    Board newBoard2 = brd.promote<BoardPiece::Rook, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    int val2 = searchFunc<status.normal(), depth - 1>(newBoard2, 0, alpha, beta, score+delta);
    
    Board newBoard3 =
        brd.promote<BoardPiece::Bishop, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    int val3 = searchFunc<status.normal(), depth - 1>(newBoard3, 0, alpha, beta, score+delta);
    
    Board newBoard4 =
        brd.promote<BoardPiece::Knight, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    int val4 = searchFunc<status.normal(), depth - 1>(newBoard4, 0, alpha, beta, score+delta);
    
}

template <class BoardState status, int depth>
constexpr inline int promoteCapture(const Board &brd, int from,
                                     int to, int alpha, int beta, int score) noexcept {
    int delta = 0;
    Board newBoard1 =
        brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val = searchFunc<status.normal(), depth - 1>(newBoard1, 0, alpha, beta, score+delta);
    return val;
    Board newBoard2 =
        brd.promoteCapture<BoardPiece::Rook, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val2 = searchFunc<status.normal(), depth - 1>(newBoard2, 0, alpha, beta, score+delta);
    
    Board newBoard3 =
        brd.promoteCapture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val3 = searchFunc<status.normal(), depth - 1>(newBoard3, 0, alpha, beta, score+delta);
    
    Board newBoard4 =
        brd.promoteCapture<BoardPiece::Knight, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val4 = searchFunc<status.normal(), depth - 1>(newBoard4, 0, alpha, beta, score+delta);
    
}

template <class BoardState status, int depth>
constexpr inline int EP(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.EP<BoardPiece::Pawn, status.IsWhite, status.WLC,
                            status.WRC, status.BLC, status.BRC>(from, to);
    int delta = 0;
    return searchFunc<status.pawn(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int knightMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Knight, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Knight>(from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int knightCapture(const Board &brd, int from,
                                    int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::Knight, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Knight>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int bishopMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Bishop, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);    
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Bishop>(from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int bishopCapture(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Bishop>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int rookMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Rook, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Rook>(from, to);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                return searchFunc<status.rookMoveLeft(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                return searchFunc<status.rookMoveRight(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                return searchFunc<status.rookMoveLeft(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                return searchFunc<status.rookMoveRight(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
    }
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int rookCapture(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::Rook, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Rook>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                return searchFunc<status.rookMoveLeft(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                return searchFunc<status.rookMoveRight(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                return searchFunc<status.rookMoveLeft(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                return searchFunc<status.rookMoveRight(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
            }
        }
    }
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int queenMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::Queen, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::Queen>(from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int queenCapture(const Board &brd, int from,
                                   int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Queen>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int kingMove(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.move<BoardPiece::King, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateMoveScoreDelta<status.IsWhite, BoardPiece::King>(from, to);
    return searchFunc<status.king(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int kingCapture(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    Board newBoard = brd.capture<BoardPiece::King, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::King>(getCapturePiece<status.IsWhite>(brd,to),from, to);
    return searchFunc<status.king(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
}

template <class BoardState status, int depth>
constexpr inline int leftCastel(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    int delta = 0;
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, true,
                                    false, false, false>();
        return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, true, false>();
        return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
    }
}

template <class BoardState status, int depth>
constexpr inline int rightCastel(const Board &brd, int from, int to, int alpha, int beta, int score) noexcept {
    int delta = 0;
    if constexpr (status.IsWhite) {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    true, false, false>();
        return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
    } else {
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, false, true>();
        return searchFunc<status.normal(), depth - 1>(newBoard, 0, alpha, beta, score+delta);
    }
}

