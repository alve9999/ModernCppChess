#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "pawns.hpp"
#include "sliding.hpp"
#include <cassert>
#include <cstdint>
template <bool IsWhite>
_fast static void pawnCheckmask(const Board &brd,
                                uint64_t &checkmask) noexcept {
    if constexpr (IsWhite) {
        const uint64_t left = ((brd.WKing << 7) & (~File1)) & brd.BPawn;
        const uint64_t right = ((brd.WKing << 9) & (~File8)) & brd.BPawn;
        checkmask &=
            ((!(static_cast<bool>(left | right)) * ONES) | (left | right));
    } else {
        const uint64_t left = ((brd.BKing >> 9) & (~File1)) & brd.WPawn;
        const uint64_t right = ((brd.BKing >> 7) & (~File8)) & brd.WPawn;
        checkmask &=
            (((!static_cast<bool>(left | right)) * ONES) | (left | right));
    }
}

template <bool IsWhite>
_fast static void knightCheckmask(const Board &brd, uint64_t &checkmask,
                                  int kingPos) noexcept {
    if constexpr (IsWhite) {
        const uint64_t knight = knightMasks[kingPos] & brd.BKnight;
        checkmask &= (!static_cast<bool>(knight)) * ONES | knight;
    } else {
        const uint64_t knight = knightMasks[kingPos] & brd.WKnight;
        checkmask &= (!static_cast<bool>(knight)) * ONES | knight;
    }
}

inline int flipX(int sq) noexcept {
    int rank = sq / 8;
    int file = sq % 8;
    int flipped_file = 7 - file;
    return rank * 8 + flipped_file;
}
template <bool IsWhite>
_fast static void slidingPieceCheckmask(const Board &brd, uint64_t &pinHV,
                                        uint64_t &pinD, uint64_t &checkmask,
                                        int kingPos) noexcept {
    if constexpr (IsWhite) {
        uint64_t sliding = 0;
        if (RookMask[kingPos] & (brd.BRook | brd.BQueen)) {
            const uint64_t attacks = getRmagic(kingPos, brd.Occ);
            sliding |= attacks & (brd.BQueen | brd.BRook);
            uint64_t xRay = getRmagic(kingPos, brd.Occ ^ (attacks & brd.Occ)) &
                            (brd.BRook | brd.BQueen);
            Bitloop(xRay) {
                const int xRayPos = __builtin_ctzll(xRay);
                pinHV |= kingPath[64 * kingPos + xRayPos];
            }
        }
        if (BishopMask[kingPos] & (brd.BBishop | brd.BQueen)) {
            const uint64_t attacks = getBmagic(kingPos, brd.Occ);
            sliding |= attacks & (brd.BQueen | brd.BBishop);
            uint64_t skewer =
                getBmagic(kingPos, brd.Occ ^ (attacks & brd.Occ)) &
                (brd.BBishop | brd.BQueen);
            Bitloop(skewer) {
                const int skewerPos = __builtin_ctzll(skewer);
                pinD |= kingPath[64 * kingPos + skewerPos];
            }
        }
        const int slidingCount = __builtin_popcountll(sliding);
        if (slidingCount >= 2) {
            checkmask = 0;
        }
        else{
            const uint64_t path = kingPath[64 * kingPos + __builtin_ctzll(sliding)];
            checkmask &= (!static_cast<bool>(sliding)) * ONES | path;
        }
    } else {
        uint64_t sliding = 0;
        if (RookMask[kingPos] & (brd.WRook | brd.WQueen)) {
            uint64_t attacks = getRmagic(kingPos, brd.Occ);
            sliding |= attacks & (brd.WQueen | brd.WRook);
            uint64_t xRay = getRmagic(kingPos, brd.Occ ^ (attacks & brd.Occ)) &
                            (brd.WRook | brd.WQueen);
            Bitloop(xRay) {
                const int xRayPos = __builtin_ctzll(xRay);
                pinHV |= kingPath[64 * kingPos + xRayPos];
            }
        }
        if (BishopMask[kingPos] & (brd.WBishop | brd.WQueen)) {
            uint64_t attacks = getBmagic(kingPos, brd.Occ);
            sliding |= attacks & (brd.WQueen | brd.WBishop);
            uint64_t skewer =
                getBmagic(kingPos, brd.Occ ^ (attacks & brd.Occ)) &
                (brd.WBishop | brd.WQueen);
            Bitloop(skewer) {
                const int skewerPos = __builtin_ctzll(skewer);
                pinD |= kingPath[64 * kingPos + skewerPos];
            }
        }
        const int slidingCount = __builtin_popcountll(sliding);
        if (slidingCount >= 2) {
            checkmask = 0;
        }
        else{
            const uint64_t path = kingPath[64 * kingPos + __builtin_ctzll(sliding)];
            checkmask &= (!static_cast<bool>(sliding)) * ONES | path;
        }
    }
}

template <bool IsWhite>
_fast void generateKingBan(const Board &brd, uint64_t &kingBan) noexcept {
    uint64_t pawns, queens, bishops, rooks, knights, kingMask;
    int kingPos;
    if constexpr (IsWhite) {
        pawns = brd.BPawn;
        queens = brd.BQueen;
        bishops = brd.BBishop;
        rooks = brd.BRook;
        knights = brd.BKnight;
        kingMask = brd.WKing;
        kingPos = __builtin_ctzll(brd.BKing);
    } else {
        pawns = brd.WPawn;
        queens = brd.WQueen;
        bishops = brd.WBishop;
        rooks = brd.WRook;
        knights = brd.WKnight;
        kingMask = brd.BKing;
        kingPos = __builtin_ctzll(brd.WKing);
    }
    kingBan |= kingMasks[kingPos];
    kingBan |= pawnCouldAttackLeft<!IsWhite>(pawns);
    kingBan |= pawnCouldAttackRight<!IsWhite>(pawns);
    Bitloop(knights) { kingBan |= knightMasks[__builtin_ctzll(knights)]; }
    Bitloop(queens) { kingBan |= getQmagic(__builtin_ctzll(queens), brd.Occ^kingMask); }
    Bitloop(bishops) {
        kingBan |= getBmagic(__builtin_ctzll(bishops), brd.Occ^kingMask);
    }
    Bitloop(rooks) { kingBan |= getRmagic(__builtin_ctzll(rooks), brd.Occ^kingMask); }
}

template <bool IsWhite>
_fast void generateCheck(const Board &brd, int kingPos, uint64_t &pinHV,
                         uint64_t &pinD, uint64_t &checkmask,
                         uint64_t &kingBan) noexcept {
    slidingPieceCheckmask<IsWhite>(brd, pinHV, pinD, checkmask, kingPos);
    pawnCheckmask<IsWhite>(brd, checkmask);
    knightCheckmask<IsWhite>(brd, checkmask, kingPos);
    generateKingBan<IsWhite>(brd, kingBan);
}
