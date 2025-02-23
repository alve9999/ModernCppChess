#pragma once
#include <cstdint>
#include <stdint.h>
#include <cmath>
#include <array>
#include "constants.hpp"

extern const std::array<uint64_t,64> r_mask;
extern const std::array<uint64_t, 64> b_mask;


extern const std::array<std::array<uint64_t, 4096>, 64> r_indexs;
extern const std::array<std::array<uint64_t, 512>, 64> b_indexs;

__inline uint64_t getRmagic(int square,uint64_t occ) noexcept {
    occ &= r_mask[square];
    occ *= Rmagics[square];
    occ >>= (64ULL - RBn[square]);
    return r_indexs[square][occ];
}

__inline uint64_t getBmagic(int square,uint64_t occ) noexcept {
    uint64_t occCopy= occ;
    occ &= b_mask[square];
    occ *= Bmagics[square];
    occ >>= (64ULL - BBn[square]);
    return b_indexs[square][occ];
}

__inline uint64_t getQmagic(int square,uint64_t occ) noexcept {
    return getRmagic(square,occ) | getBmagic(square,occ);
}
