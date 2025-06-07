#pragma once
#include "board.hpp"
#include "check.hpp"
#include "constants.hpp"
#include "move.hpp"
#include "pawns.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>

extern int historyTable[2][64][64];

using SearchMoveFunc = int (*)(const Board &, move_info_t&);

using MakeMoveFunc = MoveResult (*)(const Board &, int, int);
inline std::string converter(int index) {
    int row = index / 8;
    int col = index % 8;

    char file = 'a' + col;

    char rank = '1' + (row);

    return std::string(1, file) + std::string(1, rank);
}

template <class BoardState status>
inline void moveHandle(const Board &brd, const MakeMoveFunc &move, Callback *ml,
                       int &count, int from, int to, int value) noexcept {
    Callback &cb = ml[count++];
    cb.makeMove = move;
    cb.from = from;
    cb.to = to;
    cb.value = value;
}

template <class BoardState status>
_fast void moveHandle(const Board &brd, const SearchMoveFunc &move,
                      Callback *ml, int &count, int from, int to, int value,
                      bool capture, bool promotions) noexcept {

    if ((brd.BKing & (1ULL << to))) {
        assert(false);
    }

    if ((brd.WKing & (1ULL << to))) {
        assert(false);
    }
    Callback &cb = ml[count++];
    cb.move = move;
    cb.from = from;
    cb.to = to;
    cb.capture = capture;
    cb.promotion = promotions;
    int history = historyTable[status.IsWhite][from][to];
    cb.value = value + history;
}

static int values[6] = {10000, 30000, 30000, 50000, 90000, 0};
#define PROMOTE 80000
#define CASTLE 5000
#define EP_VAL 10000
#define CAPTURE 70000

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
int captureValue(const Board &brd, int to, int p) noexcept {
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
                    moveHandle<status>(brd, promote<status>, ml, count, from,
                                       to, PROMOTE, 0, 1);
                } else {
                    moveHandle<status>(brd, makePromote<status>, ml, count,
                                       from, to, PROMOTE);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    moveHandle<status>(brd, promote<status>, ml, count, from,
                                       to, PROMOTE, 0, 1);
                } else {
                    moveHandle<status>(brd, makePromote<status>, ml, count,
                                       from, to, PROMOTE);
                }
            }
        }
        Bitloop(forward) {
            const int to = __builtin_ctzll(forward);
            if constexpr (status.IsWhite) {
                const int from = to - 8;
                if constexpr (search) {
                    moveHandle<status>(brd, pawnMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makePawnMove<status>, ml, count,
                                       from, to, 0);
                }
            } else {
                const int from = to + 8;
                if constexpr (search) {
                    moveHandle<status>(brd, pawnMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makePawnMove<status>, ml, count,
                                       from, to, 0);
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
                    moveHandle<status>(brd, pawnDoubleMove<status>, ml, count,
                                       from, to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makePawnDoubleMove<status>, ml,
                                       count, from, to, 0);
                }
            } else {
                const int from = to + 16;
                if constexpr (search) {
                    moveHandle<status>(brd, pawnDoubleMove<status>, ml, count,
                                       from, to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makePawnDoubleMove<status>, ml,
                                       count, from, to, 0);
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
                moveHandle<status>(brd, promoteCapture<status, capturesOnly>,
                                   ml, count, from, to, value, 1, 1);
            } else {
                moveHandle<status>(brd, makePromote<status>, ml, count, from,
                                   to, value);
            }

        } else {
            const int from = to + 9;
            if constexpr (search) {
                moveHandle<status>(brd, promoteCapture<status, capturesOnly>,
                                   ml, count, from, to, value, 1, 1);
            } else {
                moveHandle<status>(brd, makePromote<status>, ml, count, from,
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
                moveHandle<status>(brd, pawnCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makePawnCapture<status>, ml, count,
                                   from, to, value);
            }
        } else {
            const int from = to + 9;
            if constexpr (search) {
                moveHandle<status>(brd, pawnCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makePawnCapture<status>, ml, count,
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
                moveHandle<status>(brd, promoteCapture<status, capturesOnly>,
                                   ml, count, from, to, value, 1, 1);
            } else {
                moveHandle<status>(brd, makePromoteCapture<status>, ml, count,
                                   from, to, value);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                moveHandle<status>(brd, promoteCapture<status, capturesOnly>,
                                   ml, count, from, to, value, 1, 1);
            } else {
                moveHandle<status>(brd, makePromoteCapture<status>, ml, count,
                                   from, to, value);
            }
        }
    }
    Bitloop(right) {
        const int to = __builtin_ctzll(right);
        const int value = captureValue<status.IsWhite>(brd, to, 0);
        if constexpr (status.IsWhite) {
            const int from = to - 9;
            if constexpr (search) {
                moveHandle<status>(brd, pawnCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makePawnCapture<status>, ml, count,
                                   from, to, value);
            }
        } else {
            const int from = to + 7;
            if constexpr (search) {
                moveHandle<status>(brd, pawnCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makePawnCapture<status>, ml, count,
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
                    moveHandle<status>(brd, knightMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeKnightMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 1);
            if constexpr (search) {
                moveHandle<status>(brd, knightCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeKnightCapture<status>, ml, count,
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
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandle<status>(brd, bishopMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeBishopMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 2);
            if constexpr (search) {
                moveHandle<status>(brd, bishopCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeBishopCapture<status>, ml, count,
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
                    moveHandle<status>(brd, bishopMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeBishopMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 2);
            if constexpr (search) {
                moveHandle<status>(brd, bishopCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeBishopCapture<status>, ml, count,
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
                    moveHandle<status>(brd, queenMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeQueenMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandle<status>(brd, queenCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeQueenCapture<status>, ml, count,
                                   from, to, value);
            }
        }
    }
    Bitloop(queenPinnedD) {
        const int from = __builtin_ctzll(queenPinnedD);
        uint64_t attacks = getBmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinD & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandle<status>(brd, queenMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeQueenMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandle<status>(brd, queenCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeQueenCapture<status>, ml, count,
                                   from, to, value);
            }
        }
    }
    Bitloop(queenPinnedHV) {
        const int from = __builtin_ctzll(queenPinnedHV);
        uint64_t attacks = getRmagic(from, brd.Occ);
        attacks =
            attacks & chessMask & pinHV & enemyOrEmpty<status.IsWhite>(brd);
        uint64_t captures = attacks & brd.Occ;
        if constexpr (!capturesOnly) {
            attacks = attacks & ~brd.Occ;
            Bitloop(attacks) {
                const int to = __builtin_ctzll(attacks);
                if constexpr (search) {
                    moveHandle<status>(brd, queenMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeQueenMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 4);
            if constexpr (search) {
                moveHandle<status>(brd, queenCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeQueenCapture<status>, ml, count,
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
                    moveHandle<status>(brd, rookMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeRookMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 3);
            if constexpr (search) {
                moveHandle<status>(brd, rookCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeRookCapture<status>, ml, count,
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
                    moveHandle<status>(brd, rookMove<status>, ml, count, from,
                                       to, 0, 0, 0);
                } else {
                    moveHandle<status>(brd, makeRookMove<status>, ml, count,
                                       from, to, 0);
                }
            }
        }
        Bitloop(captures) {
            const int to = __builtin_ctzll(captures);
            const int value = captureValue<status.IsWhite>(brd, to, 3);
            if constexpr (search) {
                moveHandle<status>(brd, rookCapture<status, capturesOnly>, ml,
                                   count, from, to, value, 1, 0);
            } else {
                moveHandle<status>(brd, makeRookCapture<status>, ml, count,
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
                moveHandle<status>(brd, kingMove<status>, ml, count, kingPos,
                                   to, 0, 0, 0);
            } else {
                moveHandle<status>(brd, makeKingMove<status>, ml, count,
                                   kingPos, to, 0);
            }
        }
    }
    Bitloop(captures) {
        const int to = __builtin_ctzll(captures);
        const int value = captureValue<status.IsWhite>(brd, to, 5);
        if constexpr (search) {
            moveHandle<status>(brd, kingCapture<status, capturesOnly>, ml,
                               count, kingPos, to, value, 1, 0);
        } else {
            moveHandle<status>(brd, makeKingCapture<status>, ml, count, kingPos,
                               to, value);
        }
    }
}

template <class BoardState status, bool search>
_fast void castels(const Board &brd, uint64_t kingBan, Callback *ml,
                   int &count) noexcept {
    if constexpr (status.IsWhite) {
        if constexpr (status.WLC) {
            if ((!(WNotOccupiedL & brd.Occ)) && (!(WNotAttackedL & kingBan)) &&
                (brd.WRook & WRookL)) {
                if constexpr (search) {
                    moveHandle<status>(brd, leftCastel<status>, ml, count, 4, 2,
                                       CASTLE, 0, 0);
                } else {
                    moveHandle<status>(brd, makeLeftCastel<status>, ml, count,
                                       4, 2, CASTLE);
                }
            }
        }
        if constexpr (status.WRC) {
            if ((!(WNotOccupiedR & brd.Occ)) && (!(WNotAttackedR & kingBan)) &&
                (brd.WRook & WRookR)) {
                if constexpr (search) {
                    moveHandle<status>(brd, rightCastel<status>, ml, count, 4,
                                       6, CASTLE, 0, 0);
                } else {
                    moveHandle<status>(brd, makeRightCastel<status>, ml, count,
                                       4, 6, CASTLE);
                }
            }
        }
    } else {
        if constexpr (status.BLC) {
            if ((!(BNotOccupiedL & brd.Occ)) && (!(BNotAttackedL & kingBan)) &&
                (brd.BRook & BRookL)) {
                if constexpr (search) {
                    moveHandle<status>(brd, leftCastel<status>, ml, count, 60,
                                       58, CASTLE, 0, 0);
                } else {
                    moveHandle<status>(brd, makeLeftCastel<status>, ml, count,
                                       60, 58, CASTLE);
                }
            }
        }
        if constexpr (status.BRC) {
            if ((!(BNotOccupiedR & brd.Occ)) && (!(BNotAttackedR & kingBan)) &&
                (brd.BRook & BRookR)) {
                if constexpr (search) {
                    moveHandle<status>(brd, rightCastel<status>, ml, count, 60,
                                       62, CASTLE, 0, 0);
                } else {
                    moveHandle<status>(brd, makeRightCastel<status>, ml, count,
                                       60, 62, CASTLE);
                }
            }
        }
    }
}

template <class BoardState status>
_fast bool isEPPinned(const Board &brd, uint64_t kingPos,
                      uint64_t movedPieces) noexcept {
    int kingIndex = __builtin_ctzll(kingPos);
    uint64_t enemies =
        status.IsWhite ? (brd.BRook | brd.BQueen) : (brd.WRook | brd.WQueen);

    uint64_t rankMask = 0xFFULL << (kingIndex & 56);
    uint64_t potentialPinners = enemies & rankMask;

    if (potentialPinners) {
        uint64_t occupancy = brd.Occ & ~movedPieces;

        uint64_t attacks = getRmagic(kingIndex, occupancy);
        if (attacks & potentialPinners) {
            return true;
        }
    }

    return false; // No pin issue
}

template <class BoardState status, bool search>
_fast void EPMoves(const Board &brd, int ep, Callback *ml, int &count,
                   uint64_t pinD, uint64_t pinHV, uint64_t checkmask) noexcept {
    if ((checkmask != ~0ULL) &&
        !(checkmask & (1ULL << (ep + (status.IsWhite ? -8 : 8))))) {
        return;
    }

    if (ep == -1) {
        return;
    }
    uint64_t EPRight, EPLeft, EPLeftPinned, EPRightPinned;
    int capturedPawnPos = ep + (status.IsWhite ? -8 : 8);
    uint64_t capturedPawnBB = 1ull << capturedPawnPos;
    uint64_t EPSquare = 1ull << ep;

    if constexpr (status.IsWhite) {
        EPLeft = (((brd.WPawn & ~(pinHV | pinD)) & ~File1) << 9ULL) & EPSquare;
        EPRight = (((brd.WPawn & ~(pinHV | pinD)) & ~File8) << 7ULL) & EPSquare;
        EPLeftPinned =
            (((brd.WPawn & pinD) & ~File1) << 9ULL) & EPSquare & pinD;
        EPRightPinned =
            (((brd.WPawn & pinD) & ~File8) << 7ULL) & EPSquare & pinD;
    } else {
        EPLeft = (((brd.BPawn & ~(pinHV | pinD)) & ~File1) >> 7ULL) & EPSquare;
        EPRight = (((brd.BPawn & ~(pinHV | pinD)) & ~File8) >> 9ULL) & EPSquare;
        EPLeftPinned =
            (((brd.BPawn & pinD) & ~File1) >> 7ULL) & EPSquare & pinD;
        EPRightPinned =
            (((brd.BPawn & pinD) & ~File8) >> 9ULL) & EPSquare & pinD;
    }
    if (EPRight || EPRightPinned) {
        const int to = __builtin_ctzll(EPRight | EPRightPinned);
        const int from = to + (status.IsWhite ? -7 : 9);

        uint64_t fromBB = 1ull << from;
        uint64_t kingPos = status.IsWhite ? brd.WKing : brd.BKing;
        uint64_t movedPiecesRay = fromBB | EPSquare | capturedPawnBB;

        if (!isEPPinned<status>(brd, kingPos, movedPiecesRay)) {
            if constexpr (search) {
                moveHandle<status>(brd, EP<status>, ml, count, from, to, EP_VAL,
                                   1, 0);
            } else {
                moveHandle<status>(brd, makeEP<status>, ml, count, from, to,
                                   EP_VAL);
            }
        }
    }

    if (EPLeft || EPLeftPinned) {
        const int to = __builtin_ctzll(EPLeft | EPLeftPinned);
        const int from = to + (status.IsWhite ? -9 : 7);

        uint64_t fromBB = 1ull << from;
        uint64_t kingPos = status.IsWhite ? brd.WKing : brd.BKing;
        uint64_t movedPiecesRay = fromBB | EPSquare | capturedPawnBB;

        if (!isEPPinned<status>(brd, kingPos, movedPiecesRay)) {
            if constexpr (search) {
                moveHandle<status>(brd, EP<status>, ml, count, from, to, EP_VAL,
                                   1, 0);
            } else {
                moveHandle<status>(brd, makeEP<status>, ml, count, from, to,
                                   EP_VAL);
            }
        }
    }
}

template <class BoardState status, bool search, bool capturesOnly>
_fast uint64_t genMoves(const Board &brd, int ep, Callback *ml,
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

    pawnMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD, ml,
                                            count);
    knightMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD, ml,
                                              count);
    bishopMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD, ml,
                                              count);
    queenMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD, ml,
                                             count);
    rookMoves<status, search, capturesOnly>(brd, checkmask, pinHV, pinD, ml,
                                            count);
    kingMoves<status, search, capturesOnly>(brd, checkmask, kingBan, kingPos,
                                            ml, count);
    if constexpr ((status.WLC || status.WRC || status.BLC || status.BRC) &&
                  (!capturesOnly)) {
        castels<status, search>(brd, kingBan, ml, count);
    }
    if constexpr ((status.EP) && (!capturesOnly)) {
        EPMoves<status, search>(brd, ep, ml, count, pinD, pinHV, checkmask);
    }
    return kingBan;
}

template <bool search, bool capturesOnly>
_fast uint64_t moveGenCall(const Board &brd, int ep, Callback *ml, int &count,
                           bool WH, bool EP, bool WL, bool WR, bool BL,
                           bool BR) noexcept {
    if (WH) {
        if (EP) {
            if (WL) {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, true, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, true, false,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, true, false,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, false, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, true, false, false,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, true, false, false,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
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
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, true, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, true, false,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, true, false,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, false, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{true, false, false,
                                                       false, false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{true, false, false,
                                                       false, false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
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
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, true, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, true, false,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, true, false,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, false, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, true, false,
                                                       false, false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, true, false,
                                                       false, false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
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
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true, true,
                                                       false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, true, true,
                                                       false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, true,
                                                       false, false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, true,
                                                       false, false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                }
            } else {
                if (WR) {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       true, false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       true, false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                } else {
                    if (BL) {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, true, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, true, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    } else {
                        if (BR)
                            return genMoves<BoardState{false, false, false,
                                                       false, false, true},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                        else
                            return genMoves<BoardState{false, false, false,
                                                       false, false, false},
                                            search, capturesOnly>(brd, ep, ml,
                                                                  count);
                    }
                }
            }
        }
    }
}
