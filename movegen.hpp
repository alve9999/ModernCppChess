#pragma once
#include "MoveList.hpp"
#include "board.hpp"
#include "check.hpp"
#include "constants.hpp"
#include "move.hpp"
#include "pawns.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

using StateMoveFunc = BoardState (BoardState::*)(int) const noexcept;
using MakeMoveFunc = Board (Board::*)(const Move&) const noexcept;


inline void moveHandler(const MakeMoveFunc& move, const StateMoveFunc& state, const uint8_t from, const uint8_t to,
                        const BoardPiece piece, const uint8_t special,
                        MoveList& ml) noexcept {
    ml.addMove(move,state,from, to, piece, special);
}

inline void moveHandlerEp(const MakeMoveFunc& move, const StateMoveFunc& state, const uint8_t from, const uint8_t to,
                        const BoardPiece piece, const uint8_t special,
                        MoveList& ml, const int ep) noexcept {
    ml.addMove(move,state,from, to, piece, special,ep);
}

template <bool IsWhite> _fast uint64_t enemyOrEmpty(const Board &brd) noexcept {
    if constexpr (IsWhite) {
        return brd.Black | ~brd.Occ;
    }
    return brd.White | ~brd.Occ;
}

template <bool IsWhite>
_fast static void pawnMoves(const Board &brd, uint64_t chessMask, int64_t pinHV,
                            uint64_t pinD, MoveList &ml) noexcept {
    uint64_t pinnedD, pinnedHV, notPinned;
    if constexpr (IsWhite) {
        pinnedD = brd.WPawn & pinD;
        pinnedHV = brd.WPawn & pinHV;
        notPinned = brd.WPawn & ~(pinnedD | pinnedHV);
    } else {
        pinnedD = brd.BPawn & pinD;
        pinnedHV = brd.BPawn & pinHV;
        notPinned = brd.BPawn & ~(pinnedD | pinnedHV);
    }
    uint64_t forwardNotPinned =
        (pawnForward<IsWhite>(notPinned, brd)) & chessMask;
    uint64_t fowardPinned =
        (pawnForward<IsWhite>(pinnedHV, brd)) & chessMask & pinHV;
    uint64_t forward = forwardNotPinned | fowardPinned;
    uint64_t promotions = promotion(forward);
    forward = forward & (~promotions);
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (IsWhite) {
            const int from = to - 8;
            moveHandler(&Board::template promote<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 8, ml);
            moveHandler(&Board::template promote<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 9, ml);
            moveHandler(&Board::template promote<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 10, ml);
            moveHandler(&Board::template promote<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 11, ml);
        } else {
            const int from = to + 8;
            moveHandler(&Board::template promote<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 8, ml);
            moveHandler(&Board::template promote<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 9, ml);
            moveHandler(&Board::template promote<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 10, ml);
            moveHandler(&Board::template promote<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 11, ml);
        }
    }
    Bitloop(forward) {
        const int to = __builtin_ctzll(forward);
        if constexpr (IsWhite) {
            const int from = to - 8;
            moveHandler(&Board::template move<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 0, ml);
        } else {
            const int from = to + 8;
            moveHandler(&Board::template move<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 0, ml);
        }
    }
    uint64_t doubleForwardNotPinned =
        pawnDoubleForward<IsWhite>(notPinned, brd) & chessMask;
    uint64_t doubleForwardPinned =
        pawnDoubleForward<IsWhite>(pinnedHV, brd) & chessMask & pinHV;
    uint64_t doubleForward = doubleForwardNotPinned | doubleForwardPinned;
    Bitloop(doubleForward) {
        const int to = __builtin_ctzll(doubleForward);
        if constexpr (IsWhite) {
            const int from = to - 16;
            moveHandlerEp(&Board::template move<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::pawn,from, to, BoardPiece::Pawn, 1, ml, to + 8);
        } else {
            const int from = to + 16;
            moveHandlerEp(&Board::template move<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::pawn,from, to, BoardPiece::Pawn, 1, ml, to - 8);
        }
    }
    uint64_t leftNotPinned = pawnAttackLeft<IsWhite>(notPinned, brd);
    uint64_t leftPinned = pawnAttackLeft<IsWhite>(pinnedD, brd);
    uint64_t left = (leftNotPinned | (leftPinned & pinD)) & chessMask;
    promotions = promotion(left);
    left = left & ~promotions;
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (IsWhite) {
            const int from = to - 7;
            moveHandler(&Board::template promoteCapture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 12, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 13, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 14, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 15, ml);
        } else {
            const int from = to + 9;
            moveHandler(&Board::template promoteCapture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 12, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 13, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 14, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 15, ml);
        }
    }
    Bitloop(left) {
        const int to = __builtin_ctzll(left);
        if constexpr (IsWhite) {
            const int from = to - 7;
            moveHandler(&Board::template capture<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 4, ml);
        } else {
            const int from = to + 9;
            moveHandler(&Board::template capture<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 4, ml);
        }
    }
    uint64_t rightNotPinned = pawnAttackRight<IsWhite>(notPinned, brd);
    uint64_t rightPinned = pawnAttackRight<IsWhite>(pinnedD, brd);
    uint64_t right = (rightNotPinned | (rightPinned & pinD)) & chessMask;
    promotions = promotion(right);
    right = right & ~promotions;
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (IsWhite) {
            const int from = to - 9;
            moveHandler(&Board::template promoteCapture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 12, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 13, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 14, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 15, ml);
        } else {
            const int from = to + 7;
            moveHandler(&Board::template promoteCapture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 12, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Rook,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 13, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 14, ml);
            moveHandler(&Board::template promoteCapture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 15, ml);
        }
    }
    Bitloop(right) {
        const int to = __builtin_ctzll(right);
        if constexpr (IsWhite) {
            const int from = to - 9;
            moveHandler(&Board::template capture<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 4, ml);
        } else {
            const int from = to + 7;
            moveHandler(&Board::template capture<BoardPiece::Pawn,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Pawn, 4, ml);
        }
    }
}

template <bool IsWhite>
_fast static void knightMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD,
                              MoveList &ml) noexcept {
    uint64_t knights;
    if constexpr (IsWhite) {
        knights = brd.WKnight & ~(pinHV | pinD);
    } else {
        knights = brd.BKnight & ~(pinHV | pinD);
    }
    Bitloop(knights) {
        const int from = __builtin_ctzll(knights);
        uint64_t attacks =
            knightMasks[from] & chessMask & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Knight, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Knight,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Knight, 4, ml);
        }
    }
}

template <bool IsWhite>
_fast static void bishopMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD,
                              MoveList &ml) noexcept {
    uint64_t bishopsNotPinned, bishopsPinnedD;
    if constexpr (IsWhite) {
        bishopsNotPinned = brd.WBishop & ~(pinHV | pinD);
        bishopsPinnedD = brd.WBishop & pinD;
    } else {
        bishopsNotPinned = brd.BBishop & ~(pinHV | pinD);
        bishopsPinnedD = brd.BBishop & pinD;
    }
    Bitloop(bishopsNotPinned) {
        const int from = __builtin_ctzll(bishopsNotPinned);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks = attacks & chessMask & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Bishop, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Bishop, 4, ml);
        }
    }
    Bitloop(bishopsPinnedD) {
        const int from = __builtin_ctzll(bishopsPinnedD);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks = attacks & chessMask & pinD & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Bishop, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Bishop,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Bishop, 4, ml);
        }
    }
}

template <bool IsWhite>
_fast static void queenMoves(const Board &brd, uint64_t chessMask,
                             uint64_t pinHV, uint64_t pinD,
                             MoveList &ml) noexcept {
    uint64_t queenNotPinned, queenPinnedD, queenPinnedHV;
    if constexpr (IsWhite) {
        queenNotPinned = brd.WQueen & ~(pinHV | pinD);
        queenPinnedD = brd.WQueen & pinD;
        queenPinnedHV = brd.WQueen & pinHV;
    } else {
        queenNotPinned = brd.BQueen & ~(pinHV | pinD);
        queenPinnedD = brd.BQueen & pinD;
        queenPinnedHV = brd.BQueen & pinHV;
    }
    Bitloop(queenNotPinned) {
        const int from = __builtin_ctzll(queenNotPinned);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks = attacks & chessMask & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 4, ml);
        }
    }
    Bitloop(queenPinnedD) {
        const int from = __builtin_ctzll(queenPinnedD);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks = attacks & chessMask & pinD & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 4, ml);
        }
    }
    Bitloop(queenPinnedHV) {
        const int from = __builtin_ctzll(queenPinnedHV);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks = attacks & chessMask & pinHV & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(&Board::template move<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 0, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(&Board::template capture<BoardPiece::Queen,IsWhite,false,false,false,false>,&BoardState::normal,from, to, BoardPiece::Queen, 4, ml);
        }
    }
}

template <bool IsWhite, bool WLC, bool WRC, bool BLC, bool BRC>
_fast void handleRookMove(int from, int to, MoveList& ml) {
    if constexpr (IsWhite) {
        if constexpr (WLC) {
            if (from == 0) {
                moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Left, from, to, BoardPiece::Rook, 0, ml);
            }
            return;
        }
        if constexpr (WRC) {
            if (from == 7) {
                moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 0, ml);
            }
            return;
        }
        moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 0, ml);
    } else {
        if constexpr (BLC) {
            if (from == 56) {
                moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Left, from, to, BoardPiece::Rook, 0, ml);
                return;
            }
        }
        if constexpr (BRC) {
            if (from == 63) {
                moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 0, ml);
                return;
            }
        }
        moveHandler(&Board::template move<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::normal, from, to, BoardPiece::Rook, 0, ml);
    }
}
template <bool IsWhite, bool WLC, bool WRC, bool BLC, bool BRC>
_fast void handleCaptureRookMove(int from, int to, MoveList& ml) {
    if constexpr (IsWhite) {
        if constexpr (WLC) {
            if (from == 0) {
                moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Left, from, to, BoardPiece::Rook, 4, ml);
            }
            return;
        }
        if constexpr (WRC) {
            if (from == 7) {
                moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 4, ml);
            }
            return;
        }
        moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 4, ml);
    } else {
        if constexpr (BLC) {
            if (from == 56) {
                moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Left, from, to, BoardPiece::Rook, 4, ml);
                return;
            }
        }
        if constexpr (BRC) {
            if (from == 63) {
                moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::RookMove_Right, from, to, BoardPiece::Rook, 4, ml);
                return;
            }
        }
        moveHandler(&Board::template capture<BoardPiece::Rook,IsWhite,false,false,false,false>, &BoardState::normal, from, to, BoardPiece::Rook, 4, ml);
    }
}

template <bool IsWhite,bool WLC,bool WRC,bool BRC,bool BLC>
_fast static void rookMoves(const Board &brd, uint64_t chessMask,
                            uint64_t pinHV, uint64_t pinD,
                            MoveList &ml) noexcept {
    uint64_t rooksNotPinned, rooksPinnedHV;
    if constexpr (IsWhite) {
        rooksNotPinned = brd.WRook & ~(pinHV | pinD);
        rooksPinnedHV = brd.WRook & pinHV;
    } else {
        rooksNotPinned = brd.BRook & ~(pinHV | pinD);
        rooksPinnedHV = brd.BRook & pinHV;
    }
    Bitloop(rooksNotPinned) {
        const int from = __builtin_ctzll(rooksNotPinned);
        assert(from < 64);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks = attacks & chessMask & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            handleRookMove<IsWhite,WLC,WRC,BRC,BLC>(from, to, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            handleCaptureRookMove<IsWhite,WLC,WRC,BRC,BLC>(from, to, ml);
        }
    }
    Bitloop(rooksPinnedHV) {
        const int from = __builtin_ctzll(rooksPinnedHV);
        assert(from < 64);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks = attacks & chessMask & pinHV & enemyOrEmpty<IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            handleRookMove<IsWhite,WLC,WRC,BRC,BLC>(from, to, ml);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            handleCaptureRookMove<IsWhite,WLC,WRC,BRC,BLC>(from, to, ml);
        }
    }
}

template <bool IsWhite>
_fast static void kingMoves(const Board &brd, uint64_t chessMask,
                            uint64_t kingBan, int kingPos,
                            MoveList &ml) noexcept {
    uint64_t moves = kingMasks[kingPos] & ~kingBan & enemyOrEmpty<IsWhite>(brd);
    uint64_t captures = moves & brd.Occ;
    moves = moves & ~brd.Occ;
    Bitloop(moves) {
        const int to = __builtin_ctzll(moves);
        moveHandler(&Board::template move<BoardPiece::King,IsWhite,false,false,false,false>,&BoardState::king,kingPos, to, BoardPiece::King, 0, ml);
    }
    Bitloop(captures) {
        const int to = __builtin_ctzll(captures);
        moveHandler(&Board::template capture<BoardPiece::King,IsWhite,false,false,false,false>,&BoardState::king,kingPos, to, BoardPiece::King, 4, ml);
    }
}

template <bool IsWhite, bool WLC, bool WRC, bool BLC, bool BRC>
_fast void castels(const Board &brd, uint64_t kingBan, MoveList &ml) noexcept {
    if constexpr (IsWhite) {
        if constexpr (WLC) {
            if ((!(WNotOccupiedL & brd.Occ)) && (!(WNotAttackedL & kingBan)) &&
                (brd.WRook & WRookL)) {
                moveHandler(&Board::template castle<BoardPiece::King, IsWhite, false, true, false, false>,&BoardState::king,4, 2, BoardPiece::King, 2, ml);
            }
        }
        if constexpr (WRC) {
            if ((!(WNotOccupiedR & brd.Occ)) && (!(WNotAttackedR & kingBan)) &&
                (brd.WRook & WRookR)) {
                moveHandler(&Board::template castle<BoardPiece::King,IsWhite,false,true,false,false>,&BoardState::king,4, 6, BoardPiece::King, 2, ml);
            }
        }
    } else {
        if constexpr (BLC) {
            if ((!(BNotOccupiedL & brd.Occ)) && (!(BNotAttackedL & kingBan)) &&
                (brd.BRook & BRookL)) {
                moveHandler(&Board::template castle<BoardPiece::King,IsWhite,false,false,true,false>,&BoardState::king,60, 58, BoardPiece::King, 2, ml);
            }
        }
        if constexpr (BRC) {
            if ((!(BNotOccupiedR & brd.Occ)) && (!(BNotAttackedR & kingBan)) &&
                (brd.BRook & BRookR)) {
                moveHandler(&Board::template castle<BoardPiece::King,IsWhite,false,false,false,true>,&BoardState::king,60, 62, BoardPiece::King, 2, ml);
            }
        }
    }
}

template <bool IsWhite>
_fast void EPMoves(const Board &brd,int ep, MoveList &ml, uint64_t pinD,
                   uint64_t pinHV) noexcept {
    uint64_t EPRight, EPLeft, EPLeftPinned, EPRightPinned;
    uint64_t EPSquare = 1ull << ep;
    if constexpr (IsWhite) {
        EPRight = (((brd.WPawn & ~(pinHV | pinD)) & ~File1) << 9) & EPSquare;
        EPLeft = (((brd.WPawn & ~(pinHV | pinD)) & ~File8) << 7) & EPSquare;
        EPRightPinned = (((brd.WPawn & pinD) & ~File1) << 9) & EPSquare & pinD;
        EPLeftPinned = (((brd.WPawn & pinD) & ~File8) << 7) & EPSquare & pinD;
    } else {
        EPRight = (((brd.BPawn & ~(pinHV | pinD)) & ~File1) >> 7) & EPSquare;
        EPLeft = (((brd.BPawn & ~(pinHV | pinD)) & ~File8) >> 9) & EPSquare;
        EPRightPinned = (((brd.BPawn & pinD) & ~File1) >> 7) & EPSquare & pinD;
        EPLeftPinned = (((brd.BPawn & pinD) & ~File8) >> 9) & EPSquare & pinD;
    }
    if (EPRight) {
        const int to = __builtin_ctzll(EPRight);
        const int from = to - (IsWhite ? -9 : 7);
        moveHandler(&Board::template EP<BoardPiece::Pawn,IsWhite,false,true,false,false>, &BoardState::normal,from, to, BoardPiece::Pawn, 3, ml);
    }
    if (EPLeft) {
        const int to = __builtin_ctzll(EPLeft);
        const int from = to - (IsWhite ? -7 : 9);
        moveHandler(&Board::template EP<BoardPiece::Pawn,IsWhite,false,true,false,false>, &BoardState::normal,from, to, BoardPiece::Pawn, 3, ml);
    }
}

template <bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC, bool BRC>
_fast void genMoves(const Board &brd,int ep, MoveList &ml) noexcept
{
    uint64_t king;
    if constexpr (IsWhite) {
        king = brd.WKing;
    } else {
        king = brd.BKing;
    }
    int kingPos = __builtin_ctzll(king);
    uint64_t checkmask = ONES;
    uint64_t pinHV = 0;
    uint64_t pinD = 0;
    uint64_t kingBan = 0;
    generateCheck<IsWhite>(brd, kingPos, pinHV, pinD, checkmask, kingBan);
    pawnMoves<IsWhite>(brd, checkmask, pinHV, pinD, ml);
    knightMoves<IsWhite>(brd, checkmask, pinHV, pinD, ml);
    bishopMoves<IsWhite>(brd, checkmask, pinHV, pinD, ml);
    queenMoves<IsWhite>(brd, checkmask, pinHV, pinD, ml);
    rookMoves<IsWhite, WLC, WRC, BLC, BRC>(brd, checkmask, pinHV, pinD, ml);
    kingMoves<IsWhite>(brd, checkmask, kingBan, kingPos, ml);
    if constexpr (WLC|| WRC|| BLC|| BRC){
        castels<IsWhite, WLC, WRC, BLC, BRC>(brd, kingBan, ml);
    }
    if constexpr (EP) {
        EPMoves<IsWhite>(brd, ep, ml, pinD, pinHV);
    }
}

_fast void moveGenCall(const Board &brd, const int ep, MoveList &ml, bool WH, bool EP,
                               bool WL, bool WR, bool BL, bool BR) noexcept {
    if (WH && EP && WL && WR && BL && BR)
        genMoves<true, true, true, true, true, true>(brd, ep, ml);
    if (WH && EP && WL && WR && BL && !BR)
        genMoves<true, true, true, true, true, false>(brd, ep, ml);
    if (WH && EP && WL && WR && !BL && BR)
        genMoves<true, true, true, true, false, true>(brd, ep, ml);
    if (WH && EP && WL && WR && !BL && !BR)
         genMoves<true, true, true, true, false, false>(brd, ep, ml);
    if (WH && EP && WL && !WR && BL && BR)
         genMoves<true, true, true, false, true, true>(brd, ep, ml);
    if (WH && EP && WL && !WR && BL && !BR)
         genMoves<true, true, true, false, true, false>(brd, ep, ml);
    if (WH && EP && WL && !WR && !BL && BR)
         genMoves<true, true, true, false, false, true>(brd, ep, ml);
    if (WH && EP && WL && !WR && !BL && !BR)
         genMoves<true, true, true, false, false, false>(brd, ep, ml);
    if (WH && EP && !WL && WR && BL && BR)
         genMoves<true, true, false, true, true, true>(brd, ep, ml);
    if (WH && EP && !WL && WR && BL && !BR)
         genMoves<true, true, false, true, true, false>(brd, ep, ml);
    if (WH && EP && !WL && WR && !BL && BR)
         genMoves<true, true, false, true, false, true>(brd, ep, ml);
    if (WH && EP && !WL && WR && !BL && !BR)
         genMoves<true, true, false, true, false, false>(brd, ep, ml);
    if (WH && EP && !WL && !WR && BL && BR)
         genMoves<true, true, false, false, true, true>(brd, ep, ml);
    if (WH && EP && !WL && !WR && BL && !BR)
         genMoves<true, true, false, false, true, false>(brd, ep, ml);
    if (WH && EP && !WL && !WR && !BL && BR)
         genMoves<true, true, false, false, false, true>(brd, ep, ml);
    if (WH && EP && !WL && !WR && !BL && !BR)
         genMoves<true, true, false, false, false, false>(brd, ep, ml);
    if (WH && !EP && WL && WR && BL && BR)
         genMoves<true, false, true, true, true, true>(brd, ep, ml);
    if (WH && !EP && WL && WR && BL && !BR)
         genMoves<true, false, true, true, true, false>(brd, ep, ml);
    if (WH && !EP && WL && WR && !BL && BR)
         genMoves<true, false, true, true, false, true>(brd, ep, ml);
    if (WH && !EP && WL && WR && !BL && !BR)
         genMoves<true, false, true, true, false, false>(brd, ep, ml);
    if (WH && !EP && WL && !WR && BL && BR)
         genMoves<true, false, true, false, true, true>(brd, ep, ml);
    if (WH && !EP && WL && !WR && BL && !BR)
         genMoves<true, false, true, false, true, false>(brd, ep, ml);
    if (WH && !EP && WL && !WR && !BL && BR)
         genMoves<true, false, true, false, false, true>(brd, ep, ml);
    if (WH && !EP && WL && !WR && !BL && !BR)
         genMoves<true, false, true, false, false, false>(brd, ep, ml);
    if (WH && !EP && !WL && WR && BL && BR)
         genMoves<true, false, false, true, true, true>(brd, ep, ml);
    if (WH && !EP && !WL && WR && BL && !BR)
         genMoves<true, false, false, true, true, false>(brd, ep, ml);
    if (WH && !EP && !WL && WR && !BL && BR)
         genMoves<true, false, false, true, false, true>(brd, ep, ml);
    if (WH && !EP && !WL && WR && !BL && !BR)
         genMoves<true, false, false, true, false, false>(brd, ep, ml);
    if (WH && !EP && !WL && !WR && BL && BR)
         genMoves<true, false, false, false, true, true>(brd, ep, ml);
    if (WH && !EP && !WL && !WR && BL && !BR)
         genMoves<true, false, false, false, true, false>(brd, ep, ml);
    if (WH && !EP && !WL && !WR && !BL && BR)
         genMoves<true, false, false, false, false, true>(brd, ep, ml);
    if (WH && !EP && !WL && !WR && !BL && !BR)
         genMoves<true, false, false, false, false, false>(brd, ep, ml);
    if (!WH && EP && WL && WR && BL && BR)
         genMoves<false, true, true, true, true, true>(brd, ep, ml);
    if (!WH && EP && WL && WR && BL && !BR)
         genMoves<false, true, true, true, true, false>(brd, ep, ml);
    if (!WH && EP && WL && WR && !BL && BR)
         genMoves<false, true, true, true, false, true>(brd, ep, ml);
    if (!WH && EP && WL && WR && !BL && !BR)
         genMoves<false, true, true, true, false, false>(brd, ep, ml);
    if (!WH && EP && WL && !WR && BL && BR)
         genMoves<false, true, true, false, true, true>(brd, ep, ml);
    if (!WH && EP && WL && !WR && BL && !BR)
         genMoves<false, true, true, false, true, false>(brd, ep, ml);
    if (!WH && EP && WL && !WR && !BL && BR)
         genMoves<false, true, true, false, false, true>(brd, ep, ml);
    if (!WH && EP && WL && !WR && !BL && !BR)
         genMoves<false, true, true, false, false, false>(brd, ep, ml);
    if (!WH && EP && !WL && WR && BL && BR)
         genMoves<false, true, false, true, true, true>(brd, ep, ml);
    if (!WH && EP && !WL && WR && BL && !BR)
         genMoves<false, true, false, true, true, false>(brd, ep, ml);
    if (!WH && EP && !WL && WR && !BL && BR)
         genMoves<false, true, false, true, false, true>(brd, ep, ml);
    if (!WH && EP && !WL && WR && !BL && !BR)
         genMoves<false, true, false, true, false, false>(brd, ep, ml);
    if (!WH && EP && !WL && !WR && BL && BR)
         genMoves<false, true, false, false, true, true>(brd, ep, ml);
    if (!WH && EP && !WL && !WR && BL && !BR)
         genMoves<false, true, false, false, true, false>(brd, ep, ml);
    if (!WH && EP && !WL && !WR && !BL && BR)
         genMoves<false, true, false, false, false, true>(brd, ep, ml);
    if (!WH && EP && !WL && !WR && !BL && !BR)
         genMoves<false, true, false, false, false, false>(brd, ep, ml);
    if (!WH && !EP && WL && WR && BL && BR)
         genMoves<false, false, true, true, true, true>(brd, ep, ml);
    if (!WH && !EP && WL && WR && BL && !BR)
         genMoves<false, false, true, true, true, false>(brd, ep, ml);
    if (!WH && !EP && WL && WR && !BL && BR)
         genMoves<false, false, true, true, false, true>(brd, ep, ml);
    if (!WH && !EP && WL && WR && !BL && !BR)
         genMoves<false, false, true, true, false, false>(brd, ep, ml);
    if (!WH && !EP && WL && !WR && BL && BR)
         genMoves<false, false, true, false, true, true>(brd, ep, ml);
    if (!WH && !EP && WL && !WR && BL && !BR)
         genMoves<false, false, true, false, true, false>(brd, ep, ml);
    if (!WH && !EP && WL && !WR && !BL && BR)
         genMoves<false, false, true, false, false, true>(brd, ep, ml);
    if (!WH && !EP && WL && !WR && !BL && !BR)
         genMoves<false, false, true, false, false, false>(brd, ep, ml);
    if (!WH && !EP && !WL && WR && BL && BR)
         genMoves<false, false, false, true, true, true>(brd, ep, ml);
    if (!WH && !EP && !WL && WR && BL && !BR)
         genMoves<false, false, false, true, true, false>(brd, ep, ml);
    if (!WH && !EP && !WL && WR && !BL && BR)
         genMoves<false, false, false, true, false, true>(brd, ep, ml);
    if (!WH && !EP && !WL && WR && !BL && !BR)
         genMoves<false, false, false, true, false, false>(brd, ep, ml);
    if (!WH && !EP && !WL && !WR && BL && BR)
         genMoves<false, false, false, false, true, true>(brd, ep, ml);
    if (!WH && !EP && !WL && !WR && BL && !BR)
         genMoves<false, false, false, false, true, false>(brd, ep, ml);
    if (!WH && !EP && !WL && !WR && !BL && BR)
         genMoves<false, false, false, false, false, true>(brd, ep, ml);
    if (!WH && !EP && !WL && !WR && !BL && !BR)
         genMoves<false, false, false, false, false, false>(brd, ep, ml);
}


template <int depth> void perft(const Board &brd, const BoardState &state, long long &c) noexcept {
    if constexpr (depth == 0) {
        c++;
        return;
    } else {
        MoveList ml = MoveList();
        moveGenCall(brd, state.ep,ml, state.IsWhite, state.EP, state.WLC,state.WRC, state.BLC, state.BRC);

        for (const moveCallback &m : ml.Moves) {
            Board newBoard = (brd.*(m.moveFunc))(m.move);
            BoardState newState = (state.*(m.state))(m.ep);
            perft<depth - 1>(newBoard, newState, c);
        }
    }
}
