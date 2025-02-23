#pragma once
#include <iostream>
#include <stdint.h>
#include <string>
#define _fast inline constexpr

inline constexpr uint64_t _blsr_u64(uint64_t x) noexcept { return x & (x - 1); }
#define Bitloop(X) for (; X; X = _blsr_u64(X))

inline void printBitboard(uint64_t bitboard) {
    for (int rank = 7; rank >= 0; rank--) {
        for (int file = 7; file >= 0; file--) {
            int square = rank * 8 + file;
            std::cout << ((bitboard & (1ULL << square)) ? '1' : '0') << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

struct BoardState {
    const bool IsWhite, EP, WLC, WRC, BLC, BRC;
    constexpr BoardState(bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC,
                         bool BRC)
        : IsWhite(IsWhite), EP(EP), WLC(WLC), WRC(WRC), BLC(BLC), BRC(BRC) {}
    constexpr BoardState()
        : IsWhite(0), EP(0), WLC(0), WRC(0), BLC(0), BRC(0) {}
};

enum class BoardPiece { Pawn, Knight, Bishop, Rook, Queen, King };
#include <math.h>
struct Board {
    const uint64_t BPawn;
    const uint64_t BKnight;
    const uint64_t BBishop;
    const uint64_t BRook;
    const uint64_t BQueen;
    const uint64_t BKing;
    const uint64_t WPawn;
    const uint64_t WKnight;
    const uint64_t WBishop;
    const uint64_t WRook;
    const uint64_t WQueen;
    const uint64_t WKing;

    const uint64_t Black;
    const uint64_t White;
    const uint64_t Occ;
    const int EPSquare;
    const BoardState state;

    constexpr Board(uint64_t bp, uint64_t bn, uint64_t bb, uint64_t br,
                    uint64_t bq, uint64_t bk, uint64_t wp, uint64_t wn,
                    uint64_t wb, uint64_t wr, uint64_t wq, uint64_t wk, int ep,
                    BoardState state) noexcept
        : BPawn(bp), BKnight(bn), BBishop(bb), BRook(br), BQueen(bq), BKing(bk),
          WPawn(wp), WKnight(wn), WBishop(wb), WRook(wr), WQueen(wq), WKing(wk),
          Black(bp | bn | bb | br | bq | bk),
          White(wp | wn | wb | wr | wq | wk), Occ(Black | White), EPSquare(ep),
          state(state) {}
    constexpr Board() noexcept
        : BPawn(0), BKnight(0), BBishop(0), BRook(0), BQueen(0), BKing(0),
          WPawn(0), WKnight(0), WBishop(0), WRook(0), WQueen(0), WKing(0),
          Black(0), White(0), Occ(0), EPSquare(0), state(BoardState()) {}
};

inline static uint64_t FenToBmp(std::string FEN, char p) {
    uint64_t result = 0;
    int Field = 63;

    for (char c : FEN) {
        if (c == ' ')
            break;
        uint64_t P = 1ull << Field;
        switch (c) {
        case '/':
            Field += 1;
            break;
        case '1':
            break;
        case '2':
            Field -= 1;
            break;
        case '3':
            Field -= 2;
            break;
        case '4':
            Field -= 3;
            break;
        case '5':
            Field -= 4;
            break;
        case '6':
            Field -= 5;
            break;
        case '7':
            Field -= 6;
            break;
        case '8':
            Field -= 7;
            break;
        default:
            if (c == p)
                result |= P;
        }
        Field--;
    }
    return result;
}

inline int parseEnPassantSquare(const std::string &FEN) {
    size_t space1 = FEN.find(' ');
    size_t space2 = FEN.find(' ', space1 + 1);
    size_t space3 = FEN.find(' ', space2 + 1);
    if (FEN[space3 + 1] == '-') {
        return -1;
    }
    char file = FEN[space3 + 1];
    char rank = FEN[space3 + 2];
    return (file - 'a') + 8 * (rank - '1');
}

inline BoardState parseBoardState(const std::string &FEN) {
    size_t space1 = FEN.find(' ');
    size_t space2 = FEN.find(' ', space1 + 1);
    size_t space3 = FEN.find(' ', space2 + 1);
    size_t space4 = FEN.find(' ', space3 + 1);
    size_t space5 = FEN.find(' ', space4 + 1);

    bool IsWhite = (FEN[space1 + 1] == 'w');
    bool WRC, WLC, BRC, BLC;
    WRC = false;
    WLC = false;
    BRC = false;
    BLC = false;
    int diff = space3 - space2;
    for (int i = 0; i < diff; i++) {
        if (FEN[space2 + i + 1] == 'K')
            WRC = true;
        if (FEN[space2 + i + 1] == 'Q')
            WLC = true;
        if (FEN[space2 + i + 1] == 'k')
            BRC = true;
        if (FEN[space2 + i + 1] == 'q')
            BLC = true;
    }
    int enPassantSquare = parseEnPassantSquare(FEN);
    bool EP = (enPassantSquare != -1);
    return BoardState(IsWhite, EP, WLC, WRC, BLC, BRC);
}

inline Board loadFen(std::string FEN) {
    return Board(FenToBmp(FEN, 'p'), FenToBmp(FEN, 'n'), FenToBmp(FEN, 'b'),
                 FenToBmp(FEN, 'r'), FenToBmp(FEN, 'q'), FenToBmp(FEN, 'k'),
                 FenToBmp(FEN, 'P'), FenToBmp(FEN, 'N'), FenToBmp(FEN, 'B'),
                 FenToBmp(FEN, 'R'), FenToBmp(FEN, 'Q'), FenToBmp(FEN, 'K'),
                 parseEnPassantSquare(FEN), parseBoardState(FEN));
}
