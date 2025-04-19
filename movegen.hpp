#pragma once
#include "board.hpp"
#include "check.hpp"
#include "constants.hpp"
#include "move.hpp"
#include "pawns.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

using SearchMoveFunc = int (*)(const Board &, int, int, int, int, int,
                               uint64_t);
using MakeMoveFunc = MoveResult (*)(const Board &, int, int);
inline std::string converter(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row);

    return std::string(1, file) + std::string(1, rank);
}

inline void moveHandler(const MakeMoveFunc &move, Callback *ml, int &count,
                        int from, int to) noexcept {
    Callback &cb = ml[count++];
    cb.makeMove = move;
    cb.from = from;
    cb.to = to;
}

_fast void moveHandler(const SearchMoveFunc &move, Callback *ml, int &count,
                       int from, int to) noexcept {
    Callback &cb = ml[count++];
    cb.move = move;
    cb.from = from;
    cb.to = to;
}

_fast void goodMoveHandler(const SearchMoveFunc &move, Callback *ml, int &count,
                           int &goodCount, int from, int to) noexcept {
    Callback *cb;
    if (goodCount != 0) {
        cb = &ml[goodCount--];
    } else {
        cb = &ml[count++];
    }
    cb->move = move;
    cb->from = from;
    cb->to = to;
}

inline void goodMoveHandler(const MakeMoveFunc &move, Callback *ml, int &count,
                            int &goodCount, int from, int to) noexcept {
    Callback *cb;
    if (goodCount != 0) {
        cb = &ml[goodCount--];
    } else {
        cb = &ml[count++];
    }
    cb->makeMove = move;
    cb->from = from;
    cb->to = to;
}

template <bool IsWhite> _fast uint64_t enemyOrEmpty(const Board &brd) noexcept {
    if constexpr (IsWhite) {
        return brd.Black | ~brd.Occ;
    }
    return brd.White | ~brd.Occ;
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void pawnMoves(const Board &brd, uint64_t chessMask, int64_t pinHV,
                            uint64_t pinD, Callback *ml, int &count,
                            int &goodCount) noexcept {
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
    uint64_t promotions = 0;
    if constexpr (!capturesOnly) {
        uint64_t forwardNotPinned =
            (pawnForward<status.IsWhite>(notPinned, brd)) & chessMask;
        uint64_t fowardPinned =
            (pawnForward<status.IsWhite>(pinnedHV, brd)) & chessMask & pinHV;
        uint64_t forward = forwardNotPinned | fowardPinned;
        promotions = promotion(forward);
        forward = forward & (~promotions);
        Bitloop(promotions) {
            const int to = __builtin_ctzll(promotions);
            if constexpr (status.IsWhite) {
                const int from = to - 8;
                if constexpr (search) {
                    goodMoveHandler(promote<status, depth>, ml, count,
                                    goodCount, from, to);
                } else {
                    goodMoveHandler(makePromote<status>, ml, count, goodCount,
                                    from, to);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    goodMoveHandler(promote<status, depth>, ml, count,
                                    goodCount, from, to);
                } else {
                    goodMoveHandler(makePromote<status>, ml, count, goodCount,
                                    from, to);
                }
            }
        }
        Bitloop(forward) {
            const int to = __builtin_ctzll(forward);
            if constexpr (status.IsWhite) {
                const int from = to - 8;
                if constexpr (search) {
                    moveHandler(pawnMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makePawnMove<status>, ml, count, from, to);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    moveHandler(pawnMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makePawnMove<status>, ml, count, from, to);
                }
            }
        }
        uint64_t doubleForwardNotPinned =
            pawnDoubleForward<status.IsWhite>(notPinned, brd) & chessMask;
        uint64_t doubleForwardPinned =
            pawnDoubleForward<status.IsWhite>(pinnedHV, brd) & chessMask &
            pinHV;
        uint64_t doubleForward = doubleForwardNotPinned | doubleForwardPinned;
        Bitloop(doubleForward) {
            const int to = __builtin_ctzll(doubleForward);
            if constexpr (status.IsWhite) {
                const int from = to - 16;
                if constexpr (search) {
                    moveHandler(pawnDoubleMove<status, depth>, ml, count, from,
                                to);
                } else {
                    moveHandler(makePawnDoubleMove<status>, ml, count, from,
                                to);
                }
            } else {
                const int from = to + 16;
                if constexpr (search) {
                    moveHandler(pawnDoubleMove<status, depth>, ml, count, from,
                                to);
                } else {
                    moveHandler(makePawnDoubleMove<status>, ml, count, from,
                                to);
                }
            }
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
            if constexpr (search) {
                goodMoveHandler(promoteCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePromote<status>, ml, count, goodCount, from,
                                to);
            }

        } else {
            const int from = to + 9;
            if constexpr (search) {
                goodMoveHandler(promoteCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePromote<status>, ml, count, goodCount, from,
                                to);
            }
        }
    }
    Bitloop(left) {
        const int to = __builtin_ctzll(left);
        if constexpr (status.IsWhite) {
            const int from = to - 7;
            if constexpr (search) {
                goodMoveHandler(pawnCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePawnCapture<status>, ml, count, goodCount,
                                from, to);
            }
        } else {
            const int from = to + 9;
            if constexpr (search) {
                goodMoveHandler(pawnCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePawnCapture<status>, ml, count, goodCount,
                                from, to);
            }
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
            if constexpr (search) {
                goodMoveHandler(promoteCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePromoteCapture<status>, ml, count,
                                goodCount, from, to);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                goodMoveHandler(promoteCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePromoteCapture<status>, ml, count,
                                goodCount, from, to);
            }
        }
    }
    Bitloop(right) {
        const int to = __builtin_ctzll(right);
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            if constexpr (search) {
                goodMoveHandler(pawnCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePawnCapture<status>, ml, count, goodCount,
                                from, to);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                goodMoveHandler(pawnCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makePawnCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void knightMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD, Callback *ml,
                              int &count, int &goodCount) noexcept {
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(knightMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeKnightMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(knightCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeKnightCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void bishopMoves(const Board &brd, uint64_t chessMask,
                              uint64_t pinHV, uint64_t pinD, Callback *ml,
                              int &count, int &goodCount) noexcept {
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
        if constexpr (!capturesOnly) {
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(bishopMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeBishopMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(bishopCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeBishopCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
    Bitloop(bishopsPinnedD) {
        const int from = __builtin_ctzll(bishopsPinnedD);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinD & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(bishopMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeBishopMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(bishopCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeBishopCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void queenMoves(const Board &brd, uint64_t chessMask,
                             uint64_t pinHV, uint64_t pinD, Callback *ml,
                             int &count, int &goodCount) noexcept {
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(queenMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(queenCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeQueenCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
    Bitloop(queenPinnedD) {
        const int from = __builtin_ctzll(queenPinnedD);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinD & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(queenMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(queenCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeQueenCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
    Bitloop(queenPinnedHV) {
        const int from = __builtin_ctzll(queenPinnedHV);
        uint64_t attacks = getQmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinHV & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(queenMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(queenCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeQueenCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void rookMoves(const Board &brd, uint64_t chessMask,
                            uint64_t pinHV, uint64_t pinD, Callback *ml,
                            int &count, int &goodCount) noexcept {
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(rookMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeRookMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(rookCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeRookCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
    Bitloop(rooksPinnedHV) {
        const int from = __builtin_ctzll(rooksPinnedHV);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinHV & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(rookMove<status, depth>, ml, count, from, to);
                } else {
                    moveHandler(makeRookMove<status>, ml, count, from, to);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            if constexpr (search) {
                goodMoveHandler(rookCapture<status, depth>, ml, count,
                                goodCount, from, to);
            } else {
                goodMoveHandler(makeRookCapture<status>, ml, count, goodCount,
                                from, to);
            }
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast static void kingMoves(const Board &brd, uint64_t chessMask,
                            uint64_t kingBan, int kingPos, Callback *ml,
                            int &count, int &goodCount) noexcept {
    uint64_t moves =
        kingMasks[kingPos] & ~kingBan & enemyOrEmpty<status.IsWhite>(brd);
    uint64_t captures = moves & brd.Occ;
    if constexpr (!capturesOnly) {
        moves = moves & ~brd.Occ;
        Bitloop(moves) {
            const int to = __builtin_ctzll(moves);
            if constexpr (search) {
                moveHandler(kingMove<status, depth>, ml, count, kingPos, to);
            } else {
                moveHandler(makeKingMove<status>, ml, count, kingPos, to);
            }
        }
    }
    Bitloop(captures) {
        const int to = __builtin_ctzll(captures);
        if constexpr (search) {
            goodMoveHandler(kingCapture<status, depth>, ml, count, goodCount,
                            kingPos, to);
        } else {
            goodMoveHandler(makeKingCapture<status>, ml, count, goodCount,
                            kingPos, to);
        }
    }
}

template <class BoardState status, int depth, bool search>
_fast void castels(const Board &brd, uint64_t kingBan, Callback *ml, int &count,
                   int &goodCount) noexcept {
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if ((!(WNotOccupiedL & brd.Occ)) && (!(WNotAttackedL & kingBan)) &&
                (brd.WRook & WRookL)) {
                if constexpr (search) {
                    goodMoveHandler(leftCastel<status, depth>, ml, count,
                                    goodCount, 4, 2);
                } else {
                    goodMoveHandler(makeLeftCastel<status>, ml, count,
                                    goodCount, 4, 2);
                }
            }
        }
        if constexpr (status.WRC) {
            if ((!(WNotOccupiedR & brd.Occ)) && (!(WNotAttackedR & kingBan)) &&
                (brd.WRook & WRookR)) {
                if constexpr (search) {
                    goodMoveHandler(rightCastel<status, depth>, ml, count,
                                    goodCount, 4, 6);
                } else {
                    goodMoveHandler(makeRightCastel<status>, ml, count,
                                    goodCount, 4, 6);
                }
            }
        }
    } else {
        if constexpr (status.BLC) {
            if ((!(BNotOccupiedL & brd.Occ)) && (!(BNotAttackedL & kingBan)) &&
                (brd.BRook & BRookL)) {
                if constexpr (search) {
                    goodMoveHandler(leftCastel<status, depth>, ml, count,
                                    goodCount, 60, 58);
                } else {
                    goodMoveHandler(makeLeftCastel<status>, ml, count,
                                    goodCount, 60, 58);
                }
            }
        }
        if constexpr (status.BRC) {
            if ((!(BNotOccupiedR & brd.Occ)) && (!(BNotAttackedR & kingBan)) &&
                (brd.BRook & BRookR)) {
                if constexpr (search) {
                    goodMoveHandler(rightCastel<status, depth>, ml, count,
                                    goodCount, 60, 62);
                } else {
                    goodMoveHandler(makeRightCastel<status>, ml, count,
                                    goodCount, 60, 62);
                }
            }
        }
    }
}

template <class BoardState status, int depth, bool search>
_fast void EPMoves(const Board &brd, int ep, Callback *ml, int &count,
                   int &goodCount, uint64_t pinD, uint64_t pinHV) noexcept {
    uint64_t EPRight, EPLeft, EPLeftPinned, EPRightPinned;
    uint64_t EPSquare = 1ull << ep;
    if constexpr (status.IsWhite) {
        EPLeft = (((brd.WPawn & ~(pinHV | pinD)) & ~File1) << 9) & EPSquare;
        EPRight = (((brd.WPawn & ~(pinHV | pinD)) & ~File8) << 7) & EPSquare;
        EPLeftPinned = (((brd.WPawn & pinD) & ~File1) << 9) & EPSquare & pinD;
        EPRightPinned = (((brd.WPawn & pinD) & ~File8) << 7) & EPSquare & pinD;
    } else {
        EPLeft = (((brd.BPawn & ~(pinHV | pinD)) & ~File1) >> 7) & EPSquare;
        EPRight = (((brd.BPawn & ~(pinHV | pinD)) & ~File8) >> 9) & EPSquare;
        EPLeftPinned = (((brd.BPawn & pinD) & ~File1) >> 7) & EPSquare & pinD;
        EPRightPinned = (((brd.BPawn & pinD) & ~File8) >> 9) & EPSquare & pinD;
    }
    if (EPRight) {
        const int to = __builtin_ctzll(EPRight);
        const int from = to + (status.IsWhite ? -7 : 9);
        if constexpr (search) {
            moveHandler(EP<status, depth>, ml, count, from, to);
        } else {
            moveHandler(makeEP<status>, ml, count, from, to);
        }
    }
    if (EPLeft) {
        const int to = __builtin_ctzll(EPLeft);
        const int from = to + (status.IsWhite ? -9 : 7);
        if constexpr (search) {
            moveHandler(EP<status, depth>, ml, count, from, to);
        } else {
            moveHandler(makeEP<status>, ml, count, from, to);
        }
    }
}

template <class BoardState status, int depth, bool search, bool capturesOnly>
_fast uint64_t genMoves(const Board &brd, int ep, Callback *ml, int &count,
                        int &goodCount) noexcept {
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

    pawnMoves<status, depth, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                   ml, count, goodCount);
    knightMoves<status, depth, search, capturesOnly>(
        brd, checkmask, pinHV, pinD, ml, count, goodCount);
    bishopMoves<status, depth, search, capturesOnly>(
        brd, checkmask, pinHV, pinD, ml, count, goodCount);
    queenMoves<status, depth, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                    ml, count, goodCount);
    rookMoves<status, depth, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                   ml, count, goodCount);
    kingMoves<status, depth, search, capturesOnly>(
        brd, checkmask, kingBan, kingPos, ml, count, goodCount);
    if constexpr ((status.WLC || status.WRC || status.BLC || status.BRC) &&
                  (!capturesOnly)) {
        castels<status, depth, search>(brd, kingBan, ml, count, goodCount);
    }
    if constexpr ((status.EP) && (!capturesOnly)) {
        EPMoves<status, depth, search>(brd, ep, ml, count, goodCount, pinD,
                                       pinHV);
    }
    return kingBan;
}

template <int depth, bool search, bool capturesOnly>
_fast uint64_t moveGenCall(const Board &brd, int ep, Callback *ml, int &count,
                           int &goodCount, bool WH, bool EP, bool WL, bool WR,
                           bool BL, bool BR) noexcept {
    if (WH) {
        if (EP) {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            }
        } else {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, true, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            }
        }
    } else {
        if (EP) {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, true, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            }
        } else {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, true, true,
                                                       true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true, true,
                                                       false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, true, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, true, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, false, true},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, false, false},
                                            depth, search, capturesOnly>(
                                brd, ep, ml, count, goodCount);
                    }
                }
            }
        }
    }
}
