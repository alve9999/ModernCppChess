#pragma once
#include "nnue.h"
struct minimax_info_t{
    int ep;
    int alpha;
    int beta;
    int score;
    uint64_t key;
    int depth;
    int irreversibleCount;
    int ply;
    bool isPVNode;
    bool isCapture;
    bool nullMove;
    int from;
    int to;
    minimax_info_t* prevMove;
    AccumulatorPair* accPair;
};

struct move_info_t {
    int from;
    int to;
    int alpha;
    int beta;
    int score;
    uint64_t key;
    int depth;
    int irreversibleCount;
    int ply;
    bool isPVNode;
    minimax_info_t* prevMove;
    AccumulatorPair* accPair;
};
