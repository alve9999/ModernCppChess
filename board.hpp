#pragma once
#include <iostream>
#include <stdint.h>
#include <string>
#include "constants.hpp"

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

enum class BoardPiece { Pawn, Knight, Bishop, Rook, Queen, King };

struct Move {
    uint8_t from;
    uint8_t to;
    BoardPiece piece;
    uint8_t special;
    std::string toAlgebraic() const {
        char file_from = 'h' - (this->from % 8);
        char rank_from = '1' + (this->from / 8);
        char file_to = 'h' - (this->to % 8);
        char rank_to = '1' + (this->to / 8);
        return std::string{file_from, rank_from, file_to, rank_to};
    }
};

typedef struct Move Move;

struct BoardState {
    const bool IsWhite, EP, WLC, WRC, BLC, BRC;
    constexpr BoardState(bool IsWhite, bool EP, bool WLC, bool WRC, bool BLC,
                         bool BRC)
        : IsWhite(IsWhite), EP(EP), WLC(WLC), WRC(WRC), BLC(BLC), BRC(BRC) {}
    constexpr BoardState()
        : IsWhite(0), EP(0), WLC(0), WRC(0), BLC(0), BRC(0) {}

    constexpr BoardState normal() const noexcept
    {
        return BoardState(!IsWhite, false, WLC, WRC, BLC, BRC);
    }

    constexpr BoardState king() const noexcept
    {
        if (IsWhite)
        {
            return BoardState(!IsWhite, false, false, false, BLC, BRC);
        }
        else
        {
            return BoardState(!IsWhite, false, WLC, WRC, false, false);
        }
    }

    constexpr BoardState pawn() const noexcept
    {
        return BoardState(!IsWhite, true, WLC, WRC, BLC, BRC);
    }

    constexpr BoardState rookMoveLeft() const noexcept
    {
        if (IsWhite) {
            return BoardState(!IsWhite, false, false, WRC, BLC, BRC);
        }
        else {
            return BoardState(!IsWhite, false, WLC, WRC, false, BRC);
        }
    }

    constexpr BoardState rookMoveRight() const noexcept
    {
        if (IsWhite) {
            return BoardState(!IsWhite, false, WLC, false, BLC, BRC);
        }
        else {
            return BoardState(!IsWhite, false, WLC, WRC, BLC, false);
        }
    }


};


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

    constexpr Board(uint64_t bp, uint64_t bn, uint64_t bb, uint64_t br,
                    uint64_t bq, uint64_t bk, uint64_t wp, uint64_t wn,
                    uint64_t wb, uint64_t wr, uint64_t wq, uint64_t wk) noexcept
        : BPawn(bp), BKnight(bn), BBishop(bb), BRook(br), BQueen(bq), BKing(bk),
          WPawn(wp), WKnight(wn), WBishop(wb), WRook(wr), WQueen(wq), WKing(wk),
          Black(bp | bn | bb | br | bq | bk),
          White(wp | wn | wb | wr | wq | wk), Occ(Black | White){}
    constexpr Board() noexcept
        : BPawn(0), BKnight(0), BBishop(0), BRook(0), BQueen(0), BKing(0),
          WPawn(0), WKnight(0), WBishop(0), WRook(0), WQueen(0), WKing(0),
          Black(0), White(0), Occ(0) {}

    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board move(int moveFrom,int moveTo) const noexcept{
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;
        const uint64_t mov = 1ULL << moveTo | 1ULL << moveFrom;
        if constexpr (IsWhite) {
            if constexpr (BoardPiece::Pawn == piece)    return Board(bp, bn, bb, br, bq, bk, wp ^ mov, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn, bb, br, bq, bk, wp, wn ^ mov, wb, wr, wq, wk);
            if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb, br, bq, bk, wp, wn, wb ^ mov, wr, wq, wk);
            if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ mov, wq, wk);
            if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq ^ mov, wk);
            if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk ^ mov);
        }
        else {
            if constexpr (BoardPiece::Pawn == piece)    return Board(bp ^ mov, bn, bb, br, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn ^ mov, bb, br, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb ^ mov, br, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br ^ mov, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq ^ mov, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk ^ mov, wp, wn, wb, wr, wq, wk);
        }

    }
    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board capture(int moveFrom,int moveTo) const noexcept{
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;
        const uint64_t rem = 1ULL << moveTo;
        const uint64_t mov = 1ULL << moveTo | 1ULL << moveFrom;
        if constexpr (IsWhite) {
            if constexpr (BoardPiece::Pawn == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ mov, wn, wb, wr, wq, wk);
            if constexpr (BoardPiece::Knight == piece)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn ^ mov, wb, wr, wq, wk);
            if constexpr (BoardPiece::Bishop == piece)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb ^ mov, wr, wq, wk);
            if constexpr (BoardPiece::Rook == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr ^ mov, wq, wk);
            if constexpr (BoardPiece::Queen == piece)   return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq ^ mov, wk);
            if constexpr (BoardPiece::King == piece)    return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp, wn, wb, wr, wq, wk ^ mov);
        }
        else {
            if constexpr (BoardPiece::Pawn == piece)    return Board(bp ^ mov, bn, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Knight == piece)  return Board(bp, bn ^ mov, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Bishop == piece)  return Board(bp, bn, bb ^ mov, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Rook == piece)    return Board(bp, bn, bb, br ^ mov, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::Queen == piece)   return Board(bp, bn, bb, br, bq ^ mov, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (BoardPiece::King == piece)    return Board(bp, bn, bb, br, bq, bk ^ mov, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
        }
    }

    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board EP(int moveFrom,int moveTo) const noexcept
    {
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;
        const uint64_t from = 1ULL << moveFrom;
        const uint64_t to = 1ULL << moveTo;
        const uint64_t mov = from | to;
        if constexpr (IsWhite) {
            const uint64_t remove = ~(1ULL << (moveTo - 8));
            return Board(bp & remove, bn & remove, bb & remove, br & remove, bq & remove, bk & remove, wp ^ mov, wn, wb, wr, wq, wk);
        } else {
            const uint64_t remove = ~(1ULL << (moveTo + 8));
            return Board(bp ^ mov, bn, bb, br, bq, bk, wp & remove, wn & remove, wb & remove, wr & remove, wq & remove, wk & remove);
        }
    }

    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board promote(int moveFrom, int moveTo) const noexcept
    {
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;
        const uint64_t from = 1ULL << moveFrom;
        const uint64_t to = 1ULL << moveTo;
        if constexpr (IsWhite){
            if constexpr (piece == BoardPiece::Queen)  return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb, wr, wq ^ to, wk);
            if constexpr (piece == BoardPiece::Rook)   return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb ,wr ^ to, wq, wk);
            if constexpr (piece == BoardPiece::Bishop) return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn, wb ^ to, wr, wq, wk);
            if constexpr (piece == BoardPiece::Knight) return Board(bp, bn, bb, br, bq, bk, wp ^ from, wn ^ to, wb, wr, wq, wk);
        }
        else {
            if constexpr (piece == BoardPiece::Queen)  return Board(bp ^ from, bn, bb, br, bq ^ to, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (piece == BoardPiece::Rook)   return Board(bp ^ from, bn, bb, br ^ to, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (piece == BoardPiece::Bishop) return Board(bp ^ from, bn, bb ^ to, br, bq, bk, wp, wn, wb, wr, wq, wk);
            if constexpr (piece == BoardPiece::Knight) return Board(bp ^ from, bn ^ to, bb, br, bq, bk, wp, wn, wb, wr, wq, wk);
        }

    }
    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board promoteCapture(int moveFrom,int moveTo) const noexcept
    {
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;
        const uint64_t from = 1ULL << moveFrom;
        const uint64_t to = 1ULL << moveTo;
        const uint64_t rem = 1ULL << moveTo;
        if constexpr (IsWhite){
            if constexpr (piece == BoardPiece::Queen)  return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb, wr, wq ^ to, wk);
            if constexpr (piece == BoardPiece::Rook)   return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb ,wr ^ to, wq, wk);
            if constexpr (piece == BoardPiece::Bishop) return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn, wb ^ to, wr, wq, wk);
            if constexpr (piece == BoardPiece::Knight) return Board(bp & rem, bn & rem, bb & rem, br & rem, bq & rem, bk, wp ^ from, wn ^ to, wb, wr, wq, wk);
        }
        else {
            if constexpr (piece == BoardPiece::Queen)  return Board(bp ^ from, bn, bb, br, bq ^ to, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (piece == BoardPiece::Rook)   return Board(bp ^ from, bn, bb, br ^ to, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (piece == BoardPiece::Bishop) return Board(bp ^ from, bn, bb ^ to, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
            if constexpr (piece == BoardPiece::Knight) return Board(bp ^ from, bn ^ to, bb, br, bq, bk, wp & rem, wn & rem, wb & rem, wr & rem, wq & rem, wk);
        }
    }
    template <BoardPiece piece,bool IsWhite,bool WLC, bool WRC, bool BLC, bool BRC>
    _fast Board castle() const noexcept
    {
        const uint64_t bp = BPawn;
        const uint64_t bn = BKnight;
        const uint64_t bb = BBishop;
        const uint64_t br = BRook;
        const uint64_t bq = BQueen;
        const uint64_t bk = BKing;
        const uint64_t wp = WPawn;
        const uint64_t wn = WKnight;
        const uint64_t wb = WBishop;
        const uint64_t wr = WRook;
        const uint64_t wq = WQueen;
        const uint64_t wk = WKing;

        if constexpr (WLC){
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ WRookLChange, wq, wk ^ WKingLChange);
        }
        if constexpr (WRC){
            return Board(bp, bn, bb, br, bq, bk, wp, wn, wb, wr ^ WRookRChange, wq, wk ^ WKingRChange);
        }
        if constexpr (BLC){
            return Board(bp, bn, bb, br ^ BRookLChange, bq, bk ^ BKingLChange, wp, wn, wb, wr, wq, wk);
        }
        if constexpr (BRC){
            return Board(bp, bn, bb, br ^ BRookRChange, bq, bk ^ BKingRChange, wp, wn, wb, wr, wq, wk);
        }
    }

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

constexpr size_t constexprFind(const char* str, char ch, size_t start = 0) {
    size_t i = start;
    while (str[i] != '\0') {
        if (str[i] == ch) return i;
        ++i;
    }
    return std::string::npos;
}


inline constexpr int parseEnPassantSquare(const char* FEN) {
    size_t space1 = constexprFind(FEN, ' ');
    size_t space2 = constexprFind(FEN, ' ', space1 + 1);
    size_t space3 = constexprFind(FEN, ' ', space2 + 1);
    if (FEN[space3 + 1] == '-') {
        return -1;
    }
    char file = FEN[space3 + 1];
    char rank = FEN[space3 + 2];
    return (file - 'a') + 8 * (rank - '1');
}

inline constexpr BoardState parseBoardState(const char* fenCStr) {
    
    size_t space1 = constexprFind(fenCStr, ' ');
    size_t space2 = constexprFind(fenCStr, ' ', space1 + 1);
    size_t space3 = constexprFind(fenCStr, ' ', space2 + 1);
    size_t space4 = constexprFind(fenCStr, ' ', space3 + 1);
    size_t space5 = constexprFind(fenCStr, ' ', space4 + 1);

    bool IsWhite = (fenCStr[space1 + 1] == 'w');
    bool WRC = false, WLC = false, BRC = false, BLC = false;

    int diff = space3 - space2;
    for (int i = 0; i < diff; i++) {
        char c = fenCStr[space2 + i + 1];
        if (c == 'K') WRC = true;
        if (c == 'Q') WLC = true;
        if (c == 'k') BRC = true;
        if (c == 'q') BLC = true;
    }

    int enPassantSquare = parseEnPassantSquare(fenCStr);
    bool EP = (enPassantSquare != -1);
    int ep = parseEnPassantSquare(fenCStr);

    return BoardState(IsWhite, EP, WLC, WRC, BLC, BRC);
}

inline Board loadFenBoard(std::string FEN) {
    return Board(FenToBmp(FEN, 'p'), FenToBmp(FEN, 'n'), FenToBmp(FEN, 'b'),
                 FenToBmp(FEN, 'r'), FenToBmp(FEN, 'q'), FenToBmp(FEN, 'k'),
                 FenToBmp(FEN, 'P'), FenToBmp(FEN, 'N'), FenToBmp(FEN, 'B'),
                 FenToBmp(FEN, 'R'), FenToBmp(FEN, 'Q'), FenToBmp(FEN, 'K'));
}
