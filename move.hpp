#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"
#include "hash.hpp"
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
MoveCallbacks algebraicToMove(std::string &alg, const Board &brd,
                              const BoardState state, int ep) {
    uint8_t from = algToCoord(alg.substr(0, 2));
    uint8_t to = algToCoord(alg.substr(2, 4));
    /*std::cout << "icnoming move:" << std::endl;
    std::cout << "alg: " << alg << std::endl;
    std::cout << (int)from << "  " << (int)to << std::endl;
    std::cout << "isWhite: " << IsWhite << std::endl;*/
    BoardPiece type = BoardPiece::Pawn;
    bool capture = ISSET(brd.Occ, to);
    int special = 0;

    if constexpr (IsWhite) {
        if (ISSET(brd.WPawn, from)) {
            type = BoardPiece::Pawn;
            if ((abs(to - from) == 7 || abs(to - from) == 9) && !capture) {
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
            if ((abs(to - from) == 7 || abs(to - from) == 9) && !capture) {
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

    return MoveCallbacks{boardCallback, stateCallback};
}

static long long c = 0;

struct MoveResult {
    Board board;
    BoardState state;
};

using SearchMoveFunc = int (*)(const Board &, int, int, int, int, int, uint64_t,
                               int);
using MakeMoveFunc = MoveResult (*)(const Board &, int, int);

struct Callback {
    MakeMoveFunc makeMove;
    SearchMoveFunc move;
    uint8_t from;
    uint8_t to;
    int value;
};

/*
constexpr const int gd = 8;
constexpr const bool count_succ = false;
template <class BoardState status>
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
        else if (brd.BKing & (1ULL << to))
            capturedPiece = 5;
        else {
            printBitboard(brd.BQueen);
            printBitboard(brd.BRook);
            printBitboard(brd.BBishop);
            printBitboard(brd.BKnight);
            printBitboard(brd.BPawn);
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
        else if (brd.WKing & (1ULL << to))
            capturedPiece = 5;
        else {
            printBitboard(brd.WQueen);
            printBitboard(brd.WRook);
            printBitboard(brd.WBishop);
            printBitboard(brd.WKnight);
            printBitboard(brd.WPawn);
            assert(false);
        }
    }
    return capturedPiece;
}

template <bool IsWhite, BoardPiece Piece>
constexpr inline int calculateMoveScoreDelta(int from, int to) {
    if constexpr (IsWhite) {
        return mg_table[static_cast<int>(Piece)][true][to] -
               mg_table[static_cast<int>(Piece)][true][from];
    } else {
        return mg_table[static_cast<int>(Piece)][false][to] -
               mg_table[static_cast<int>(Piece)][false][from];
    }
}

template <bool IsWhite, BoardPiece AttackerPiece>
constexpr inline int calculateCaptureScoreDelta(int victimPiece, int from,
                                                int to) {
    int delta = 0;

    if constexpr (IsWhite) {
        delta += mg_table[static_cast<int>(AttackerPiece)][true][to] -
                 mg_table[static_cast<int>(AttackerPiece)][true][from];
    } else {
        delta += mg_table[static_cast<int>(AttackerPiece)][false][to] -
                 mg_table[static_cast<int>(AttackerPiece)][false][from];
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
    return key ^ random_key[768];
}

template <class BoardState status>
constexpr inline int pawnMove(const Board &brd, int from, int to, int alpha,
                              int beta, int score, uint64_t key,
                              int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 0, from, to);
    newKey = toggle_side_to_move(newKey);
    return searchFunc<status.normal()>(newBoard, 0, alpha, beta, score + delta,
                                       newKey, depth);
}

template <class BoardState status>
constexpr inline int pawnDoubleMove(const Board &brd, int from, int to,
                                    int alpha, int beta, int score,
                                    uint64_t key, int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Pawn, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);

    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 0, from, to);
    newKey = toggle_side_to_move(newKey);
    return searchFunc<status.pawn()>(newBoard, to + 8, alpha, beta,
                                     score + delta, newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int pawnCapture(const Board &brd, int from, int to, int alpha,
                                 int beta, int score, uint64_t key,
                                 int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::Pawn, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);

    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Pawn>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 0, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int promote(const Board &brd, int from, int to, int alpha,
                             int beta, int score, uint64_t key,
                             int depth) noexcept {
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);

    uint64_t newKey = update_hash_promotion<status.IsWhite>(key, from, to, 4);
    newKey = toggle_side_to_move(newKey);

    if constexpr (status.IsWhite) {
        delta += mg_value[4] - mg_value[0];
        delta += mg_table[4][true][to] - mg_table[0][true][to];
    } else {
        delta += mg_value[4] - mg_value[0];
        delta += mg_table[4][false][to] - mg_table[0][false][to];
    }
    Board newBoard1 = brd.promote<BoardPiece::Queen, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    int val = searchFunc<status.normal()>(newBoard1, 0, alpha, beta,
                                          score + delta, newKey, depth);
    return val;
    Board newBoard2 = brd.promote<BoardPiece::Rook, status.IsWhite, status.WLC,
                                  status.WRC, status.BLC, status.BRC>(from, to);
    int val2 = searchFunc<status.normal()>(newBoard2, 0, alpha, beta,
                                           score + delta, newKey, depth);

    Board newBoard3 =
        brd.promote<BoardPiece::Bishop, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    int val3 = searchFunc<status.normal()>(newBoard3, 0, alpha, beta,
                                           score + delta, newKey, depth);

    Board newBoard4 =
        brd.promote<BoardPiece::Knight, status.IsWhite, status.WLC, status.WRC,
                    status.BLC, status.BRC>(from, to);
    int val4 = searchFunc<status.normal()>(newBoard4, 0, alpha, beta,
                                           score + delta, newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int promoteCapture(const Board &brd, int from, int to,
                                    int alpha, int beta, int score,
                                    uint64_t key, int depth) noexcept {
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);

    uint64_t newKey =
        update_hash_promotion_capture<status.IsWhite, !status.IsWhite>(
            key, capturedPiece, from, to, 4);
    newKey = toggle_side_to_move(newKey);

    if constexpr (status.IsWhite) {
        delta += mg_value[capturedPiece];
        delta += mg_table[capturedPiece][false][to];
        delta += mg_value[4] - mg_value[0];
        delta += mg_table[4][true][to] - mg_table[0][true][to];
    } else {
        delta += mg_value[capturedPiece];
        delta += mg_table[capturedPiece][true][to];
        delta += mg_value[4] - mg_value[0];
        delta += mg_table[4][false][to] - mg_table[0][false][to];
    }

    Board newBoard1 =
        brd.promoteCapture<BoardPiece::Queen, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard1, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard1, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
    Board newBoard2 =
        brd.promoteCapture<BoardPiece::Rook, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val2 = searchFunc<status.normal()>(newBoard2, 0, alpha, beta,
                                           score + delta, newKey, depth);

    Board newBoard3 =
        brd.promoteCapture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val3 = searchFunc<status.normal()>(newBoard3, 0, alpha, beta,
                                           score + delta, newKey, depth);

    Board newBoard4 =
        brd.promoteCapture<BoardPiece::Knight, status.IsWhite, status.WLC,
                           status.WRC, status.BLC, status.BRC>(from, to);
    int val4 = searchFunc<status.normal()>(newBoard4, 0, alpha, beta,
                                           score + delta, newKey, depth);
}

template <class BoardState status>
constexpr inline int EP(const Board &brd, int from, int to, int alpha, int beta,
                        int score, uint64_t key, int depth) noexcept {
    Board newBoard = brd.EP<BoardPiece::Pawn, status.IsWhite, status.WLC,
                            status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Pawn>(from, to);
    if constexpr (status.IsWhite) {
        delta += mg_value[0];
        delta += mg_table[0][false][to - 8];
    } else {
        delta += mg_value[0];
        delta += mg_table[0][true][to + 8];
    }

    uint64_t newKey = update_hash_en_passant<status.IsWhite>(key, from, to);
    newKey = toggle_side_to_move(newKey);
    return searchFunc<status.pawn()>(newBoard, 0, alpha, beta, score + delta,
                                     newKey, depth);
}

template <class BoardState status>
constexpr inline int knightMove(const Board &brd, int from, int to, int alpha,
                                int beta, int score, uint64_t key,
                                int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Knight, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Knight>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 1, from, to);
    newKey = toggle_side_to_move(newKey);

    return searchFunc<status.normal()>(newBoard, 0, alpha, beta, score + delta,
                                       newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int knightCapture(const Board &brd, int from, int to,
                                   int alpha, int beta, int score, uint64_t key,
                                   int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::Knight, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Knight>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 1, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int bishopMove(const Board &brd, int from, int to, int alpha,
                                int beta, int score, uint64_t key,
                                int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Bishop, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Bishop>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 2, from, to);
    newKey = toggle_side_to_move(newKey);

    return searchFunc<status.normal()>(newBoard, 0, alpha, beta, score + delta,
                                       newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int bishopCapture(const Board &brd, int from, int to,
                                   int alpha, int beta, int score, uint64_t key,
                                   int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::Bishop, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Bishop>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 2, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int rookMove(const Board &brd, int from, int to, int alpha,
                              int beta, int score, uint64_t key,
                              int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Rook, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Rook>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 3, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                return searchFunc<status.rookMoveLeft()>(
                    newBoard, 0, alpha, beta, score + delta, newKey, depth);
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                return searchFunc<status.rookMoveRight()>(
                    newBoard, 0, alpha, beta, score + delta, newKey, depth);
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                return searchFunc<status.rookMoveLeft()>(
                    newBoard, 0, alpha, beta, score + delta, newKey, depth);
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                return searchFunc<status.rookMoveRight()>(
                    newBoard, 0, alpha, beta, score + delta, newKey, depth);
            }
        }
    }
    return searchFunc<status.normal()>(newBoard, 0, alpha, beta, score + delta,
                                       newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int rookCapture(const Board &brd, int from, int to, int alpha,
                                 int beta, int score, uint64_t key,
                                 int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::Rook, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);
    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Rook>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 3, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if (from == 0) {
                if constexpr (quite) {
                    return quiescence<status.rookMoveLeft()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                } else {
                    return searchFunc<status.rookMoveLeft()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                }
            }
        }
        if constexpr (status.WRC) {
            if (from == 7) {
                if constexpr (quite) {
                    return quiescence<status.rookMoveRight()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                } else {
                    return searchFunc<status.rookMoveRight()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                }
            }
        }
    } else {
        if constexpr (status.BLC) {
            if (from == 56) {
                if constexpr (quite) {
                    return quiescence<status.rookMoveLeft()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                } else {
                    return searchFunc<status.rookMoveLeft()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                }
            }
        }
        if constexpr (status.BRC) {
            if (from == 63) {
                if constexpr (quite) {
                    return quiescence<status.rookMoveRight()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                } else {
                    return searchFunc<status.rookMoveRight()>(
                        newBoard, 0, alpha, beta, score + delta, newKey, depth);
                }
            }
        }
    }
    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int queenMove(const Board &brd, int from, int to, int alpha,
                               int beta, int score, uint64_t key,
                               int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::Queen, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::Queen>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 4, from, to);
    newKey = toggle_side_to_move(newKey);

    return searchFunc<status.normal()>(newBoard, 0, alpha, beta, score + delta,
                                       newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int queenCapture(const Board &brd, int from, int to, int alpha,
                                  int beta, int score, uint64_t key,
                                  int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::Queen, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::Queen>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 4, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (quite) {
        return quiescence<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int kingMove(const Board &brd, int from, int to, int alpha,
                              int beta, int score, uint64_t key,
                              int depth) noexcept {
    Board newBoard = brd.move<BoardPiece::King, status.IsWhite, status.WLC,
                              status.WRC, status.BLC, status.BRC>(from, to);
    int delta =
        calculateMoveScoreDelta<status.IsWhite, BoardPiece::King>(from, to);

    uint64_t newKey = update_hash_move<status.IsWhite>(key, 5, from, to);
    newKey = toggle_side_to_move(newKey);

    return searchFunc<status.king()>(newBoard, 0, alpha, beta, score + delta,
                                     newKey, depth);
}

template <class BoardState status, bool quite>
constexpr inline int kingCapture(const Board &brd, int from, int to, int alpha,
                                 int beta, int score, uint64_t key,
                                 int depth) noexcept {
    Board newBoard = brd.capture<BoardPiece::King, status.IsWhite, status.WLC,
                                 status.WRC, status.BLC, status.BRC>(from, to);

    int capturedPiece = getCapturePiece<status.IsWhite>(brd, to);
    int delta = calculateCaptureScoreDelta<status.IsWhite, BoardPiece::King>(
        capturedPiece, from, to);

    uint64_t newKey = update_hash_capture<status.IsWhite, !status.IsWhite>(
        key, 5, capturedPiece, from, to);
    newKey = toggle_side_to_move(newKey);

    if constexpr (quite) {
        return quiescence<status.king()>(newBoard, 0, alpha, beta,
                                         score + delta, newKey, depth);
    } else {
        return searchFunc<status.king()>(newBoard, 0, alpha, beta,
                                         score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int leftCastel(const Board &brd, int from, int to, int alpha,
                                int beta, int score, uint64_t key,
                                int depth) noexcept {
    int delta = 0;
    uint64_t newKey;
    if constexpr (status.IsWhite) {
        delta += mg_table[static_cast<int>(BoardPiece::King)][true][2] -
                 mg_table[static_cast<int>(BoardPiece::King)][true][4];
        delta += mg_table[static_cast<int>(BoardPiece::Rook)][true][3] -
                 mg_table[static_cast<int>(BoardPiece::Rook)][true][0];

        newKey = update_hash_castle<true, false>(key);
        newKey = toggle_side_to_move(newKey);

        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, true,
                                    false, false, false>();

        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        delta += mg_table[static_cast<int>(BoardPiece::King)][false][58] -
                 mg_table[static_cast<int>(BoardPiece::King)][false][60];
        delta += mg_table[static_cast<int>(BoardPiece::Rook)][false][59] -
                 mg_table[static_cast<int>(BoardPiece::Rook)][false][56];

        newKey = update_hash_castle<false, false>(key);
        newKey = toggle_side_to_move(newKey);

        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, true, false>();
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}

template <class BoardState status>
constexpr inline int rightCastel(const Board &brd, int from, int to, int alpha,
                                 int beta, int score, uint64_t key,
                                 int depth) noexcept {
    int delta = 0;
    uint64_t newKey;
    if constexpr (status.IsWhite) {
        delta += mg_table[static_cast<int>(BoardPiece::King)][true][6] -
                 mg_table[static_cast<int>(BoardPiece::King)][true][4];
        delta += mg_table[static_cast<int>(BoardPiece::Rook)][true][5] -
                 mg_table[static_cast<int>(BoardPiece::Rook)][true][7];

        newKey = update_hash_castle<true, true>(key);
        newKey = toggle_side_to_move(newKey);

        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    true, false, false>();

        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    } else {
        delta += mg_table[static_cast<int>(BoardPiece::King)][false][62] -
                 mg_table[static_cast<int>(BoardPiece::King)][false][60];
        delta += mg_table[static_cast<int>(BoardPiece::Rook)][false][61] -
                 mg_table[static_cast<int>(BoardPiece::Rook)][false][63];

        newKey = update_hash_castle<false, true>(key);
        newKey = toggle_side_to_move(newKey);

        Board newBoard = brd.castle<BoardPiece::King, status.IsWhite, false,
                                    false, false, true>();
        return searchFunc<status.normal()>(newBoard, 0, alpha, beta,
                                           score + delta, newKey, depth);
    }
}
