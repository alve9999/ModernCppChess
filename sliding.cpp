#include "sliding.hpp"
#include <cstdint>
#include <stdint.h>
#include <bit>
#include <cmath>
#include <array>
#include "constants.hpp"

constexpr uint64_t set_occupancy(int index, int bits_in_mask, uint64_t attack_mask) {
    uint64_t occupancy = 0;
    for (int count = 0; count < bits_in_mask; count++){
        int square = std::countr_zero(attack_mask);
        attack_mask& (1ULL << square) ? (attack_mask ^= (1ULL << square)) : 0;
        if (index & 1 << count) {
            occupancy |= 1ULL << square;
        }
    }
    return occupancy;
}

constexpr uint64_t r_mask_square(int square) {
    // attack bitboard
    uint64_t attackboard = 0;

    //file, rank, target rank, target file
    int f = 0;
    int r = 0;
    int tr = square / 8;
    int tf = square % 8;

    // generate attacks
    for (r = tr + 1; r <= 6; r++) attackboard |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--) attackboard |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++) attackboard |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--) attackboard |= (1ULL << (tr * 8 + f));

    return attackboard;
}

constexpr uint64_t b_mask_square(int square) {
    // attack bitboard
    uint64_t attackboard = 0;

    //file, rank, target rank, target file
    int f = 0;
    int r = 0;
    int tr = square / 8;
    int tf = square % 8;

    // generate attacks
    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++) attackboard |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--) attackboard |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++) attackboard |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--) attackboard |= (1ULL << (r * 8 + f));

    return attackboard;
}

constexpr std::array<uint64_t, 64> init_rmask() {
    std::array<uint64_t, 64> r_mask{};
    for(int i = 0;i<64;i++){
        r_mask[i]=r_mask_square(i);
    }
    return r_mask;
}

constexpr std::array<uint64_t, 64> init_bmask() {
    std::array<uint64_t, 64> b_mask{};
    for(int i = 0;i<64;i++){
        b_mask[i]=b_mask_square(i);
    }
    return b_mask;
}

constexpr uint64_t BC(uint64_t occ, int square) {
    uint64_t result = 0;

    int f = 0;
    int r = 0;
    int tr = square / 8;
    int tf = square % 8;
    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++) {
        result |= (1ULL << (r * 8 + f));
        if ((occ >> (r * 8 + f))&1) { break; };
    }
    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--) {
        result |= (1ULL << (r * 8 + f));
        if ((occ >> (r * 8 + f))&1) { break; };
    }
    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++) {
        result |= (1ULL << (r * 8 + f));
        if ((occ>> (r * 8 + f))&1) { break; };
    }
    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--) {
        result |= (1ULL << (r * 8 + f));
        if ((occ >> (r * 8 + f))&1) { break; };
    }
    return result;
}

constexpr uint64_t RC(uint64_t occ, int square) {
    uint64_t result = 0ULL;

    int f = 0;
    int r = 0;
    int tr = square / 8;
    int tf = square % 8;
    for (r = tr + 1; r <= 7; r++) {
        result |= (1ULL << (r * 8 + tf));
        if ((occ >> (r * 8 + tf))&1) { break; };
    }
    for (r = tr - 1; r >=0; r--) {
        result |= (1ULL << (r * 8 + tf));
        if ((occ >> (r * 8 + tf))&1) { break; };
    }
    for (f = tf + 1;f <= 7;f++) {
        result |= (1ULL << (tr * 8 + f));
        if ((occ >> (tr * 8 + f))&1) { break; };
    }
    for (f = tf - 1;f >= 0; f--) {
        result |= (1ULL << (tr * 8 + f));
        if ((occ >> (tr * 8 + f))&1) { break; };
    }
    return result;
}

const std::array<uint64_t,64> r_mask = init_rmask();
const std::array<uint64_t, 64> b_mask = init_bmask();

constexpr std::array<std::array<uint64_t, 4096>, 64> r_magic_init() {
    std::array<std::array<uint64_t, 4096>, 64> r_indexs{};
    for (int i = 0; i < 64; i++) {
        //counts bit in masks
        int r_count = 0;
        r_count = std::popcount(r_mask[i]);
        int p_count = 1 << r_count;

        //repeat for rook
        for (int j = 0; j < p_count; j++) {
            uint64_t occ = set_occupancy(j, r_count, r_mask[i]);
            uint64_t magic = occ* Rmagics[i] >> (64 - RBn[i]);
            r_indexs[i][magic] = RC(occ, i);
        }
    }
    return r_indexs;
}


constexpr std::array<std::array<uint64_t, 512>, 64> b_magic_init() {
    std::array<std::array<uint64_t, 512>, 64> b_indexs{};
    for (int i = 0; i < 64; i++) {
        //counts bit in masks
        int b_count = 0;
        b_count = std::popcount(b_mask[i]);
        int p_count = 1 << b_count;

        //repeat for rook
        for (int j = 0; j < p_count; j++) {
            uint64_t occ = set_occupancy(j, b_count, b_mask[i]);
            uint64_t magic = occ* Bmagics[i] >> (64 - BBn[i]);
            b_indexs[i][magic] = BC(occ, i);
        }
    }
    return b_indexs;
}

const std::array<std::array<uint64_t, 4096>, 64> r_indexs = r_magic_init();
const std::array<std::array<uint64_t, 512>, 64> b_indexs = b_magic_init();


