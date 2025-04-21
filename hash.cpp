#include "hash.hpp"
#include <cstdint>
#include <array>

TranspositionTable TT(27);
long ttc = 0;
long ttf = 0;

TranspositionTable::TranspositionTable(uint64_t log2_size) {
    this->size = 1ULL << log2_size;
    this->Table = (entry*)calloc(this->size, sizeof(entry));
}

void TranspositionTable::store(int depth, int val, int flag, uint64_t key, uint8_t from, uint8_t to) {
    entry* node = &Table[key & (size - 1)];
    if (node->key != key || depth >= node->depth) {
        node->depth = depth;
        node->value = val;
        node->flags = flag;
        node->key = key;
        node->from = from;
        node->to = to;
    }
}

res TranspositionTable::probe_hash(int depth, int alpha, int beta, uint64_t key) {
    __builtin_prefetch(&Table[(key + 1) & (size - 1)], 0, 1);
    entry* node = &Table[key & (size - 1)];
    res result = {UNKNOWN, 255, 255};
    if (__builtin_expect(node->key == key, 1)) {
        result.from = node->from;
        result.to = node->to;
        if (node->depth >= depth) {
            ttc++;
            int val = node->value;
            switch (node->flags) {
                case 0: result.value = val;
                case 1: if (val <= alpha)  result.value = val; break;
                case 2: if (val >= beta) result.value = val; break;
            }
        }
    }
    ttf++;
    return result;
}

/*
TranspositionTable TT(26);


long ttc = 0;
long ttf = 0;

TranspositionTable::TranspositionTable(uint64_t log2_size) {
    this->size = 1ULL << log2_size;
    this->Table = (bucket*)calloc(this->size, sizeof(bucket));
}

void TranspositionTable::store(int depth, int val, int flag, uint64_t key) {
    bucket* node = &Table[key & (size - 1)];
    
    node->always.depth = depth;
    node->always.value = val;
    node->always.flags = flag;
    node->always.key = key;
    
    if (depth >= node->main.depth) {
        node->main.depth = depth;
        node->main.value = val;
        node->main.flags = flag;
        node->main.key = key;
    }
}

int TranspositionTable::probe_hash(int depth, int alpha, int beta, uint64_t key) {
    __builtin_prefetch(&Table[(key + 1) & (size - 1)], 0, 1);
    bucket* node = &Table[key & (size - 1)];
    
    if (__builtin_expect(node->main.key == key, 1)) {
        if (node->main.depth >= depth) {
            ttc++;
            int val = node->main.value;
            switch (node->main.flags) {
                case 0: return val;
                case 1: if (val <= alpha) return val; break;
                case 2: if (val >= beta) return val; break;
            }
        }
    }
    
    if (__builtin_expect(node->always.key == key, 1)) {
        if (node->main.depth >= depth) {
            ttc++;
            int val = node->always.value;
            switch (node->always.flags) {
                case 0: return val;
                case 1: if (val <= alpha) return val; break;
                case 2: if (val >= beta) return val; break;
            }
        }
    }
    
    ttf++;
    return UNKNOWN;
}*/

uint64_t create_hash(const Board& board, bool isWhite) {
    uint64_t key = 0;
    
    for (int i = 0; i < 64; i++) {
        if ((board.WPawn >> i) & 1ULL) {
            key ^= random_key[i];
        }
        if ((board.WKnight >> i) & 1ULL) {
            key ^= random_key[i + 64];
        }
        if ((board.WBishop >> i) & 1ULL) {
            key ^= random_key[i + 128];
        }
        if ((board.WRook >> i) & 1ULL) {
            key ^= random_key[i + 192];
        }
        if ((board.WQueen >> i) & 1ULL) {
            key ^= random_key[i + 256];
        }
        if ((board.WKing >> i) & 1ULL) {
            key ^= random_key[i + 320];
        }
        
        if ((board.BPawn >> i) & 1ULL) {
            key ^= random_key[i + 384];
        }
        if ((board.BKnight >> i) & 1ULL) {
            key ^= random_key[i + 448];
        }
        if ((board.BBishop >> i) & 1ULL) {
            key ^= random_key[i + 512];
        }
        if ((board.BRook >> i) & 1ULL) {
            key ^= random_key[i + 576];
        }
        if ((board.BQueen >> i) & 1ULL) {
            key ^= random_key[i + 640];
        }
        if ((board.BKing >> i) & 1ULL) {
            key ^= random_key[i + 704];
        }
    }
    
    if (isWhite) {
        key ^= random_key[768];
    }
    
    return key;
}
