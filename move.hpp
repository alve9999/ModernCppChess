#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "hash.hpp"
#include "nnue.h"
#include "move.hpp"
#include "minimax_info.hpp"
#include <cstdint>
#include <functional>
#include <sys/types.h>
#include <utility>

inline int algToCoord(const std::string &square) {
    int file = (square[0] - 'a');
    int rank = square[1] - '1';
    return rank * 8 + file;
}

inline std::string convertToUCI(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row);

    return std::string(1, file) + std::string(1, rank);
}

inline std::string convertMoveToUCI(const Board &brd, int from, int to) {
    std::string uci = convertToUCI(from) + convertToUCI(to);

    uint64_t fromMask = 1ULL << from;

    bool isWhitePawn = (brd.WPawn & fromMask) != 0;
    bool isBlackPawn = (brd.BPawn & fromMask) != 0;

    if (isWhitePawn && to / 8 == 7) {
        uci += 'q';
    } else if (isBlackPawn && to / 8 == 0) {
        uci += 'q';
    }

    return uci;
}

struct MoveCallbacks {
    std::function<Board()> boardCallback;
    std::function<BoardState()> stateCallback;
    bool irreversible;
    int ep;
};

template <bool IsWhite>
MoveCallbacks algebraicToMove(std::string &alg, const Board &brd,
                              const BoardState state) {
    uint8_t from = algToCoord(alg.substr(0, 2));
    uint8_t to = algToCoord(alg.substr(2, 4));
    /*std::cout << "icnoming move:" << std::endl;
    std::cout << "alg: " << alg << std::endl;
    std::cout << (int)from << "  " << (int)to << std::endl;
    std::cout << "isWhite: " << IsWhite << std::endl;*/
    BoardPiece type = BoardPiece::Pawn;
    bool capture = ISSET(brd.Occ, to);
    int special = 0;
    int ep = -1;
    bool irreversible = capture;
    if constexpr (IsWhite) {
        if (ISSET(brd.WPawn, from)) {
            irreversible = true;
            type = BoardPiece::Pawn;
            if ((abs(to - from) == 7 || abs(to - from) == 9) && !capture) {
                special = 3; // en passant
            }
            if (abs(from - to) == 16) {
                ep = to + (state.IsWhite ? -8 : 8);
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
                irreversible = true;
                special = 2; // castling
            }
        }
    } else { // Black pieces
        if (ISSET(brd.BPawn, from)) {
            irreversible = true;
            type = BoardPiece::Pawn;
            if ((abs(to - from) == 7 || abs(to - from) == 9) && !capture) {
                special = 3; // en passant
            }
            if (abs(from - to) == 16) {
                ep = to + (state.IsWhite ? -8 : 8);
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
                irreversible = true;
                special = 2; // castling
            }
        }
    }

    std::function<Board()> boardCallback;
    std::function<BoardState()> stateCallback;

    if (special == 2) { // castling
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
        stateCallback = [state]() { return state.pawn(); };
    } else if (special == 2) {
        stateCallback = [state]() { return state.king(); };
    } else if (special == 4) {
        stateCallback = [state]() { return state.rookMoveLeft(); };
    } else if (special == 5) {
        stateCallback = [state]() { return state.rookMoveRight(); };
    } else if (type == BoardPiece::King) {
        stateCallback = [state]() { return state.king(); };
    } else {
        stateCallback = [state]() { return state.normal(); };
    }
    return MoveCallbacks{boardCallback, stateCallback, irreversible, ep};
}

static long long c = 0;

struct MoveResult {
    Board board;
    BoardState state;
};

using SearchMoveFunc = int (*)(const Board &, move_info_t&);
using MakeMoveFunc = MoveResult (*)(const Board &, int, int);

struct Callback {
    MakeMoveFunc makeMove;
    SearchMoveFunc move;
    uint8_t from;
    uint8_t to;
    int value;
    bool capture;
    bool promotion;
};

/*
static long long counter = 0;
template <class BoardState status>
constexpr inline int perft(const Board &brd, int ep,int a, int b, int c,
uint64_t e, int depth,int hej) noexcept { if (depth == 0) { counter++; return 1;
    } else {
        Callback ml[100];
        int count = 0;
        genMoves<status,1,0>(brd, ep, ml, count);
        //printf("count %d\n", count);
        int cur = counter;
        for (int i = 0; i < count; i++) {
            ml[i].move(brd, ml[i].from, ml[i].to,a,b,c,0,depth - 1,hej);
            printf("from %d to %d\n", ml[i].from, ml[i].to);
        }
        printf("count %d\n", counter-cur);
    }
    return counter;
}*/

template <class BoardState status>
inline MoveResult makePawnMove(const Board &brd, int from, int to) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    return {newBoard, status.normal()};
}

template <class BoardState status>
inline MoveResult makePawnDoubleMove(const Board &brd, int from,
                                     int to) noexcept {
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
inline MoveResult makePromoteCapture(const Board &brd, int from,
                                     int to) noexcept {
    Board newBoard =
        brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
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
inline MoveResult makeKnightCapture(const Board &brd, int from,
                                    int to) noexcept {
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
inline MoveResult makeBishopCapture(const Board &brd, int from,
                                    int to) noexcept {
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
inline MoveResult makeQueenCapture(const Board &brd, int from,
                                   int to) noexcept {
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

// for perft testing
// #define searchFunc perft
// for minimax testing
#define searchFunc minimax
#include <cassert>
template <bool IsWhite>
constexpr inline int getCapturePiece(const Board &brd, int to) noexcept {
    int capturedPiece;
    if constexpr (IsWhite) {
        if (brd.BPawn & (1ULL << to))
            capturedPiece = 0;
        else if (brd.BKnight & (1ULL << to))
            capturedPiece = 1;
        else if (brd.BBishop & (1ULL << to))
            capturedPiece = 2;
        else if (brd.BRook & (1ULL << to))
            capturedPiece = 3;
        else if (brd.BQueen & (1ULL << to))
            capturedPiece = 4;
        else {
            assert(false);
        }
    } else {
        if (brd.WPawn & (1ULL << to))
            capturedPiece = 0;
        else if (brd.WKnight & (1ULL << to))
            capturedPiece = 1;
        else if (brd.WBishop & (1ULL << to))
            capturedPiece = 2;
        else if (brd.WRook & (1ULL << to))
            capturedPiece = 3;
        else if (brd.WQueen & (1ULL << to))
            capturedPiece = 4;
        else {

            assert(false);
        }
    }
    return capturedPiece;
}

template <bool IsWhite>
constexpr inline int getAttackerPiece(const Board &brd, int to) noexcept {
    int capturedPiece;
    if constexpr (IsWhite) {
        if (brd.BPawn & (1ULL << to))
            capturedPiece = 0;
        else if (brd.BKnight & (1ULL << to))
            capturedPiece = 1;
        else if (brd.BBishop & (1ULL << to))
            capturedPiece = 2;
        else if (brd.BRook & (1ULL << to))
            capturedPiece = 3;
        else if (brd.BQueen & (1ULL << to))
            capturedPiece = 4;
        else {
            capturedPiece = 5;
        }
    } else {
        if (brd.WPawn & (1ULL << to))
            capturedPiece = 0;
        else if (brd.WKnight & (1ULL << to))
            capturedPiece = 1;
        else if (brd.WBishop & (1ULL << to))
            capturedPiece = 2;
        else if (brd.WRook & (1ULL << to))
            capturedPiece = 3;
        else if (brd.WQueen & (1ULL << to))
            capturedPiece = 4;
        else {
            capturedPiece = 5;
        }
    }
    return capturedPiece;
}

template <bool IsWhite>
inline uint64_t update_hash_move(uint64_t key, int piece_index, int from,
                                 int to) {
    constexpr int color_offset = IsWhite ? 0 : 384;
    int piece_offset = piece_index * 64;

    key ^= random_key[from + color_offset + piece_offset];
    key ^= random_key[to + color_offset + piece_offset];

    return key;
}

template <bool IsWhite, bool IsCapturedWhite>
inline uint64_t update_hash_capture(uint64_t key, int moving_piece_index,
                                    int captured_piece_index, int from,
                                    int to) {
    constexpr int moving_color_offset = IsWhite ? 0 : 384;
    constexpr int captured_color_offset = IsCapturedWhite ? 0 : 384;

    int moving_piece_offset = moving_piece_index * 64;
    int captured_piece_offset = captured_piece_index * 64;

    key ^= random_key[from + moving_color_offset + moving_piece_offset];
    key ^= random_key[to + captured_color_offset + captured_piece_offset];
    key ^= random_key[to + moving_color_offset + moving_piece_offset];

    return key;
}

template <bool IsWhite>
inline uint64_t update_hash_promotion(uint64_t key, int from, int to,
                                      int new_piece_index) {
    constexpr int color_offset = IsWhite ? 0 : 384;

    key ^= random_key[from + color_offset];
    key ^= random_key[to + color_offset + new_piece_index * 64];

    return key;
}

template <bool IsWhite, bool IsCapturedWhite>
inline uint64_t
update_hash_promotion_capture(uint64_t key, int captured_piece_index, int from,
                              int to, int new_piece_index) {
    constexpr int moving_color_offset = IsWhite ? 0 : 384;
    constexpr int captured_color_offset = IsCapturedWhite ? 0 : 384;

    int captured_piece_offset = captured_piece_index * 64;

    key ^= random_key[from + moving_color_offset];
    key ^= random_key[to + captured_color_offset + captured_piece_offset];
    key ^= random_key[to + moving_color_offset + new_piece_index * 64];

    return key;
}

template <bool IsWhite, bool IsKingside>
inline uint64_t update_hash_castle(uint64_t key) {
    constexpr int color_offset = IsWhite ? 0 : 384;
    constexpr int king_offset = 5 * 64;
    constexpr int rook_offset = 3 * 64;

    if constexpr (IsWhite) {
        if constexpr (IsKingside) {
            key ^= random_key[4 + color_offset + king_offset];
            key ^= random_key[6 + color_offset + king_offset];
            key ^= random_key[7 + color_offset + rook_offset];
            key ^= random_key[5 + color_offset + rook_offset];
        } else {
            key ^= random_key[4 + color_offset + king_offset];
            key ^= random_key[2 + color_offset + king_offset];
            key ^= random_key[0 + color_offset + rook_offset];
            key ^= random_key[3 + color_offset + rook_offset];
        }
    } else {
        if constexpr (IsKingside) {
            key ^= random_key[60 + color_offset + king_offset];
            key ^= random_key[62 + color_offset + king_offset];
            key ^= random_key[63 + color_offset + rook_offset];
            key ^= random_key[61 + color_offset + rook_offset];
        } else {
            key ^= random_key[60 + color_offset + king_offset];
            key ^= random_key[58 + color_offset + king_offset];
            key ^= random_key[56 + color_offset + rook_offset];
            key ^= random_key[59 + color_offset + rook_offset];
        }
    }

    return key;
}

template <bool IsWhite>
inline uint64_t update_hash_en_passant(uint64_t key, int from, int to) {
    constexpr int moving_color_offset = IsWhite ? 0 : 384;
    constexpr int captured_color_offset = IsWhite ? 384 : 0;

    key ^= random_key[from + moving_color_offset];

    key ^= random_key[to + moving_color_offset];

    int captured_square = IsWhite ? (to - 8) : (to + 8);
    key ^= random_key[captured_square + captured_color_offset];

    return key;
}

inline uint64_t toggle_side_to_move(uint64_t key) {
    key ^= random_key[768];
    __builtin_prefetch(&TT.Table[(key) & (TT.size - 1)], 1, 1);
    return key;
}

#include "hash.hpp"


#define CREATE_SEARCH_INFO(ep_val, irreversible_val, capture_val) \
    minimax_info_t searchInfo; \
    searchInfo.ep = ep_val; \
    searchInfo.alpha = alpha; \
    searchInfo.beta = beta; \
    searchInfo.score = score; \
    searchInfo.key = newKey; \
    searchInfo.depth = depth; \
    searchInfo.irreversibleCount = irreversible_val; \
    searchInfo.ply = ply; \
    searchInfo.isPVNode = isPVNode; \
    searchInfo.isCapture = capture_val; \
    searchInfo.prevMove = prevMove; \
    searchInfo.from = from; \
    searchInfo.to = to; \
    searchInfo.nullMove = false; \
    searchInfo.accPair = accPair; \


#define EXTRACT_MOVE_INFO(info) \
    int from = info.from; \
    int to = info.to; \
    int alpha = info.alpha; \
    int beta = info.beta; \
    int score = info.score; \
    uint64_t key = info.key; \
    int depth = info.depth; \
    int irreversibleCount = info.irreversibleCount; \
    int ply = info.ply; \
    bool isPVNode = info.isPVNode; \
    minimax_info_t* prevMove = info.prevMove; \
    AccumulatorPair* accPair = info.accPair; \



template <class BoardState status>
constexpr inline int pawnMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 0, from, to);
    newKey = toggle_side_to_move(newKey);
    accumulatorAddPiece(accPair, 0 ,status.IsWhite, to);
    accumulatorSubPiece(accPair, 0 ,status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, 0, 0);
    int val = searchFunc<status.normal()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 0 ,status.IsWhite, to);
    accumulatorAddPiece(accPair, 0 ,status.IsWhite, from);
    return val;
}

template <class BoardState status>
constexpr inline int
pawnDoubleMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 0, from, to);
    newKey = toggle_side_to_move(newKey);
    accumulatorAddPiece(accPair, 0 ,status.IsWhite, to);
    accumulatorSubPiece(accPair, 0 ,status.IsWhite, from);
    CREATE_SEARCH_INFO(to+(status.IsWhite ? -8 : 8), 0, 0);
    int val = searchFunc<status.pawn()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 0 ,status.IsWhite, to);
    accumulatorAddPiece(accPair, 0 ,status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int pawnCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::Pawn, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 0, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);
    accumulatorAddPiece(accPair, 0, status.IsWhite, to);
    accumulatorSubPiece(accPair, 0, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.normal()>(newBoard,searchInfo);
    } else {
        val = searchFunc<status.normal()>(newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 0, status.IsWhite, to);
    accumulatorAddPiece(accPair, 0, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int promote(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);

    uint64_t newKey = update_hash_promotion<status.IsWhite>(key, from, to, 4);
    newKey = toggle_side_to_move(newKey);

    Board newBoard1 = brd.promote<BoardPiece::Queen, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);

    accumulatorAddPiece(accPair, 4, status.IsWhite, to);
    accumulatorSubPiece(accPair, 0, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, 0, 0);
    int val = searchFunc<status.normal()>(newBoard1, searchInfo);
    accumulatorSubPiece(accPair, 4, status.IsWhite, to);
    accumulatorAddPiece(accPair, 0, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int
promoteCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey =
        update_hash_promotion_capture<status.IsWhite, !status.IsWhite>(
            key, capturedPiece, from, to, 4);
    newKey = toggle_side_to_move(newKey);

    Board newBoard1 =
        brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    
    accumulatorAddPiece(accPair, 4, status.IsWhite, to);
    accumulatorSubPiece(accPair, 0, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.normal()>(newBoard1, searchInfo);
    } else {
        val = searchFunc<status.normal()>(newBoard1, searchInfo);
    }
    accumulatorSubPiece(accPair, 4, status.IsWhite, to);
    accumulatorAddPiece(accPair, 0, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int EP(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.EP<BoardPiece::Pawn, status.IsWhite, status.WLC,
                            status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_en_passant<status.IsWhite>(key, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 0, status.IsWhite, to);
    accumulatorSubPiece(accPair, 0, status.IsWhite, from);
    accumulatorSubPiece(accPair, 0, !status.IsWhite,
                           (status.IsWhite ? to - 8 : to + 8));
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val = searchFunc<status.pawn()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 0, status.IsWhite, to);
    accumulatorAddPiece(accPair, 0, status.IsWhite, from);
    accumulatorAddPiece(accPair, 0, !status.IsWhite,
                        (status.IsWhite ? to - 8 : to + 8));
    return val;
}

template <class BoardState status>
constexpr inline int knightMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Knight, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 1, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 1, status.IsWhite, to);
    accumulatorSubPiece(accPair, 1, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, irreversibleCount + 1, 0);
    int val = searchFunc<status.normal()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 1, status.IsWhite, to);
    accumulatorAddPiece(accPair, 1, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int knightCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::Knight, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 1, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 1, status.IsWhite, to);
    accumulatorSubPiece(accPair, 1, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.normal()>(newBoard, searchInfo);
    } else {
        val = searchFunc<status.normal()>(newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 1, status.IsWhite, to);
    accumulatorAddPiece(accPair, 1, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int bishopMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Bishop, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 2, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 2, status.IsWhite, to);
    accumulatorSubPiece(accPair, 2, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, irreversibleCount + 1, 0);
    int val = searchFunc<status.normal()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 2, status.IsWhite, to);
    accumulatorAddPiece(accPair, 2, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int bishopCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 2, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 2, status.IsWhite, to);
    accumulatorSubPiece(accPair, 2, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.normal()>(newBoard, searchInfo);
    } else {
        val = searchFunc<status.normal()>(newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 2, status.IsWhite, to);
    accumulatorAddPiece(accPair, 2, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int rookMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Rook, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 3, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 3, status.IsWhite, to);
    accumulatorSubPiece(accPair, 3, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, 0, 0);
    int val = -2000000;
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                val = searchFunc<status.rookMoveLeft()>(
                    newBoard, searchInfo);
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                val = searchFunc<status.rookMoveRight()>(
                    newBoard, searchInfo);
            }
        }

    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                val = searchFunc<status.rookMoveLeft()>(
                    newBoard, searchInfo);
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                val = searchFunc<status.rookMoveRight()>(
                    newBoard, searchInfo);
            }
        }

    }
    if (val == -2000000){
        searchInfo.irreversibleCount = irreversibleCount + 1;
        val = searchFunc<status.normal()>(
                newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 3, status.IsWhite, to);
    accumulatorAddPiece(accPair, 3, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int rookCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::Rook, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 3, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 3, status.IsWhite, to);
    accumulatorSubPiece(accPair, 3, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    int val = -2000000;
    CREATE_SEARCH_INFO(-1, 0, 1);
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                if constexpr (quite) {
                    val = quiescence<status.rookMoveLeft()>(
                        newBoard, searchInfo);
                } else {
                    val = searchFunc<status.rookMoveLeft()>(
                        newBoard, searchInfo);
                }
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                if constexpr (quite) {
                    val = quiescence<status.rookMoveRight()>(
                        newBoard, searchInfo);
                } else {
                    val = searchFunc<status.rookMoveRight()>(
                        newBoard, searchInfo);
                }
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                if constexpr (quite) {
                    val = quiescence<status.rookMoveLeft()>(
                        newBoard, searchInfo);
                } else {
                    val = searchFunc<status.rookMoveLeft()>(
                        newBoard, searchInfo);
                }
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                if constexpr (quite) {
                    val = quiescence<status.rookMoveRight()>(
                        newBoard, searchInfo);
                } else {
                    val = searchFunc<status.rookMoveRight()>(
                        newBoard, searchInfo);
                }
            }
        }
    }
    if (val == -2000000) {
        if constexpr (quite) {
            val = quiescence<status.normal()>(newBoard, searchInfo);
        } else {
            val = searchFunc<status.normal()>(newBoard, searchInfo);
        }
    }
    accumulatorSubPiece(accPair, 3, status.IsWhite, to);
    accumulatorAddPiece(accPair, 3, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int queenMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::Queen, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 4, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 4, status.IsWhite, to);
    accumulatorSubPiece(accPair, 4, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, irreversibleCount + 1, 0);
    int val = searchFunc<status.normal()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 4, status.IsWhite, to);
    accumulatorAddPiece(accPair, 4, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int queenCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 4, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 4, status.IsWhite, to);
    accumulatorSubPiece(accPair, 4, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.normal()>(newBoard, searchInfo);
    } else {
        val = searchFunc<status.normal()>(newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 4, status.IsWhite, to);
    accumulatorAddPiece(accPair, 4, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int kingMove(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.move<BoardPiece::King, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 5, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 5, status.IsWhite, to);
    accumulatorSubPiece(accPair, 5, status.IsWhite, from);
    CREATE_SEARCH_INFO(-1, irreversibleCount + 1, 0);
    int val = searchFunc<status.king()>(newBoard, searchInfo);
    accumulatorSubPiece(accPair, 5, status.IsWhite, to);
    accumulatorAddPiece(accPair, 5, status.IsWhite, from);
    return val;
}

template <class BoardState status, bool quite>
constexpr inline int kingCapture(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    Board newBoard = brd.capture<BoardPiece::King, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 5, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    accumulatorAddPiece(accPair, 5, status.IsWhite, to);
    accumulatorSubPiece(accPair, 5, status.IsWhite, from);
    accumulatorSubPiece(accPair, capturedPiece, !status.IsWhite, to);
    CREATE_SEARCH_INFO(-1, 0, 1);
    int val;
    if constexpr (quite) {
        val = quiescence<status.king()>(newBoard, searchInfo);
    } else {
        val = searchFunc<status.king()>(newBoard, searchInfo);
    }
    accumulatorSubPiece(accPair, 5, status.IsWhite, to);
    accumulatorAddPiece(accPair, 5, status.IsWhite, from);
    accumulatorAddPiece(accPair, capturedPiece, !status.IsWhite, to);
    return val;
}

template <class BoardState status>
constexpr inline int leftCastel(const Board &brd, move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    uint64_t newKey;


    int val;
    if constexpr (status.IsWhite) {
        newKey = update_hash_castle<true, false>(key);
        newKey = toggle_side_to_move(newKey);
        
        accumulatorAddPiece(accPair, 5, status.IsWhite, 2);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 4);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 3);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 0);
        CREATE_SEARCH_INFO(-1, 0, 0);
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, true,
                                    false, false, false>();

        val = searchFunc<status.king()>(newBoard, searchInfo);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 2);
        accumulatorAddPiece(accPair, 5, status.IsWhite, 4);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 3);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 0);
    } else {
        newKey = update_hash_castle<false, false>(key);
        newKey = toggle_side_to_move(newKey);

        accumulatorAddPiece(accPair, 5, status.IsWhite, 58);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 60);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 59);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 56);
        CREATE_SEARCH_INFO(-1, 0, 0);
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, true, false>();

        val = searchFunc<status.king()>(newBoard, searchInfo);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 58);
        accumulatorAddPiece(accPair, 5, status.IsWhite, 60);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 59);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 56);
    }
    return val;
}

template <class BoardState status>
constexpr inline int rightCastel(const Board &brd,move_info_t& info) noexcept {
    EXTRACT_MOVE_INFO(info);
    uint64_t newKey;

    int val;
    if constexpr (status.IsWhite) {
        newKey = update_hash_castle<true, true>(key);
        newKey = toggle_side_to_move(newKey);

        accumulatorAddPiece(accPair, 5, status.IsWhite, 6);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 4);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 5);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 7);
        CREATE_SEARCH_INFO(-1, 0, 0);
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    true, false, false>();

        val = searchFunc<status.king()>(newBoard, searchInfo);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 6);
        accumulatorAddPiece(accPair, 5, status.IsWhite, 4);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 5);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 7);

    } else {
        newKey = update_hash_castle<false, true>(key);
        newKey = toggle_side_to_move(newKey);
        accumulatorAddPiece(accPair, 5, status.IsWhite, 62);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 60);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 61);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 63);
        CREATE_SEARCH_INFO(-1, 0, 0);
        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, false, true>();

        val = searchFunc<status.king()>(newBoard, searchInfo);
        accumulatorSubPiece(accPair, 5, status.IsWhite, 62);
        accumulatorAddPiece(accPair, 5, status.IsWhite, 60);
        accumulatorSubPiece(accPair, 3, status.IsWhite, 61);
        accumulatorAddPiece(accPair, 3, status.IsWhite, 63);
    }
    return val;
}
