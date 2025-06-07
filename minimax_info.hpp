#pragma once
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
    minimax_info_t* prevMove;
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
};
