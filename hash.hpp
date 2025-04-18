#pragma once
#include <cstdint>
#include <array>
#define UNKNOWN 989

extern long ttc;
extern long ttf;
constexpr uint64_t rand_constexpr(uint64_t seed) {
    return seed * 6364136223846793005ULL + 1;
}

constexpr std::array<uint64_t, 769> gen_random_keys(uint64_t seed = 0xdeadbeefcafebabeULL) {
    std::array<uint64_t, 769> keys{};
    uint64_t value = seed;
    for (size_t i = 0; i < keys.size(); i++) {
        value = rand_constexpr(value);
        keys[i] = value;
    }
    return keys;
}

constexpr auto random_key = gen_random_keys();

struct entry {
    uint64_t key;
    int32_t value;
    int8_t depth;
    int8_t flags;
};

static_assert(sizeof(entry) == 16, "entry must be 16 bytes");

/*
struct bucket {
    entry main; 
    entry always;
};
*/
class TranspositionTable {
public:
    uint64_t size;
    //bucket* Table;
    entry* Table;
	TranspositionTable(uint64_t size);

    void store(int depth, int val, int flag, uint64_t key);
    int probe_hash(int depth, int alpha, int beta, uint64_t key);
};

uint64_t create_hash(const Board& board, bool isWhite);

extern TranspositionTable TT;
