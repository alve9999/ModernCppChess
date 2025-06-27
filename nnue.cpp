#include "nnue.h"
#include "network_weights4.hpp"
#include <immintrin.h>

#define HL_SIZE 512
#define SCALE 400
#define ACTIVATION_CLIP 256
#define FEATURE_QUANT 64


int calculate_idx(int piece_type, int side, int square, int perspective)
{
    perspective = perspective ^ 1;
    side = side ^ 1;
    if (perspective == 1) {
        side = side ^ 1;          
        square = square ^ 0b111000;
    }
    return side * 64 * 6 + piece_type * 64 + square;
}


void nnue_init(AccumulatorPair* pair, const Board &brd) {
    for (int i = 0; i < HL_SIZE; i++) {
        pair->white.values[i] = FEATURE_BIAS[i];
        pair->black.values[i] = FEATURE_BIAS[i];
    }

    uint64_t occ = brd.Occ;

    while (occ) {
        int square = __builtin_ctzll(occ);
        int piece_type, piece_color;

        if ((1ULL << square) & brd.WPawn) { piece_type = 0; piece_color = 1; }
        else if ((1ULL << square) & brd.WKnight) { piece_type = 1; piece_color = 1; }
        else if ((1ULL << square) & brd.WBishop) { piece_type = 2; piece_color = 1; }
        else if ((1ULL << square) & brd.WRook) { piece_type = 3; piece_color = 1; }
        else if ((1ULL << square) & brd.WQueen) { piece_type = 4; piece_color = 1; }
        else if ((1ULL << square) & brd.WKing) { piece_type = 5; piece_color = 1; }
        else if ((1ULL << square) & brd.BPawn) { piece_type = 0; piece_color = 0; }
        else if ((1ULL << square) & brd.BKnight) { piece_type = 1; piece_color = 0; }
        else if ((1ULL << square) & brd.BBishop) { piece_type = 2; piece_color = 0; }
        else if ((1ULL << square) & brd.BRook) { piece_type = 3; piece_color = 0; }
        else if ((1ULL << square) & brd.BQueen) { piece_type = 4; piece_color = 0; }
        else if ((1ULL << square) & brd.BKing) { piece_type = 5; piece_color = 0; }

        int white_idx = calculate_idx(piece_type, piece_color, square, 1);
        int black_idx = calculate_idx(piece_type, piece_color, square, 0);

        for (int i = 0; i < HL_SIZE; i++) {
            pair->white.values[i] += FEATURE_WEIGHTS[white_idx][i];
            pair->black.values[i] += FEATURE_WEIGHTS[black_idx][i];
        }

        occ &= occ - 1; 
    }
}



void accumulatorAddPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square) {
    int white_idx = calculate_idx(piece_type, piece_color, square, 1);
    int black_idx = calculate_idx(piece_type, piece_color, square, 0);

    for (int i = 0; i < HL_SIZE; i++) {
        pair->white.values[i] += FEATURE_WEIGHTS[white_idx][i];
        pair->black.values[i] += FEATURE_WEIGHTS[black_idx][i];
    }
}

void accumulatorSubPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square) {
    int white_idx = calculate_idx(piece_type, piece_color, square, 1);
    int black_idx = calculate_idx(piece_type, piece_color, square, 0);

    for (int i = 0; i < HL_SIZE; i++) {
        pair->white.values[i] -= FEATURE_WEIGHTS[white_idx][i];
        pair->black.values[i] -= FEATURE_WEIGHTS[black_idx][i];
    }
}

/*

void accumulatorAddPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square) {
    const int white_idx = calculate_idx(piece_type, piece_color, square, 1);
    const int black_idx = calculate_idx(piece_type, piece_color, square, 0);

    const __m512i* w_ptr = (__m512i*)&FEATURE_WEIGHTS[white_idx][0];
    const __m512i* b_ptr = (__m512i*)&FEATURE_WEIGHTS[black_idx][0];
    __m512i* w_acc_ptr = (__m512i*)&pair->white.values[0];
    __m512i* b_acc_ptr = (__m512i*)&pair->black.values[0];

    for (int i = 0; i < HL_SIZE / 32; ++i) {
        __m512i w = _mm512_loadu_si512(&w_ptr[i]);
        __m512i b = _mm512_loadu_si512(&b_ptr[i]);

        __m512i w_acc = _mm512_loadu_si512(&w_acc_ptr[i]);
        __m512i b_acc = _mm512_loadu_si512(&b_acc_ptr[i]);

        _mm512_storeu_si512(&w_acc_ptr[i], _mm512_add_epi16(w_acc, w));
        _mm512_storeu_si512(&b_acc_ptr[i], _mm512_add_epi16(b_acc, b));
    }
}

void accumulatorSubPiece(AccumulatorPair* pair, int piece_type, int piece_color, int square) {
    const int white_idx = calculate_idx(piece_type, piece_color, square, 1);
    const int black_idx = calculate_idx(piece_type, piece_color, square, 0);

    const __m512i* w_ptr = (__m512i*)&FEATURE_WEIGHTS[white_idx][0];
    const __m512i* b_ptr = (__m512i*)&FEATURE_WEIGHTS[black_idx][0];
    __m512i* w_acc_ptr = (__m512i*)&pair->white.values[0];
    __m512i* b_acc_ptr = (__m512i*)&pair->black.values[0];

    for (int i = 0; i < HL_SIZE / 32; ++i) {

        __m512i w = _mm512_loadu_si512(&w_ptr[i]);
        __m512i b = _mm512_loadu_si512(&b_ptr[i]);

        __m512i w_acc = _mm512_loadu_si512(&w_acc_ptr[i]);
        __m512i b_acc = _mm512_loadu_si512(&b_acc_ptr[i]);

        _mm512_storeu_si512(&w_acc_ptr[i], _mm512_sub_epi16(w_acc, w));
        _mm512_storeu_si512(&b_acc_ptr[i], _mm512_sub_epi16(b_acc, b));
    }
}

*/

#include <cstring>

int nnue_evaluate(AccumulatorPair* pair, int side_to_move) {
    const int16_t* own = (side_to_move == 0) ? pair->white.values : pair->black.values;
    const int16_t* opp = (side_to_move == 0) ? pair->black.values : pair->white.values;

    const __m512i zero = _mm512_setzero_si512();
    const __m512i clip = _mm512_set1_epi16(ACTIVATION_CLIP);

    __m512i acc = _mm512_setzero_si512();

    for (int i = 0; i < HL_SIZE; i += 32) {
        __m512i own_v = _mm512_loadu_si512((__m512i*)&own[i]);
        __m512i opp_v = _mm512_loadu_si512((__m512i*)&opp[i]);

        own_v = _mm512_min_epi16(_mm512_max_epi16(own_v, zero), clip);
        opp_v = _mm512_min_epi16(_mm512_max_epi16(opp_v, zero), clip);

        __m512i w_own = _mm512_load_si512((__m512i*)&OUTPUT_WEIGHTS[i]);
        __m512i w_opp = _mm512_load_si512((__m512i*)&OUTPUT_WEIGHTS[HL_SIZE + i]);

        __m512i own_lo = _mm512_cvtepi16_epi32(_mm512_castsi512_si256(own_v));
        __m512i own_hi = _mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(own_v, 1));
        __m512i opp_lo = _mm512_cvtepi16_epi32(_mm512_castsi512_si256(opp_v));
        __m512i opp_hi = _mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(opp_v, 1));

        __m512i wown_lo = _mm512_cvtepi16_epi32(_mm512_castsi512_si256(w_own));
        __m512i wown_hi = _mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(w_own, 1));
        __m512i wopp_lo = _mm512_cvtepi16_epi32(_mm512_castsi512_si256(w_opp));
        __m512i wopp_hi = _mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(w_opp, 1));

        acc = _mm512_add_epi32(acc, _mm512_mullo_epi32(_mm512_mullo_epi32(own_lo, own_lo), wown_lo));
        acc = _mm512_add_epi32(acc, _mm512_mullo_epi32(_mm512_mullo_epi32(own_hi, own_hi), wown_hi));
        acc = _mm512_add_epi32(acc, _mm512_mullo_epi32(_mm512_mullo_epi32(opp_lo, opp_lo), wopp_lo));
        acc = _mm512_add_epi32(acc, _mm512_mullo_epi32(_mm512_mullo_epi32(opp_hi, opp_hi), wopp_hi));
    }

    int32_t temp[16];
    _mm512_store_si512((__m512i*)temp, acc);

    __m256i lo = _mm512_castsi512_si256(acc);
    __m256i hi = _mm512_extracti64x4_epi64(acc, 1);
    __m256i sum = _mm256_add_epi32(lo, hi);
    sum = _mm256_hadd_epi32(sum, sum);
    sum = _mm256_hadd_epi32(sum, sum);
    int32_t output = OUTPUT_BIAS + _mm256_extract_epi32(sum, 0) + _mm256_extract_epi32(sum, 4);
    return output / (256*64);
}
