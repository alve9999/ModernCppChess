#pragma once
#include "board.hpp"
#include "constants.hpp"

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
    int score = 0;

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

    score += pawnCount * mg_value[0];
    score += knightCount * mg_value[1];
    score += bishopCount * mg_value[2];
    score += rookCount * mg_value[3];
    score += queenCount * mg_value[4];

    score += pieceSquareScore<IsWhite>(pawns, mg_table[0][IsWhite]);
    score += pieceSquareScore<IsWhite>(knights, mg_table[1][IsWhite]);
    score += pieceSquareScore<IsWhite>(bishops, mg_table[2][IsWhite]);
    score += pieceSquareScore<IsWhite>(rooks, mg_table[3][IsWhite]);
    score += pieceSquareScore<IsWhite>(queens, mg_table[4][IsWhite]);
    score += pieceSquareScore<IsWhite>(king, mg_table[5][IsWhite]);

    return score;
}

template <bool IsWhite> int evaluate(const Board &brd) {
    int whiteScore = evalSide<true>(brd);
    int blackScore = evalSide<false>(brd);

    return IsWhite ? (whiteScore - blackScore) : (blackScore - whiteScore);
}
