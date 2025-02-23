#pragma once
#include <cstdint>
#include "board.hpp"
#include "constants.hpp"



template <bool IsWhite>
_fast uint64_t pawnForward(const uint64_t pawns, const Board& brd) noexcept {
    if constexpr (IsWhite){
        return (pawns<<8)&(~brd.Occ);
    }
    return (pawns>>8)&(~brd.Occ);
}

template <bool IsWhite>
_fast uint64_t pawnDoubleForward(const uint64_t pawns, const Board& brd) noexcept{
    if constexpr (IsWhite){
        return ((((pawns & Rank2) << 8) & (~brd.Occ)) << 8) & (~brd.Occ);
    }
    return ((((pawns & Rank7) >> 8) & (~brd.Occ)) >> 8) & (~brd.Occ);
}

template<bool IsWhite>
_fast uint64_t enemy(const Board& brd) noexcept {
    if constexpr (IsWhite) return brd.Black;
    return brd.White;
}


template <bool IsWhite>
_fast uint64_t pawnCouldAttackLeft(const uint64_t pawns) noexcept {
    if constexpr (IsWhite){
        return ((pawns & ~File8)<<7);
    }
    return ((pawns & ~File8)>>9);
}


template <bool IsWhite>
_fast uint64_t pawnCouldAttackRight(const uint64_t pawns) noexcept {
    if constexpr (IsWhite){
        return ((pawns & ~File1)<<9);
    }
    return ((pawns & ~File1)>>7);
}


template <bool IsWhite>
_fast uint64_t pawnAttackLeft(const uint64_t pawns, const Board& brd) noexcept {
    if constexpr (IsWhite){
        return ((pawns & ~File8)<<7) & enemy<IsWhite>(brd);
    }
    return ((pawns & ~File8)>>9) & enemy<IsWhite>(brd);
}


template <bool IsWhite>
_fast uint64_t pawnAttackRight(const uint64_t pawns, const Board& brd) noexcept {
    if constexpr (IsWhite){
        return ((pawns & ~File1)<<9) & enemy<IsWhite>(brd);
    }
    return ((pawns & ~File1)>>7) & enemy<IsWhite>(brd);
}

_fast uint64_t promotion(uint64_t bitboard) noexcept {
    return bitboard&Rank_18;
}
