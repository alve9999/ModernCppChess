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

using MakeMoveFunc = void (*)(const Board &, int, int);

_fast void moveHandler(const MakeMoveFunc &move, Callback *ml, int &count,
                       int from, int to) noexcept {
    Callback &cb = ml[count++];
    cb.move = move;
    cb.from = from;
    cb.to = to;
}

template <bool IsWhite> _fast uint64_t enemyOrEmpty(const Board &brd) noexcept {
    if constexpr (IsWhite) {
        return brd.Black | ~brd.Occ;
    }
    return brd.White | ~brd.Occ;
}

template <class BoardState status, int depth>
_fast static void pawnMoves(const Board &brd, uint64_t chessMask, int64_t pinHV,
                            uint64_t pinD, Callback *ml, int &count) noexcept {
    uint64_t pinnedD, pinnedHV, notPinned;
    if constexpr (status.IsWhite) {
        pinnedD = brd.WPawn & pinD;
        pinnedHV = brd.WPawn & pinHV;
        notPinned = brd.WPawn & ~(pinnedD | pinnedHV);
    } else {
        pinnedD = brd.BPawn & pinD;
        pinnedHV = brd.BPawn & pinHV;
        notPinned = brd.BPawn & ~(pinnedD | pinnedHV);
    }
    uint64_t forwardNotPinned =
        (pawnForward<status.IsWhite>(notPinned, brd)) & chessMask;
    uint64_t fowardPinned =
        (pawnForward<status.IsWhite>(pinnedHV, brd)) & chessMask & pinHV;
    uint64_t forward = forwardNotPinned | fowardPinned;
    uint64_t promotions = promotion(forward);
    forward = forward & (~promotions);
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (status.IsWhite) {
            const int from = to - 8;
            moveHandler(promote<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 8;
            moveHandler(promote<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(forward) {
        const int to = __builtin_ctzll(forward);
        if constexpr (status.IsWhite) {
            const int from = to - 8;
            moveHandler(pawnMove<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 8;
            moveHandler(pawnMove<status, depth>, ml, count, from, to);
        }
    }
    uint64_t doubleForwardNotPinned =
        pawnDoubleForward<status.IsWhite>(notPinned, brd) & chessMask;
    uint64_t doubleForwardPinned =
        pawnDoubleForward<status.IsWhite>(pinnedHV, brd) & chessMask & pinHV;
    uint64_t doubleForward = doubleForwardNotPinned | doubleForwardPinned;
    Bitloop(doubleForward) {
        const int to = __builtin_ctzll(doubleForward);
        if constexpr (status.IsWhite) {
            const int from = to - 16;
            moveHandler(pawnDoubleMove<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 16;
            moveHandler(pawnDoubleMove<status, depth>, ml, count, from, to);
        }
    }
    uint64_t leftNotPinned = pawnAttackLeft<status.IsWhite>(notPinned, brd);
    uint64_t leftPinned = pawnAttackLeft<status.IsWhite>(pinnedD, brd);
    uint64_t left = (leftNotPinned | (leftPinned & pinD)) & chessMask;
    promotions = promotion(left);
    left = left & ~promotions;
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (status.IsWhite) {
            const int from = to - 7;
            moveHandler(promoteCapture<status, depth>, ml, count, from, to);

        } else {
            const int from = to + 9;
            moveHandler(promoteCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(left) {
        const int to = __builtin_ctzll(left);
        if constexpr (status.IsWhite) {
            const int from = to - 7;
            moveHandler(pawnCapture<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 9;
            moveHandler(pawnCapture<status, depth>, ml, count, from, to);
        }
    }
    uint64_t rightNotPinned = pawnAttackRight<status.IsWhite>(notPinned, brd);
    uint64_t rightPinned = pawnAttackRight<status.IsWhite>(pinnedD, brd);
    uint64_t right = (rightNotPinned | (rightPinned & pinD)) & chessMask;
    promotions = promotion(right);
    right = right & ~promotions;
    Bitloop(promotions) {
        const int to = __builtin_ctzll(promotions);
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            moveHandler(promoteCapture<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 7;
            moveHandler(promoteCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(right) {
        const int to = __builtin_ctzll(right);
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            moveHandler(pawnCapture<status, depth>, ml, count, from, to);
        } else {
            const int from = to + 7;
            moveHandler(pawnCapture<status, depth>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth>
_fast static void knightMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD, Callback *ml,
                              int &count) noexcept {
    uint64_t knights;
    if constexpr (status.IsWhite) {
        knights = brd.WKnight & ~(pinHV | pinD);
    } else {
        knights = brd.BKnight & ~(pinHV | pinD);
    }
    Bitloop(knights) {
        const int from = __builtin_ctzll(knights);
        uint64_t attacks =
            knightMasks[from] & chessMask & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(knightMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(knightCapture<status, depth>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth>
_fast static void bishopMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD, Callback *ml,
                              int &count) noexcept {
    uint64_t bishopsNotPinned, bishopsPinnedD;
    if constexpr (status.IsWhite) {
        bishopsNotPinned = brd.WBishop & ~(pinHV | pinD);
        bishopsPinnedD = brd.WBishop & pinD;
    } else {
        bishopsNotPinned = brd.BBishop & ~(pinHV | pinD);
        bishopsPinnedD = brd.BBishop & pinD;
    }
    Bitloop(bishopsNotPinned) {
        const int from = __builtin_ctzll(bishopsNotPinned);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks = attacks & chessMask & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(bishopMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(bishopCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(bishopsPinnedD) {
        const int from = __builtin_ctzll(bishopsPinnedD);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinD & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(bishopMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(bishopCapture<status, depth>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth>
_fast static void queenMoves(const Board &brd, uint64_t chessMask,
                             uint64_t pinHV, uint64_t pinD, Callback *ml,
                             int &count) noexcept {
    uint64_t queenNotPinned, queenPinnedD, queenPinnedHV;
    if constexpr (status.IsWhite) {
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
        attacks = attacks & chessMask & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(queenMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(queenCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(queenPinnedD) {
        const int from = __builtin_ctzll(queenPinnedD);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinD & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(queenMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(queenCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(queenPinnedHV) {
        const int from = __builtin_ctzll(queenPinnedHV);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinHV & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(queenMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(queenCapture<status, depth>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth>
_fast static void rookMoves(const Board &brd, uint64_t chessMask,
                            uint64_t pinHV, uint64_t pinD, Callback *ml,
                            int &count) noexcept {
    uint64_t rooksNotPinned, rooksPinnedHV;
    if constexpr (status.IsWhite) {
        rooksNotPinned = brd.WRook & ~(pinHV | pinD);
        rooksPinnedHV = brd.WRook & pinHV;
    } else {
        rooksNotPinned = brd.BRook & ~(pinHV | pinD);
        rooksPinnedHV = brd.BRook & pinHV;
    }
    Bitloop(rooksNotPinned) {
        const int from = __builtin_ctzll(rooksNotPinned);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks = attacks & chessMask & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(rookMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(rookCapture<status, depth>, ml, count, from, to);
        }
    }
    Bitloop(rooksPinnedHV) {
        const int from = __builtin_ctzll(rooksPinnedHV);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinHV & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        attacks = attacks & ~brd.Occ;
        Bitloop(attacks) {
            const int to = __builtin_ctzll(attacks);
            moveHandler(rookMove<status, depth>, ml, count, from, to);
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            moveHandler(rookCapture<status, depth>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth>
_fast static void kingMoves(const Board &brd, uint64_t chessMask,
                            uint64_t kingBan, int kingPos, Callback *ml,
                            int &count) noexcept {
    uint64_t moves =
        kingMasks[kingPos] & ~kingBan & enemyOrEmpty<status.IsWhite>(brd);
    uint64_t captures = moves & brd.Occ;
    moves = moves & ~brd.Occ;
    Bitloop(moves) {
        const int to = __builtin_ctzll(moves);
        moveHandler(kingMove<status, depth>, ml, count, kingPos, to);
    }
    Bitloop(captures) {
        const int to = __builtin_ctzll(captures);
        moveHandler(kingCapture<status, depth>, ml, count, kingPos, to);
    }
}

template <class BoardState status, int depth>
_fast void castels(const Board &brd, uint64_t kingBan, Callback *ml,
                   int &count) noexcept {
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if ((!(WNotOccupiedL & brd.Occ)) && (!(WNotAttackedL & kingBan)) &&
                (brd.WRook & WRookL)) {
                moveHandler(leftCastel<status, depth>, ml, count, 4, 2);
            }
        }
        if constexpr (status.WRC) {
            if ((!(WNotOccupiedR & brd.Occ)) && (!(WNotAttackedR & kingBan)) &&
                (brd.WRook & WRookR)) {
                moveHandler(rightCastel<status, depth>, ml, count, 4, 6);
            }
        }
    } else {
        if constexpr (status.BLC) {
            if ((!(BNotOccupiedL & brd.Occ)) && (!(BNotAttackedL & kingBan)) &&
                (brd.BRook & BRookL)) {
                moveHandler(leftCastel<status, depth>, ml, count, 60, 58);
            }
        }
        if constexpr (status.BRC) {
            if ((!(BNotOccupiedR & brd.Occ)) && (!(BNotAttackedR & kingBan)) &&
                (brd.BRook & BRookR)) {
                moveHandler(rightCastel<status, depth>, ml, count, 60, 62);
            }
        }
    }
}

template <class BoardState status, int depth>
_fast void EPMoves(const Board &brd, int ep, Callback *ml, int &count,
                   uint64_t pinD, uint64_t pinHV) noexcept {
    uint64_t EPRight, EPLeft, EPLeftPinned, EPRightPinned;
    uint64_t EPSquare = 1ull << ep;
    if constexpr (status.IsWhite) {
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
        const int from = to - (status.IsWhite ? -9 : 7);
        moveHandler(EP<status, depth>, ml, count, from, to);
    }
    if (EPLeft) {
        const int to = __builtin_ctzll(EPLeft);
        const int from = to - (status.IsWhite ? -7 : 9);
        moveHandler(EP<status, depth>, ml, count, from, to);
    }
}

template <class BoardState status, int depth>
_fast void genMoves(const Board &brd, int ep, Callback *ml,
                    int &count) noexcept {
    uint64_t king;
    if constexpr (status.IsWhite) {
        king = brd.WKing;
    } else {
        king = brd.BKing;
    }
    int kingPos = __builtin_ctzll(king);
    uint64_t checkmask = ONES;
    uint64_t pinHV = 0;
    uint64_t pinD = 0;
    uint64_t kingBan = 0;
    generateCheck<status.IsWhite>(brd, kingPos, pinHV, pinD, checkmask,
                                  kingBan);
    pawnMoves<status, depth>(brd, checkmask, pinHV, pinD, ml, count);
    knightMoves<status, depth>(brd, checkmask, pinHV, pinD, ml, count);
    bishopMoves<status, depth>(brd, checkmask, pinHV, pinD, ml, count);
    queenMoves<status, depth>(brd, checkmask, pinHV, pinD, ml, count);
    rookMoves<status, depth>(brd, checkmask, pinHV, pinD, ml, count);
    kingMoves<status, depth>(brd, checkmask, kingBan, kingPos, ml, count);
    if constexpr (status.WLC || status.WRC || status.BLC || status.BRC) {
        castels<status, depth>(brd, kingBan, ml, count);
    }
    if constexpr (status.EP) {
        EPMoves<status, depth>(brd, ep, ml, count, pinD, pinHV);
    }
}
/*
_fast void moveGenCall(const Board &brd, const int ep, Callback* ml, count, bool
WH, bool EP, bool WL, bool WR, bool BL, bool BR) noexcept { if (WH && EP && WL
&& WR
&& BL && BR) genMoves<true, true, true, true, true, true>(brd, ep, ml, count);
if (WH
&& EP && WL && WR && BL && !BR) genMoves<true, true, true, true, true,
false>(brd, ep, ml, count); if (WH && EP && WL && WR && !BL && BR)
genMoves<true, true, true, true, false, true>(brd, ep, ml, count); if (WH && EP
&& WL && WR && !BL && !BR) genMoves<true, true, true, true, false, false>(brd,
ep, ml, count); if (WH && EP && WL && !WR && BL && BR) genMoves<true, true,
true, false, true, true>(brd, ep, ml, count); if (WH && EP && WL && !WR && BL &&
!BR) genMoves<true, true, true, false, true, false>(brd, ep, ml, count); if (WH
&& EP && WL && !WR && !BL && BR) genMoves<true, true, true, false, false,
true>(brd, ep, ml, count); if (WH && EP && WL && !WR && !BL && !BR)
         genMoves<true, true, true, false, false, false>(brd, ep, ml, count);
    if (WH && EP && !WL && WR && BL && BR)
         genMoves<true, true, false, true, true, true>(brd, ep, ml, count);
    if (WH && EP && !WL && WR && BL && !BR)
         genMoves<true, true, false, true, true, false>(brd, ep, ml, count);
    if (WH && EP && !WL && WR && !BL && BR)
         genMoves<true, true, false, true, false, true>(brd, ep, ml, count);
    if (WH && EP && !WL && WR && !BL && !BR)
         genMoves<true, true, false, true, false, false>(brd, ep, ml, count);
    if (WH && EP && !WL && !WR && BL && BR)
         genMoves<true, true, false, false, true, true>(brd, ep, ml, count);
    if (WH && EP && !WL && !WR && BL && !BR)
         genMoves<true, true, false, false, true, false>(brd, ep, ml, count);
    if (WH && EP && !WL && !WR && !BL && BR)
         genMoves<true, true, false, false, false, true>(brd, ep, ml, count);
    if (WH && EP && !WL && !WR && !BL && !BR)
         genMoves<true, true, false, false, false, false>(brd, ep, ml, count);
    if (WH && !EP && WL && WR && BL && BR)
         genMoves<true, false, true, true, true, true>(brd, ep, ml, count);
    if (WH && !EP && WL && WR && BL && !BR)
         genMoves<true, false, true, true, true, false>(brd, ep, ml, count);
    if (WH && !EP && WL && WR && !BL && BR)
         genMoves<true, false, true, true, false, true>(brd, ep, ml, count);
    if (WH && !EP && WL && WR && !BL && !BR)
         genMoves<true, false, true, true, false, false>(brd, ep, ml, count);
    if (WH && !EP && WL && !WR && BL && BR)
         genMoves<true, false, true, false, true, true>(brd, ep, ml, count);
    if (WH && !EP && WL && !WR && BL && !BR)
         genMoves<true, false, true, false, true, false>(brd, ep, ml, count);
    if (WH && !EP && WL && !WR && !BL && BR)
         genMoves<true, false, true, false, false, true>(brd, ep, ml, count);
    if (WH && !EP && WL && !WR && !BL && !BR)
         genMoves<true, false, true, false, false, false>(brd, ep, ml, count);
    if (WH && !EP && !WL && WR && BL && BR)
         genMoves<true, false, false, true, true, true>(brd, ep, ml, count);
    if (WH && !EP && !WL && WR && BL && !BR)
         genMoves<true, false, false, true, true, false>(brd, ep, ml, count);
    if (WH && !EP && !WL && WR && !BL && BR)
         genMoves<true, false, false, true, false, true>(brd, ep, ml, count);
    if (WH && !EP && !WL && WR && !BL && !BR)
         genMoves<true, false, false, true, false, false>(brd, ep, ml, count);
    if (WH && !EP && !WL && !WR && BL && BR)
         genMoves<true, false, false, false, true, true>(brd, ep, ml, count);
    if (WH && !EP && !WL && !WR && BL && !BR)
         genMoves<true, false, false, false, true, false>(brd, ep, ml, count);
    if (WH && !EP && !WL && !WR && !BL && BR)
         genMoves<true, false, false, false, false, true>(brd, ep, ml, count);
    if (WH && !EP && !WL && !WR && !BL && !BR)
         genMoves<true, false, false, false, false, false>(brd, ep, ml, count);
    if (!WH && EP && WL && WR && BL && BR)
         genMoves<false, true, true, true, true, true>(brd, ep, ml, count);
    if (!WH && EP && WL && WR && BL && !BR)
         genMoves<false, true, true, true, true, false>(brd, ep, ml, count);
    if (!WH && EP && WL && WR && !BL && BR)
         genMoves<false, true, true, true, false, true>(brd, ep, ml, count);
    if (!WH && EP && WL && WR && !BL && !BR)
         genMoves<false, true, true, true, false, false>(brd, ep, ml, count);
    if (!WH && EP && WL && !WR && BL && BR)
         genMoves<false, true, true, false, true, true>(brd, ep, ml, count);
    if (!WH && EP && WL && !WR && BL && !BR)
         genMoves<false, true, true, false, true, false>(brd, ep, ml, count);
    if (!WH && EP && WL && !WR && !BL && BR)
         genMoves<false, true, true, false, false, true>(brd, ep, ml, count);
    if (!WH && EP && WL && !WR && !BL && !BR)
         genMoves<false, true, true, false, false, false>(brd, ep, ml, count);
    if (!WH && EP && !WL && WR && BL && BR)
         genMoves<false, true, false, true, true, true>(brd, ep, ml, count);
    if (!WH && EP && !WL && WR && BL && !BR)
         genMoves<false, true, false, true, true, false>(brd, ep, ml, count);
    if (!WH && EP && !WL && WR && !BL && BR)
         genMoves<false, true, false, true, false, true>(brd, ep, ml, count);
    if (!WH && EP && !WL && WR && !BL && !BR)
         genMoves<false, true, false, true, false, false>(brd, ep, ml, count);
    if (!WH && EP && !WL && !WR && BL && BR)
         genMoves<false, true, false, false, true, true>(brd, ep, ml, count);
    if (!WH && EP && !WL && !WR && BL && !BR)
         genMoves<false, true, false, false, true, false>(brd, ep, ml, count);
    if (!WH && EP && !WL && !WR && !BL && BR)
         genMoves<false, true, false, false, false, true>(brd, ep, ml, count);
    if (!WH && EP && !WL && !WR && !BL && !BR)
         genMoves<false, true, false, false, false, false>(brd, ep, ml, count);
    if (!WH && !EP && WL && WR && BL && BR)
         genMoves<false, false, true, true, true, true>(brd, ep, ml, count);
    if (!WH && !EP && WL && WR && BL && !BR)
         genMoves<false, false, true, true, true, false>(brd, ep, ml, count);
    if (!WH && !EP && WL && WR && !BL && BR)
         genMoves<false, false, true, true, false, true>(brd, ep, ml, count);
    if (!WH && !EP && WL && WR && !BL && !BR)
         genMoves<false, false, true, true, false, false>(brd, ep, ml, count);
    if (!WH && !EP && WL && !WR && BL && BR)
         genMoves<false, false, true, false, true, true>(brd, ep, ml, count);
    if (!WH && !EP && WL && !WR && BL && !BR)
         genMoves<false, false, true, false, true, false>(brd, ep, ml, count);
    if (!WH && !EP && WL && !WR && !BL && BR)
         genMoves<false, false, true, false, false, true>(brd, ep, ml, count);
    if (!WH && !EP && WL && !WR && !BL && !BR)
         genMoves<false, false, true, false, false, false>(brd, ep, ml, count);
    if (!WH && !EP && !WL && WR && BL && BR)
         genMoves<false, false, false, true, true, true>(brd, ep, ml, count);
    if (!WH && !EP && !WL && WR && BL && !BR)
         genMoves<false, false, false, true, true, false>(brd, ep, ml, count);
    if (!WH && !EP && !WL && WR && !BL && BR)
         genMoves<false, false, false, true, false, true>(brd, ep, ml, count);
    if (!WH && !EP && !WL && WR && !BL && !BR)
         genMoves<false, false, false, true, false, false>(brd, ep, ml, count);
    if (!WH && !EP && !WL && !WR && BL && BR)
         genMoves<false, false, false, false, true, true>(brd, ep, ml, count);
    if (!WH && !EP && !WL && !WR && BL && !BR)
         genMoves<false, false, false, false, true, false>(brd, ep, ml, count);
    if (!WH && !EP && !WL && !WR && !BL && BR)
         genMoves<false, false, false, false, false, true>(brd, ep, ml, count);
    if (!WH && !EP && !WL && !WR && !BL && !BR)
         genMoves<false, false, false, false, false, false>(brd, ep, ml, count);
}*/
