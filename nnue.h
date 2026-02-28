#pragma once
#define HL_SIZE 512

struct alignas(64) Accumulator {
    int16_t values[HL_SIZE];
};

struct AccumulatorPair {
    Accumulator white;
    Accumulator black;
};


int calculate_idx(int piece_type, int side, int square, int perspective);

void nnue_init(AccumulatorPair* pair,const Board &brd);

int16_t activate(int16_t x);

void accumulatorAddPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square);


void accumulatorSubPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square);

int nnue_evaluate(AccumulatorPair* pair, int side_to_move);
