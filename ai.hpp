#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "eval.hpp"



template <class BoardState status, int depth>
constexpr inline int minimax(const Board &brd, int ep, int alpha, int beta, int score) noexcept {
    if constexpr (depth == 0) {
        // Evaluate from current player's perspective
        return -score;
    } 
    else {
        Callback ml[100];
        int count = 0;
        genMoves<status, depth, 1>(brd, ep, ml, count);
        
        if (count == 0) {
            return -99998; // No moves available (checkmate or stalemate)
        }
        
        int maxEval = -99999;
        for (int i = 0; i < count; i++) {
            // Assuming move() makes the move on a copy of the board and returns negamax evaluation
            int eval = -ml[i].move(brd, ml[i].from, ml[i].to, -beta, -alpha,-score);
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) {
                break; // Alpha-beta pruning
            }
        }
        return maxEval;
    }
}


template <int depth>
constexpr inline Callback findBestMove(const Board &brd, int ep, bool WH, bool EP, bool WL, bool WR, bool BL, bool BR){
    Callback ml[100];
    int count = 0;
    moveGenCall<depth, 1>(brd, ep, ml, count, WH, EP, WL, WR, BL, BR);
    int intitalScore = WH ? eval<true>(brd) : eval<false>(brd);
    int bestScore = -99999;
    int bestMoveIndex = 0; // Initialize to first move in case no better move is found

    for (int i = 0; i < count; i++) {
        // Get the score for this move
        int score = -ml[i].move(brd, ml[i].from, ml[i].to, -99999, 99999,intitalScore);
        
        // Update best move based on whether we're white or black
        if (score > bestScore) {
            bestScore = score;
            bestMoveIndex = i;
        }
    }
    
    printf("score: %d\n", bestScore);
    count = 0;
    moveGenCall<1,0>(brd, ep, ml, count, WH, EP, WL, WR, BL, BR);
    return ml[bestMoveIndex];
}
