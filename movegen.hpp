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
                               uint64_t,int);
using MakeMoveFunc = MoveResult (*)(const Board &, int, int);
inline std::string converter(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row);

    return std::string(1, file) + std::string(1, rank);
}

inline void moveHandler(const MakeMoveFunc &move, Callback *ml, int &count,
                        int from, int to, int value) noexcept {
    Callback &cb = ml[count++];
    cb.makeMove = move;
    cb.from = from;
    cb.to = to;
    cb.value = value;
}

_fast void moveHandler(const SearchMoveFunc &move, Callback *ml, int &count,
                       int from, int to, int value) noexcept {
    Callback &cb = ml[count++];
    cb.move = move;
    cb.from = from;
    cb.to = to;
    cb.value = value;
}

static int values[6] = {1, 3, 3, 5, 9, 0};
#define PROMOTE 9
#define CASTLE 0
#define EP_VAL 1
#define CAPTURE 7

template <bool IsWhite>
constexpr inline int getVictimValue(const Board &brd, int to) noexcept {
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
    }
    return values[capturedPiece];
}

template <bool IsWhite>
int captureValue(const Board &brd,int to, int p) noexcept {
    int value = -values[(int)p] + CAPTURE;
    value += getVictimValue<IsWhite>(brd, to);

    return value;
}

template <bool IsWhite> _fast uint64_t enemyOrEmpty(const Board &brd) noexcept {
    if constexpr (IsWhite) {
        return brd.Black | ~brd.Occ;
    }
    return brd.White | ~brd.Occ;
}

template <class BoardState status, bool search, bool capturesOnly>
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
                    moveHandler(promote<status>, ml, count,
                                     from, to, PROMOTE);
                } else {
                    moveHandler(makePromote<status>, ml, count, 
                                    from, to, PROMOTE);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    moveHandler(promote<status>, ml, count,
                                     from, to, PROMOTE);
                } else {
                    moveHandler(makePromote<status>, ml, count, 
                                    from, to, PROMOTE);
                }
            }
        }
        Bitloop(forward) {
            const int to = __builtin_ctzll(forward);
            if constexpr (status.IsWhite) {
                const int from = to - 8;
                if constexpr (search) {
                    moveHandler(pawnMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makePawnMove<status>, ml, count, from, to, 0);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    moveHandler(pawnMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makePawnMove<status>, ml, count, from, to, 0);
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
                    moveHandler(pawnDoubleMove<status>, ml, count, from,
                                to, 0);
                } else {
                    moveHandler(makePawnDoubleMove<status>, ml, count, from,
                                to, 0);
                }
            } else {
                const int from = to + 16;
                if constexpr (search) {
                    moveHandler(pawnDoubleMove<status>, ml, count, from,
                                to, 0);
                } else {
                    moveHandler(makePawnDoubleMove<status>, ml, count, from,
                                to, 0);
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
        const int value = captureValue<status.IsWhite>(brd, to, 0) + PROMOTE;
        if constexpr (status.IsWhite) {
            const int from = to - 7;
            if constexpr (search) {
                moveHandler(promoteCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePromote<status>, ml, count,  from,
                                to, value);
            }

        } else {
            const int from = to + 9;
            if constexpr (search) {
                moveHandler(promoteCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePromote<status>, ml, count,  from,
                                to, value);
            }
        }
    }
    Bitloop(left) {
        const int to = __builtin_ctzll(left);
        const int value = captureValue<status.IsWhite>(brd, to, 0);
        if constexpr (status.IsWhite) {
            const int from = to - 7;
            if constexpr (search) {
                moveHandler(pawnCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePawnCapture<status>, ml, count, 
                                from, to, value);
            }
        } else {
            const int from = to + 9;
            if constexpr (search) {
                moveHandler(pawnCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePawnCapture<status>, ml, count, 
                                from, to, value);
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
        const int value = captureValue<status.IsWhite>(brd, to, 0) + PROMOTE;
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            if constexpr (search) {
                moveHandler(promoteCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePromoteCapture<status>, ml, count,
                                 from, to, value);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                moveHandler(promoteCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePromoteCapture<status>, ml, count,
                                 from, to, value);
            }
        }
    }
    Bitloop(right) {
        const int to = __builtin_ctzll(right);
        const int value = captureValue<status.IsWhite>(brd, to,0);
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            if constexpr (search) {
                moveHandler(pawnCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePawnCapture<status>, ml, count, 
                                from, to, value);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                moveHandler(pawnCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makePawnCapture<status>, ml, count, 
                                from, to, value);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(knightMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeKnightMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 1);
            if constexpr (search) {
                moveHandler(knightCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeKnightCapture<status>, ml, count, 
                                from, to, value);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
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
        if constexpr (!capturesOnly) {
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(bishopMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeBishopMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 2);
            if constexpr (search) {
                moveHandler(bishopCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeBishopCapture<status>, ml, count, 
                                from, to, value);
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
                    moveHandler(bishopMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeBishopMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 2);
            if constexpr (search) {
                moveHandler(bishopCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeBishopCapture<status>, ml, count, 
                                from, to, value);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(queenMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandler(queenCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeQueenCapture<status>, ml, count, 
                                from, to, value);
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
                    moveHandler(queenMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandler(queenCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeQueenCapture<status>, ml, count, 
                                from, to, value);
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
                    moveHandler(queenMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeQueenMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandler(queenCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeQueenCapture<status>, ml, count, 
                                from, to, value);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandler(rookMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeRookMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 3);
            if constexpr (search) {
                moveHandler(rookCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeRookCapture<status>, ml, count, 
                                from, to, value);
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
                    moveHandler(rookMove<status>, ml, count, from, to, 0);
                } else {
                    moveHandler(makeRookMove<status>, ml, count, from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 3);
            if constexpr (search) {
                moveHandler(rookCapture<status,capturesOnly>, ml, count,
                                 from, to, value);
            } else {
                moveHandler(makeRookCapture<status>, ml, count, 
                                from, to, value);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
_fast static void kingMoves(const Board &brd, uint64_t chessMask,
                            uint64_t kingBan, int kingPos, Callback *ml,
                            int &count) noexcept {
    uint64_t moves =
        kingMasks[kingPos] & ~kingBan & enemyOrEmpty<status.IsWhite>(brd);
    uint64_t captures = moves & brd.Occ;
    if constexpr (!capturesOnly) {
        moves = moves & ~brd.Occ;
        Bitloop(moves) {
            const int to = __builtin_ctzll(moves);
            if constexpr (search) {
                moveHandler(kingMove<status>, ml, count, kingPos, to, 0);
            } else {
                moveHandler(makeKingMove<status>, ml, count, kingPos, to, 0);
            }
        }
    }
    Bitloop(captures) {
        const int to = __builtin_ctzll(captures);
        const int value = captureValue<status.IsWhite>(brd, to, 5);
        if constexpr (search) {
            moveHandler(kingCapture<status,capturesOnly>, ml, count, 
                            kingPos, to, value);
        } else {
            moveHandler(makeKingCapture<status>, ml, count, 
                            kingPos, to, value);
        }
    }
}

template <class BoardState status, bool search>
_fast void castels(const Board &brd, uint64_t kingBan, Callback *ml, int &count) noexcept {
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if ((!(WNotOccupiedL & brd.Occ)) && (!(WNotAttackedL & kingBan)) &&
                (brd.WRook & WRookL)) {
                if constexpr (search) {
                    moveHandler(leftCastel<status>, ml, count,
                                     4, 2, CASTLE);
                } else {
                    moveHandler(makeLeftCastel<status>, ml, count,
                                     4, 2, CASTLE);
                }
            }
        }
        if constexpr (status.WRC) {
            if ((!(WNotOccupiedR & brd.Occ)) && (!(WNotAttackedR & kingBan)) &&
                (brd.WRook & WRookR)) {
                if constexpr (search) {
                    moveHandler(rightCastel<status>, ml, count,
                                     4, 6, CASTLE);
                } else {
                    moveHandler(makeRightCastel<status>, ml, count,
                                     4, 6, CASTLE);
                }
            }
        }
    } else {
        if constexpr (status.BLC) {
            if ((!(BNotOccupiedL & brd.Occ)) && (!(BNotAttackedL & kingBan)) &&
                (brd.BRook & BRookL)) {
                if constexpr (search) {
                    moveHandler(leftCastel<status>, ml, count,
                                     60, 58, CASTLE);
                } else {
                    moveHandler(makeLeftCastel<status>, ml, count,
                                     60, 58, CASTLE);
                }
            }
        }
        if constexpr (status.BRC) {
            if ((!(BNotOccupiedR & brd.Occ)) && (!(BNotAttackedR & kingBan)) &&
                (brd.BRook & BRookR)) {
                if constexpr (search) {
                    moveHandler(rightCastel<status>, ml, count,
                                     60, 62, CASTLE);
                } else {
                    moveHandler(makeRightCastel<status>, ml, count,
                                     60, 62, CASTLE);
                }
            }
        }
    }
}

template <class BoardState status, bool search>
_fast void EPMoves(const Board &brd, int ep, Callback *ml, int &count,
                   uint64_t pinD, uint64_t pinHV) noexcept {
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
            moveHandler(EP<status>, ml, count, from, to, EP_VAL);
        } else {
            moveHandler(makeEP<status>, ml, count, from, to, EP_VAL);
        }
    }
    if (EPLeft) {
        const int to = __builtin_ctzll(EPLeft);
        const int from = to + (status.IsWhite ? -9 : 7);
        if constexpr (search) {
            moveHandler(EP<status>, ml, count, from, to, EP_VAL);
        } else {
            moveHandler(makeEP<status>, ml, count, from, to, EP_VAL);
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
_fast uint64_t genMoves(const Board &brd, int ep, Callback *ml, int &count) noexcept {
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

    pawnMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                   ml, count);
    knightMoves<status, search, capturesOnly>(
        brd, checkmask, pinHV, pinD, ml, count);
    bishopMoves<status, search, capturesOnly>(
        brd, checkmask, pinHV, pinD, ml, count);
    queenMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                    ml, count);
    rookMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD,
                                                   ml, count);
    kingMoves<status, search, capturesOnly>(
        brd, checkmask, kingBan, kingPos, ml, count);
    if constexpr ((status.WLC || status.WRC || status.BLC || status.BRC) &&
                  (!capturesOnly)) {
        castels<status, search>(brd, kingBan, ml, count);
    }
    if constexpr ((status.EP) && (!capturesOnly)) {
        EPMoves<status, search>(brd, ep, ml, count,  pinD,
                                       pinHV);
    }
    return kingBan;
}

template <bool search, bool capturesOnly>
_fast uint64_t moveGenCall(const Board &brd, int ep, Callback *ml, int &count,
                           bool WH, bool EP, bool WL, bool WR,
                           bool BL, bool BR) noexcept {
    if (WH) {
        if (EP) {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
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
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
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
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
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
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true, true,
                                                       false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, true, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, true, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, false, true},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, false, false},
                                            search, capturesOnly>(
                                brd, ep, ml, count);
                    }
                }
            }
        }
    }
}
