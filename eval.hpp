#pragma once
#include "board.hpp"
#include "constants.hpp"

extern int eg_phase;
extern int mg_phase;


inline int countBits(uint64_t bitboard) {
    int count = 0;
    while (bitboard) {
        count++;
        bitboard &= bitboard - 1;
    }
    return count;
}

inline int calculatePhaseInterpolation(const Board& board) {
    int phase = countBits(board.WBishop)+ countBits(board.BBishop) +
                 countBits(board.WKnight) + countBits(board.BKnight) +
                 2*countBits(board.WRook) + 2*countBits(board.BRook) +
                 4*countBits(board.WQueen) + 4*countBits(board.BQueen);
    return phase;
}

template <bool IsWhite>
int pieceSquareScore(uint64_t bitboard, const int (&PST)[64]) {
    int score = 0;
    while (bitboard) {
        int square = __builtin_ctzll(bitboard);
        score += PST[square];
        bitboard &= bitboard - 1;
    }
    return score;
}

template <bool IsWhite> int evalSide(const Board &brd) {
    int mg_score = 0;
    int eg_score = 0;

    uint64_t pawns = IsWhite ? brd.WPawn : brd.BPawn;
    uint64_t knights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t bishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t rooks = IsWhite ? brd.WRook : brd.BRook;
    uint64_t queens = IsWhite ? brd.WQueen : brd.BQueen;
    uint64_t king = IsWhite ? brd.WKing : brd.BKing;

    int pawnCount = __builtin_popcountll(pawns);
    int knightCount = __builtin_popcountll(knights);
    int bishopCount = __builtin_popcountll(bishops);
    int rookCount = __builtin_popcountll(rooks);
    int queenCount = __builtin_popcountll(queens);

    mg_score += pawnCount * mg_value[0];
    mg_score += knightCount * mg_value[1];
    mg_score += bishopCount * mg_value[2];
    mg_score += rookCount * mg_value[3];
    mg_score += queenCount * mg_value[4];

    mg_score += pieceSquareScore<IsWhite>(pawns, mg_table[0][IsWhite]);
    mg_score += pieceSquareScore<IsWhite>(knights, mg_table[1][IsWhite]);
    mg_score += pieceSquareScore<IsWhite>(bishops, mg_table[2][IsWhite]);
    mg_score += pieceSquareScore<IsWhite>(rooks, mg_table[3][IsWhite]);
    mg_score += pieceSquareScore<IsWhite>(queens, mg_table[4][IsWhite]);
    mg_score += pieceSquareScore<IsWhite>(king, mg_table[5][IsWhite]);

    eg_score += pawnCount * eg_value[0];
    eg_score += knightCount * eg_value[1];
    eg_score += bishopCount * eg_value[2];
    eg_score += rookCount * eg_value[3];
    eg_score += queenCount * eg_value[4];

    eg_score += pieceSquareScore<IsWhite>(pawns, eg_table[0][IsWhite]);
    eg_score += pieceSquareScore<IsWhite>(knights, eg_table[1][IsWhite]);
    eg_score += pieceSquareScore<IsWhite>(bishops, eg_table[2][IsWhite]);
    eg_score += pieceSquareScore<IsWhite>(rooks, eg_table[3][IsWhite]);
    eg_score += pieceSquareScore<IsWhite>(queens, eg_table[4][IsWhite]);
    eg_score += pieceSquareScore<IsWhite>(king, eg_table[5][IsWhite]);
    
    int score = (mg_phase * mg_score + eg_phase * eg_score) / 24;
    return score;
}

template <bool IsWhite> int evaluate(const Board &brd) {
    int whiteScore = evalSide<true>(brd);
    int blackScore = evalSide<false>(brd);

    return IsWhite ? (whiteScore - blackScore) : (blackScore - whiteScore);
}
