#pragma once
#include "board.hpp"
#include "constants.hpp"
#include "sliding.hpp"

extern int eg_phase;
extern int mg_phase;

constexpr int mobility_pawn[2][1] = {
    {0},
    {0}
};

constexpr int mobility_king[2][1] = {
    {0},
    {0}
};

constexpr int mobility_knight[2][8] = {
    {-44, -24, -13, -8, 3, 9, 17, 22},
    {-13, -8, -5, 0, 3, 12, 10, 9}
};
constexpr int mobility_bishop[2][14] = {
    {-56, -41, -27, -19, -10, -5, 0, 3, 2, 12, 23, 49, 7, 62},
    {4, -14, -24, -17, -8, 1, 7, 9, 14, 10, 4, 0, 25, -11}
};
constexpr int mobility_rook[2][15] = {
    {-44,-31,-24,-19,-12,-9,-4,5,12,15,25,26,45,36},
    {-40,-16,-16,-11,-7,-4,2,5,7,10,13,15,19,11,12}
};

constexpr int mobility_queen[2][28] = {
    {-33,-33,-34,-35,-33,-26,-21,-18,-15,-9,-7,-4,-5,4,5,1,0,17,12,29,35,68,47,88,37,44,-45,-70},
    {67,235,95,56,52,-24,-62,-72,-70,-78,-63,-52,-48,-42,-31,-15,-4,-19,-5,-9,-5,-20,-4,-13,4,1,66,61}
};



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

template <bool IsWhite>
int evalMobility(const Board& brd) {
    int mg_score = 0;
    int eg_score = 0;

    uint64_t knights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t bishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t rooks   = IsWhite ? brd.WRook   : brd.BRook;
    uint64_t queens  = IsWhite ? brd.WQueen  : brd.BQueen;
    uint64_t col     = IsWhite ? brd.White    : brd.Black;

    while (knights) {
        int sq = __builtin_ctzll(knights);
        uint64_t attacks = knightMasks[sq] & ~col;
        int mobility = __builtin_popcountll(attacks);
        mg_score += mobility_knight[0][mobility];
        eg_score += mobility_knight[1][mobility];
        knights &= knights - 1;
    }

    while (bishops) {
        int sq = __builtin_ctzll(bishops);
        uint64_t attacks = getBmagic(sq, brd.Occ) & ~col;
        int mobility = __builtin_popcountll(attacks);
        mg_score += mobility_bishop[0][mobility];
        eg_score += mobility_bishop[1][mobility];
        bishops &= bishops - 1;
    }

    while (rooks) {
        int sq = __builtin_ctzll(rooks);
        uint64_t attacks = getRmagic(sq, brd.Occ) & ~col;
        int mobility = __builtin_popcountll(attacks);
        mg_score += mobility_rook[0][mobility];
        eg_score += mobility_rook[1][mobility];
        rooks &= rooks - 1;
    }

    while (queens) {
        int sq = __builtin_ctzll(queens);
        uint64_t attacks = getQmagic(sq, brd.Occ) & ~col;
        int mobility = __builtin_popcountll(attacks);
        mg_score += mobility_queen[0][mobility];
        eg_score += mobility_queen[1][mobility];
        queens &= queens - 1;
    }

    int score = (mg_phase * mg_score + eg_phase * eg_score) / 24;
    return score;
}

template <bool IsWhite> int evaluate(const Board &brd) {
    int whiteScore = evalSide<true>(brd);
    int blackScore = evalSide<false>(brd);

    return IsWhite ? (whiteScore - blackScore) : (blackScore - whiteScore);
}

inline bool areRooksConnected(uint64_t rooks) {
    if (__builtin_popcountll(rooks) < 2) {
        return false;
    }
    
    // Check for rooks on the same rank
    for (int rank = 0; rank < 8; rank++) {
        uint64_t rankMask = 0xFFULL << (rank * 8);
        if (__builtin_popcountll(rooks & rankMask) >= 2) {
            return true;
        }
    }
    
    // Check for rooks on the same file
    for (int file = 0; file < 8; file++) {
        uint64_t fileMask = 0x0101010101010101ULL << file;
        if (__builtin_popcountll(rooks & fileMask) >= 2) {
            return true;
        }
    }
    
    return false;
}

// Helper functions for staticEval


template <bool IsWhite>
uint64_t identifyBackwardPawns(uint64_t friendlyPawns, uint64_t enemyPawns) {
    uint64_t backwardPawns = 0;
    uint64_t pawnsCopy = friendlyPawns;
    
    while (pawnsCopy) {
        int sq = __builtin_ctzll(pawnsCopy);
        int file = sq % 8;
        int rank = sq / 8;
        
        int forwardRank = IsWhite ? rank + 1 : rank - 1;
        
        // Check if can be supported by friendly pawns
        bool canBeSupported = false;
        
        // Check adjacent files for supporting pawns
        if (file > 0) {
            int supportSq = forwardRank * 8 + (file - 1);
            if ((IsWhite && rank < 6) || (!IsWhite && rank > 1)) {
                if (friendlyPawns & (1ULL << supportSq)) {
                    canBeSupported = true;
                }
            }
        }
        
        if (file < 7) {
            int supportSq = forwardRank * 8 + (file + 1);
            if ((IsWhite && rank < 6) || (!IsWhite && rank > 1)) {
                if (friendlyPawns & (1ULL << supportSq)) {
                    canBeSupported = true;
                }
            }
        }
        
        // Check if can be attacked by enemy pawns
        bool canBeAttacked = false;
        
        // Check adjacent files for attacking enemy pawns
        if (file > 0) {
            for (int r = IsWhite ? rank + 1 : rank - 1; 
                 IsWhite ? r <= 7 : r >= 0; 
                 IsWhite ? r++ : r--) {
                int attackSq = r * 8 + (file - 1);
                if (enemyPawns & (1ULL << attackSq)) {
                    canBeAttacked = true;
                    break;
                }
            }
        }
        
        if (file < 7) {
            for (int r = IsWhite ? rank + 1 : rank - 1; 
                 IsWhite ? r <= 7 : r >= 0; 
                 IsWhite ? r++ : r--) {
                int attackSq = r * 8 + (file + 1);
                if (enemyPawns & (1ULL << attackSq)) {
                    canBeAttacked = true;
                    break;
                }
            }
        }
        
        if (!canBeSupported && canBeAttacked) {
            backwardPawns |= 1ULL << sq;
        }
        
        pawnsCopy &= pawnsCopy - 1; // Clear the least significant bit
    }
    
    return backwardPawns;
}

template <bool IsWhite>
uint64_t identifyPawnChains(uint64_t pawns) {
    uint64_t pawnChains = 0;
    uint64_t pawnsCopy = pawns;
    
    while (pawnsCopy) {
        int sq = __builtin_ctzll(pawnsCopy);
        int file = sq % 8;
        int rank = sq / 8;
        
        // Check if pawn is supported by another pawn
        if (file > 0) {
            int supportSq = IsWhite ? (rank - 1) * 8 + (file - 1) : (rank + 1) * 8 + (file - 1);
            if ((IsWhite && rank > 0) || (!IsWhite && rank < 7)) {
                if (pawns & (1ULL << supportSq)) {
                    pawnChains |= 1ULL << sq;
                }
            }
        }
        
        if (file < 7) {
            int supportSq = IsWhite ? (rank - 1) * 8 + (file + 1) : (rank + 1) * 8 + (file + 1);
            if ((IsWhite && rank > 0) || (!IsWhite && rank < 7)) {
                if (pawns & (1ULL << supportSq)) {
                    pawnChains |= 1ULL << sq;
                }
            }
        }
        
        pawnsCopy &= pawnsCopy - 1; // Clear the least significant bit
    }
    
    return pawnChains;
}

template <bool IsWhite>
int evaluatePawnStorm(uint64_t friendlyPawns, uint64_t enemyPawns, int enemyKingSq) {
    int score = 0;
    int kingFile = enemyKingSq % 8;
    
    // Check pawns advancing towards enemy king
    for (int file = std::max(0, kingFile - 1); file <= std::min(7, kingFile + 1); file++) {
        uint64_t fileMask = 0x0101010101010101ULL << file;
        uint64_t friendlyPawnsOnFile = friendlyPawns & fileMask;
        
        if (friendlyPawnsOnFile) {
            int pawnSq = IsWhite ? 
                         63 - __builtin_clzll(friendlyPawnsOnFile) : 
                         __builtin_ctzll(friendlyPawnsOnFile);
            
            int pawnRank = pawnSq / 8;
            int enemyKingRank = enemyKingSq / 8;
            
            // Distance from pawn to enemy king's rank
            int rankDistance = IsWhite ? 
                              enemyKingRank - pawnRank : 
                              pawnRank - enemyKingRank;
            
            if (rankDistance > 0 && rankDistance < 4) {
                // The closer the pawn, the higher the bonus
                score += (4 - rankDistance) * 5;
                
                // Extra bonus for central files
                if (file >= 2 && file <= 5) {
                    score += 2;
                }
            }
        }
    }
    
    return score;
}

template <bool IsWhite>
int evaluatePawnStructure(const Board &brd, uint64_t friendlyPawns, uint64_t enemyPawns,uint64_t enemyKingSquare) {
    int mg_score = 0;
    int eg_score = 0;
    
    // Doubled pawns penalty
    uint64_t doubledPawns = 0;
    for (int file = 0; file < 8; file++) {
        uint64_t fileMask = 0x0101010101010101ULL << file;
        uint64_t pawnsOnFile = friendlyPawns & fileMask;
        int pawnCount = __builtin_popcountll(pawnsOnFile);
        if (pawnCount > 1) {
            mg_score -= 10 * (pawnCount - 1); // Penalty for each doubled pawn
            eg_score -= 20 * (pawnCount - 1); // Higher penalty in endgame
            doubledPawns |= pawnsOnFile;
        }
    }
    
    // Isolated pawns penalty
    uint64_t isolatedPawns = 0;
    for (int file = 0; file < 8; file++) {
        uint64_t fileMask = 0x0101010101010101ULL << file;
        uint64_t pawnsOnFile = friendlyPawns & fileMask;
        
        if (pawnsOnFile) {
            uint64_t adjacentFiles = 0;
            if (file > 0) adjacentFiles |= 0x0101010101010101ULL << (file - 1);
            if (file < 7) adjacentFiles |= 0x0101010101010101ULL << (file + 1);
            
            if (!(friendlyPawns & adjacentFiles)) {
                isolatedPawns |= pawnsOnFile;
                mg_score -= 20; // Penalty for isolated pawn
                eg_score -= 10; // Less penalty in endgame
            }
        }
    }
    
    // Backward pawns penalty
    uint64_t backwardPawns = identifyBackwardPawns<IsWhite>(friendlyPawns, enemyPawns);
    mg_score -= 8 * __builtin_popcountll(backwardPawns);
    eg_score -= 5 * __builtin_popcountll(backwardPawns);
    
    // Pawn chains bonus
    uint64_t pawnChains = identifyPawnChains<IsWhite>(friendlyPawns);
    mg_score += 5 * __builtin_popcountll(pawnChains);
    eg_score += 3 * __builtin_popcountll(pawnChains);
    
    // Pawn storm bonus/penalty
    if (mg_phase > 12) { // Only relevant in middlegame
        int pawnStormScore = evaluatePawnStorm<IsWhite>(friendlyPawns, enemyPawns, enemyKingSquare);
        mg_score += pawnStormScore;
    }
    
    int score = (mg_phase * mg_score + eg_phase * eg_score) / 24;
    return score;
}

template <bool IsWhite>
int evaluateKingSafety(const Board &brd, int kingSquare, uint64_t enemyPieces) {
    int mg_score = 0;
    int eg_score = 0;
    
    uint64_t kingZone = kingMasks[kingSquare];
    uint64_t enemyAttacksInKingZone = getAttacksInZone<IsWhite>(brd, kingZone);
    int attackCount = __builtin_popcountll(enemyAttacksInKingZone);
    
    // King safety penalty depends on number of attacking pieces
    if (attackCount > 0) {
        int attackWeight = std::min(attackCount * 10, 50); // Cap the penalty
        mg_score -= attackWeight;
        // Less important in endgame
        eg_score -= attackWeight / 2;
    }
    
    // Pawn shield bonus
    int pawnShieldBonus = evaluatePawnShield<IsWhite>(brd, kingSquare);
    mg_score += pawnShieldBonus;
    // Pawn shield less important in endgame
    eg_score += pawnShieldBonus / 3;
    
    // King on open file penalty
    if (isKingOnOpenFile<IsWhite>(brd, kingSquare)) {
        mg_score -= 25; // Significant penalty in middlegame
        eg_score -= 10; // Less penalty in endgame
    }
    
    // Castle status bonus
    bool hasCastled = hasKingCastled<IsWhite>(brd, kingSquare);
    if (hasCastled) {
        mg_score += 30; // Bonus for having castled
    } else if (canStillCastle<IsWhite>(brd)) {
        mg_score += 10; // Smaller bonus if castling is still an option
    } else {
        mg_score -= 15; // Penalty for losing castling rights without castling
    }
    
    int score = (mg_phase * mg_score + eg_phase * eg_score) / 24;
    return score;
}

template <bool IsWhite>
uint64_t getAttacksInZone(const Board &brd, uint64_t zone) {
    uint64_t attackedSquares = 0;
    uint64_t enemyKnights = IsWhite ? brd.BKnight : brd.WKnight;
    uint64_t enemyBishops = IsWhite ? brd.BBishop : brd.WBishop;
    uint64_t enemyRooks = IsWhite ? brd.BRook : brd.WRook;
    uint64_t enemyQueens = IsWhite ? brd.BQueen : brd.WQueen;
    
    // Knight attacks
    while (enemyKnights) {
        int sq = __builtin_ctzll(enemyKnights);
        attackedSquares |= knightMasks[sq] & zone;
        enemyKnights &= enemyKnights - 1;
    }
    
    // Bishop attacks
    while (enemyBishops) {
        int sq = __builtin_ctzll(enemyBishops);
        attackedSquares |= getBmagic(sq, brd.Occ) & zone;
        enemyBishops &= enemyBishops - 1;
    }
    
    // Rook attacks
    while (enemyRooks) {
        int sq = __builtin_ctzll(enemyRooks);
        attackedSquares |= getRmagic(sq, brd.Occ) & zone;
        enemyRooks &= enemyRooks - 1;
    }
    
    // Queen attacks
    while (enemyQueens) {
        int sq = __builtin_ctzll(enemyQueens);
        attackedSquares |= getQmagic(sq, brd.Occ) & zone;
        enemyQueens &= enemyQueens - 1;
    }
    
    return attackedSquares;
}

template <bool IsWhite>
int evaluatePawnShield(const Board &brd, int kingSquare) {
    int bonus = 0;
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    int kingFile = kingSquare % 8;
    int kingRank = kingSquare / 8;
    
    // Define the ranks in front of the king based on color
    int shieldRank1 = IsWhite ? kingRank + 1 : kingRank - 1;
    int shieldRank2 = IsWhite ? kingRank + 2 : kingRank - 2;
    
    // Check if these ranks are valid
    if ((IsWhite && (shieldRank1 > 7 || shieldRank2 > 7)) || 
        (!IsWhite && (shieldRank1 < 0 || shieldRank2 < 0))) {
        return 0; // King at edge of board
    }
    
    // Check pawns on the king's file and adjacent files
    for (int file = std::max(0, kingFile - 1); file <= std::min(7, kingFile + 1); file++) {
        // Check rank 1 in front of king
        int sq1 = shieldRank1 * 8 + file;
        if (friendlyPawns & (1ULL << sq1)) {
            bonus += 10; // Pawn directly in front of king
        } else {
            bonus -= 5; // Penalty for missing pawn
            
            // Check rank 2 in front of king - less valuable than rank 1
            int sq2 = shieldRank2 * 8 + file;
            if (friendlyPawns & (1ULL << sq2)) {
                bonus += 5; // Pawn two squares in front of king
            } else {
                bonus -= 3; // Smaller penalty for missing pawn
            }
        }
    }
    
    return bonus;
}

template <bool IsWhite>
bool isKingOnOpenFile(const Board &brd, int kingSquare) {
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    int kingFile = kingSquare % 8;
    
    // Create a mask for the king's file
    uint64_t fileMask = 0x0101010101010101ULL << kingFile;
    
    // Check if there are any friendly pawns on the king's file
    return (friendlyPawns & fileMask) == 0;
}

template <bool IsWhite>
bool hasKingCastled(const Board &brd, int kingSquare) {
    // Simplistic approach - check if king is on typical castled squares
    if (IsWhite) {
        return (kingSquare == 6 || kingSquare == 2); // g1 or c1
    } else {
        return (kingSquare == 62 || kingSquare == 58); // g8 or c8
    }
}

template <bool IsWhite>
bool canStillCastle(const Board &brd) {
    // This function should check castling rights using your board representation
    // Since I don't have access to your castling rights implementation,
    // this is a placeholder
    
    // Example implementation:
    // return (IsWhite ? (brd.castlingRights & WHITE_CASTLE_RIGHTS) : (brd.castlingRights & BLACK_CASTLE_RIGHTS));
    
    // Simple placeholder assuming kings on original squares might be able to castle
    uint64_t friendlyKing = IsWhite ? brd.WKing : brd.BKing;
    int expectedKingPos = IsWhite ? 4 : 60; // e1 or e8
    
    return (friendlyKing == (1ULL << expectedKingPos));
}

template <bool IsWhite>
int evaluateRooksOnOpenFiles(const Board &brd, uint64_t friendlyRooks, uint64_t friendlyPawns, uint64_t enemyPawns) {
    int score = 0;
    uint64_t rooksCopy = friendlyRooks;
    
    while (rooksCopy) {
        int sq = __builtin_ctzll(rooksCopy);
        int file = sq % 8;
        
        // Create a mask for this file
        uint64_t fileMask = 0x0101010101010101ULL << file;
        
        // Check if file is fully open (no pawns)
        if (!(friendlyPawns & fileMask) && !(enemyPawns & fileMask)) {
            score += 20; // Bonus for rook on fully open file
        }
        // Check if file is semi-open (no friendly pawns)
        else if (!(friendlyPawns & fileMask)) {
            score += 10; // Bonus for rook on semi-open file
        }
        
        rooksCopy &= rooksCopy - 1; // Clear the least significant bit
    }
    
    return score;
}

template <bool IsWhite>
int evaluateKnightOutposts(const Board &brd, uint64_t friendlyKnights, uint64_t friendlyPawns, uint64_t enemyPawns) {
    int score = 0;
    uint64_t knightsCopy = friendlyKnights;
    
    // Define outpost ranks (4th-6th rank for White, 3rd-5th for Black)
    uint64_t outpostRanks = IsWhite ? 0x0000FFFFFFFF0000ULL : 0x0000FFFFFF000000ULL;
    
    while (knightsCopy) {
        int sq = __builtin_ctzll(knightsCopy);
        uint64_t squareBit = 1ULL << sq;
        
        // Check if knight is in potential outpost position
        if (squareBit & outpostRanks) {
            int file = sq % 8;
            
            // Check if it can be attacked by enemy pawns
            bool canBeAttacked = false;
            
            // Create masks for adjacent files
            uint64_t adjacentFilesMask = 0;
            if (file > 0) adjacentFilesMask |= 0x0101010101010101ULL << (file - 1);
            if (file < 7) adjacentFilesMask |= 0x0101010101010101ULL << (file + 1);
            
            // Check for enemy pawns that could attack this square
            uint64_t attackingPawnsMask = adjacentFilesMask;
            if (IsWhite) {
                // For white knights, check for black pawns that could advance and capture
                attackingPawnsMask &= (squareBit >> 7) | (squareBit >> 9);
                attackingPawnsMask = attackingPawnsMask << 8; // Move mask one rank down
            } else {
                // For black knights, check for white pawns that could advance and capture
                attackingPawnsMask &= (squareBit << 7) | (squareBit << 9);
                attackingPawnsMask = attackingPawnsMask >> 8; // Move mask one rank up
            }
            
            // If no enemy pawns can attack, it's a strong outpost
            if (!(enemyPawns & attackingPawnsMask)) {
                // Check if supported by friendly pawn
                uint64_t supportingPawnsMask = 0;
                if (file > 0) supportingPawnsMask |= IsWhite ? (squareBit >> 9) : (squareBit << 7);
                if (file < 7) supportingPawnsMask |= IsWhite ? (squareBit >> 7) : (squareBit << 9);
                
                if (friendlyPawns & supportingPawnsMask) {
                    // Knight is in outpost and supported by pawn
                    score += 25;
                    
                    // Additional bonus for central outposts
                    if (file >= 2 && file <= 5) {
                        score += 10;
                    }
                } else {
                    // Knight is in outpost but not supported
                    score += 15;
                    
                    // Additional bonus for central outposts
                    if (file >= 2 && file <= 5) {
                        score += 5;
                    }
                }
            }
        }
        
        knightsCopy &= knightsCopy - 1; // Clear the least significant bit
    }
    
    return score;
}

template <bool IsWhite>
int evaluateDevelopment(const Board &brd) {
    int score = 0;
    
    uint64_t friendlyKnights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t friendlyBishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t friendlyRooks = IsWhite ? brd.WRook : brd.BRook;
    uint64_t friendlyQueens = IsWhite ? brd.WQueen : brd.BQueen;
    
    // Starting positions
    uint64_t knightStartPos = IsWhite ? 0x0000000000000042ULL : 0x4200000000000000ULL;
    uint64_t bishopStartPos = IsWhite ? 0x0000000000000024ULL : 0x2400000000000000ULL;
    uint64_t rookStartPos = IsWhite ? 0x0000000000000081ULL : 0x8100000000000000ULL;
    uint64_t queenStartPos = IsWhite ? 0x0000000000000008ULL : 0x0800000000000000ULL;
    
    // Penalty for undeveloped pieces
    if (friendlyKnights & knightStartPos) {
        score -= 10 * __builtin_popcountll(friendlyKnights & knightStartPos);
    }
    
    if (friendlyBishops & bishopStartPos) {
        score -= 10 * __builtin_popcountll(friendlyBishops & bishopStartPos);
    }
    
    // Smaller penalty for rooks as they typically develop later
    if (friendlyRooks & rookStartPos) {
        score -= 5 * __builtin_popcountll(friendlyRooks & rookStartPos);
    }
    
    // Early queen development might be a problem
    uint64_t centralSquares = 0x0000001818000000ULL; // e4, d4, e5, d5
    if (!(friendlyQueens & queenStartPos) && !(friendlyQueens & centralSquares)) {
        score -= 5; // Small penalty for early queen development unless it's to a central square
    }
    
    return score;
}

template <bool IsWhite>
int evaluatePassedPawns(const Board &brd, uint64_t friendlyPawns, uint64_t enemyPawns) {
    int score = 0;
    uint64_t pawnsCopy = friendlyPawns;
    
    while (pawnsCopy) {
        int sq = __builtin_ctzll(pawnsCopy);
        int file = sq % 8;
        int rank = sq / 8;
        
        // Create masks for the file and adjacent files
        uint64_t fileMask = 0x0101010101010101ULL << file;
        uint64_t adjacentFilesMask = 0;
        if (file > 0) adjacentFilesMask |= 0x0101010101010101ULL << (file - 1);
        if (file < 7) adjacentFilesMask |= 0x0101010101010101ULL << (file + 1);
        
        // Create mask for squares in front of the pawn
        uint64_t frontSquares = fileMask;
        if (IsWhite) {
            frontSquares &= ~((1ULL << (rank * 8 + file)) | ((1ULL << (rank * 8 + file)) - 1));
        } else {
            frontSquares &= ((1ULL << (rank * 8 + file + 1)) - 1);
        }
        
        // Create mask for front and diagonal squares that could block or attack the pawn
        uint64_t relevantSquares = frontSquares | (adjacentFilesMask & frontSquares);
        
        // Check if the pawn is passed
        if (!(enemyPawns & relevantSquares)) {
            // This is a passed pawn
            int rankFromPromotion = IsWhite ? 7 - rank : rank;
            
            // Base bonus depends on rank
            int passedBonus = (7 - rankFromPromotion) * 10;
            
            // Additional bonus for protected passed pawns
            if (isPawnProtected<IsWhite>(brd, sq)) {
                passedBonus += 5;
            }
            
            // Higher bonus for central passed pawns
            if (file >= 2 && file <= 5) {
                passedBonus += 5;
            }
            
            // Check if path to promotion is free
            if (!(brd.Occ & frontSquares)) {
                passedBonus += 5;
            }
            
            score += passedBonus;
        }
        
        pawnsCopy &= pawnsCopy - 1; // Clear the least significant bit
    }
    
    return score;
}

template <bool IsWhite>
bool isPawnProtected(const Board &brd, int sq) {
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    int file = sq % 8;
    int rank = sq / 8;
    
    // Check if pawn is protected by another pawn
    if (file > 0) {
        int supportSq = IsWhite ? (rank - 1) * 8 + (file - 1) : (rank + 1) * 8 + (file - 1);
        if ((IsWhite && rank > 0) || (!IsWhite && rank < 7)) {
            if (friendlyPawns & (1ULL << supportSq)) {
                return true;
            }
        }
    }
    
    if (file < 7) {
        int supportSq = IsWhite ? (rank - 1) * 8 + (file + 1) : (rank + 1) * 8 + (file + 1);
        if ((IsWhite && rank > 0) || (!IsWhite && rank < 7)) {
            if (friendlyPawns & (1ULL << supportSq)) {
                return true;
            }
        }
    }
    
    return false;
}

template <bool IsWhite>
int evaluateMaterialImbalance(const Board &brd) {
    int score = 0;
    
    int friendlyKnightCount = __builtin_popcountll(IsWhite ? brd.WKnight : brd.BKnight);
    int friendlyBishopCount = __builtin_popcountll(IsWhite ? brd.WBishop : brd.BBishop);
    int friendlyRookCount = __builtin_popcountll(IsWhite ? brd.WRook : brd.BRook);
    int friendlyQueenCount = __builtin_popcountll(IsWhite ? brd.WQueen : brd.BQueen);
    int friendlyPawnCount = __builtin_popcountll(IsWhite ? brd.WPawn : brd.BPawn);
    
    int enemyKnightCount = __builtin_popcountll(IsWhite ? brd.BKnight : brd.WKnight);
    int enemyBishopCount = __builtin_popcountll(IsWhite ? brd.BBishop : brd.WBishop);
    int enemyRookCount = __builtin_popcountll(IsWhite ? brd.BRook : brd.WRook);
    int enemyQueenCount = __builtin_popcountll(IsWhite ? brd.BQueen : brd.WQueen);
    int enemyPawnCount = __builtin_popcountll(IsWhite ? brd.BPawn : brd.WPawn);
    
    // Bishop pair bonus
    if (friendlyBishopCount >= 2) {
        score += 50;
    }
    
    // Knight pair bonus (smaller than bishop pair)
    if (friendlyKnightCount >= 2) {
        score += 10;
    }
    
    // Bonus for rook pair
    if (friendlyRookCount >= 2) {
        score += 10;
    }
    
    // Knight is better in closed positions (lots of pawns)
    if (friendlyKnightCount > enemyKnightCount && friendlyPawnCount + enemyPawnCount > 12) {
        score += 10 * (friendlyKnightCount - enemyKnightCount);
    }
    
    // Bishop is better in open positions (few pawns)
    if (friendlyBishopCount > enemyBishopCount && friendlyPawnCount + enemyPawnCount < 10) {
        score += 15 * (friendlyBishopCount - enemyBishopCount);
    }
    
    // Rook is better with few enemy pawns
    if (friendlyRookCount > 0 && enemyPawnCount < 4) {
        score += 10 * friendlyRookCount;
    }
    
    // Knight and bishop work well together
    if (friendlyKnightCount > 0 && friendlyBishopCount > 0) {
        score += 15;
    }
    
    // Queen and knight work well together
    if (friendlyQueenCount > 0 && friendlyKnightCount > 0) {
        score += 10;
    }
    
    return score;
}

template <bool IsWhite>
int evaluateSpaceAdvantage(const Board &brd, uint64_t friendlyPawns, uint64_t friendlyPieces) {
    int score = 0;
    
    // Space is defined as squares on ranks 3-6 for white or 3-6 for black
    // that are not attacked by enemy pawns and are either empty or occupied by friendly pieces
    uint64_t centerRanks = 0x00FFFFFFFF000000ULL; // Ranks 3-6
    
    // Count controlled squares in center
    uint64_t controlledSquares = getControlledSquares<IsWhite>(brd) & centerRanks;
    int controlCount = __builtin_popcountll(controlledSquares);
    
    // Space advantage is more important in closed positions
    int pawnCount = __builtin_popcountll(brd.WPawn | brd.BPawn);
    int spaceFactor = pawnCount > 10 ? 2 : 1; // Double importance in closed positions
    
    score += controlCount * spaceFactor;
    
    return score;
}

template <bool IsWhite>
uint64_t getControlledSquares(const Board &brd) {
    uint64_t controlledSquares = 0;
    
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    uint64_t friendlyKnights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t friendlyBishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t friendlyRooks = IsWhite ? brd.WRook : brd.BRook;
    uint64_t friendlyQueens = IsWhite ? brd.WQueen : brd.BQueen;
    uint64_t friendlyKing = IsWhite ? brd.WKing : brd.BKing;
    
    // Pawn attacks
    uint64_t pawnsCopy = friendlyPawns;
    while (pawnsCopy) {
        int sq = __builtin_ctzll(pawnsCopy);
        
        // Add pawn attack squares based on color
        if (IsWhite) {
            if (sq % 8 > 0) controlledSquares |= 1ULL << (sq + 7); // Up-left
            if (sq % 8 < 7) controlledSquares |= 1ULL << (sq + 9); // Up-right
        } else {
            if (sq % 8 > 0) controlledSquares |= 1ULL << (sq - 9); // Down-left
            if (sq % 8 < 7) controlledSquares |= 1ULL << (sq - 7); // Down-right
        }
        
        pawnsCopy &= pawnsCopy - 1;
    }
    
    // Knight attacks
    uint64_t knightsCopy = friendlyKnights;
    while (knightsCopy) {
        int sq = __builtin_ctzll(knightsCopy);
        controlledSquares |= knightMasks[sq];
        knightsCopy &= knightsCopy - 1;
    }
    
    // Bishop attacks
    uint64_t bishopsCopy = friendlyBishops;
    while (bishopsCopy) {
        int sq = __builtin_ctzll(bishopsCopy);
        controlledSquares |= getBmagic(sq, brd.Occ);
        bishopsCopy &= bishopsCopy - 1;
    }
    
    // Rook attacks
    uint64_t rooksCopy = friendlyRooks;
    while (rooksCopy) {
        int sq = __builtin_ctzll(rooksCopy);
        controlledSquares |= getRmagic(sq, brd.Occ);
        rooksCopy &= rooksCopy - 1;
    }
    
    // Queen attacks
    uint64_t queensCopy = friendlyQueens;
    while (queensCopy) {
        int sq = __builtin_ctzll(queensCopy);
        controlledSquares |= getQmagic(sq, brd.Occ);
        queensCopy &= queensCopy - 1;
    }
    
    // King attacks
    if (friendlyKing) {
        int sq = __builtin_ctzll(friendlyKing);
        // Add king movement masks (implementation may vary)
        controlledSquares |= kingMasks[sq];
    }
    
    return controlledSquares;
}


inline int calculateManhattanDistance(int sq1, int sq2) {
    int file1 = sq1 % 8;
    int rank1 = sq1 / 8;
    int file2 = sq2 % 8;
    int rank2 = sq2 / 8;
    
    return abs(file1 - file2) + abs(rank1 - rank2);
}

template <bool IsWhite>
int evaluateKingTropism(const Board &brd, int kingSquare, uint64_t enemyKnights, uint64_t enemyBishops, uint64_t enemyRooks, uint64_t enemyQueens) {
    int penalty = 0;
    
    // Process knights
    uint64_t knightsCopy = enemyKnights;
    while (knightsCopy) {
        int sq = __builtin_ctzll(knightsCopy);
        int distance = calculateManhattanDistance(sq, kingSquare);
        
        // Knight tropism is more dangerous when close
        if (distance <= 4) {
            penalty += (5 - distance) * 5;
        }
        
        knightsCopy &= knightsCopy - 1;
    }
    
    // Process bishops (slightly less dangerous than knights at close range)
    uint64_t bishopsCopy = enemyBishops;
    while (bishopsCopy) {
        int sq = __builtin_ctzll(bishopsCopy);
        int distance = calculateManhattanDistance(sq, kingSquare);
        
        if (distance <= 4) {
            penalty += (5 - distance) * 3;
        }
        
        bishopsCopy &= bishopsCopy - 1;
    }
    
    // Process rooks
    uint64_t rooksCopy = enemyRooks;
    while (rooksCopy) {
        int sq = __builtin_ctzll(rooksCopy);
        int distance = calculateManhattanDistance(sq, kingSquare);
        
        if (distance <= 3) {
            penalty += (4 - distance) * 4;
        }
        
        rooksCopy &= rooksCopy - 1;
    }
    
    // Process queens (most dangerous)
    uint64_t queensCopy = enemyQueens;
    while (queensCopy) {
        int sq = __builtin_ctzll(queensCopy);
        int distance = calculateManhattanDistance(sq, kingSquare);
        
        if (distance <= 5) {
            penalty += (6 - distance) * 8;
        }
        
        queensCopy &= queensCopy - 1;
    }
    
    return penalty;
}

template <bool IsWhite>
int evaluatePieceCoordination(const Board &brd) {
    int score = 0;
    
    uint64_t friendlyPieces = IsWhite ? brd.White : brd.Black;
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    uint64_t friendlyKnights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t friendlyBishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t friendlyRooks = IsWhite ? brd.WRook : brd.BRook;
    uint64_t friendlyQueens = IsWhite ? brd.WQueen : brd.BQueen;
    
    // Count protecting relationships between pieces
    int protectCount = 0;
    
    // Check knights protecting pieces
    uint64_t knightsCopy = friendlyKnights;
    while (knightsCopy) {
        int sq = __builtin_ctzll(knightsCopy);
        uint64_t knightAttacks = knightMasks[sq];
        protectCount += __builtin_popcountll(knightAttacks & friendlyPieces & ~friendlyPawns);
        knightsCopy &= knightsCopy - 1;
    }
    
    // Check bishops protecting pieces
    uint64_t bishopsCopy = friendlyBishops;
    while (bishopsCopy) {
        int sq = __builtin_ctzll(bishopsCopy);
        uint64_t bishopAttacks = getBmagic(sq, brd.Occ);
        protectCount += __builtin_popcountll(bishopAttacks & friendlyPieces & ~friendlyPawns);
        bishopsCopy &= bishopsCopy - 1;
    }
    
    // Check rooks protecting pieces
    uint64_t rooksCopy = friendlyRooks;
    while (rooksCopy) {
        int sq = __builtin_ctzll(rooksCopy);
        uint64_t rookAttacks = getRmagic(sq, brd.Occ);
        protectCount += __builtin_popcountll(rookAttacks & friendlyPieces & ~friendlyPawns);
        rooksCopy &= rooksCopy - 1;
    }
    
    // Check queens protecting pieces
    uint64_t queensCopy = friendlyQueens;
    while (queensCopy) {
        int sq = __builtin_ctzll(queensCopy);
        uint64_t queenAttacks = getQmagic(sq, brd.Occ);
        protectCount += __builtin_popcountll(queenAttacks & friendlyPieces & ~friendlyPawns);
        queensCopy &= queensCopy - 1;
    }
    
    // Bonus for protected pieces
    score += protectCount * 2;
    
    return score;
}


template <bool IsWhite>
int staticEval(const Board &brd) {
    int mg_score = 0;
    int eg_score = 0;
    
    // Get base piece and mobility evaluations from existing functions
    int mobilityScore = evalMobility<IsWhite>(brd);
    
    // Determine side to move bitboards
    uint64_t friendlyPieces = IsWhite ? brd.White : brd.Black;
    uint64_t enemyPieces = IsWhite ? brd.Black : brd.White;
    
    uint64_t friendlyPawns = IsWhite ? brd.WPawn : brd.BPawn;
    uint64_t enemyPawns = IsWhite ? brd.BPawn : brd.WPawn;
    
    uint64_t friendlyKing = IsWhite ? brd.WKing : brd.BKing;
    uint64_t enemyKing = IsWhite ? brd.BKing : brd.WKing;
    
    uint64_t friendlyKnights = IsWhite ? brd.WKnight : brd.BKnight;
    uint64_t friendlyBishops = IsWhite ? brd.WBishop : brd.BBishop;
    uint64_t friendlyRooks = IsWhite ? brd.WRook : brd.BRook;
    uint64_t friendlyQueens = IsWhite ? brd.WQueen : brd.BQueen;
    
    uint64_t enemyKnights = IsWhite ? brd.BKnight : brd.WKnight;
    uint64_t enemyBishops = IsWhite ? brd.BBishop : brd.WBishop;
    uint64_t enemyRooks = IsWhite ? brd.BRook : brd.WRook;
    uint64_t enemyQueens = IsWhite ? brd.BQueen : brd.WQueen;
    
    // King safety evaluation
    int kingSquare = __builtin_ctzll(friendlyKing);
    int enemyKingSquare = __builtin_ctzll(enemyKing);
    
    // Evaluate pawn structure
    int pawnStructureScore = evaluatePawnStructure<IsWhite>(brd, friendlyPawns, enemyPawns, enemyKingSquare);
    
    // King safety evaluation
    int kingSafetyScore = evaluateKingSafety<IsWhite>(brd, kingSquare, enemyPieces);
    
    // Bishop pair bonus
    int bishopPairBonus = 0;
    if (__builtin_popcountll(friendlyBishops) >= 2) {
        bishopPairBonus = 50; // Bonus for having bishop pair in middlegame
    }
    
    // Rook on open file bonus
    /*int rookOpenFileBonus = evaluateRooksOnOpenFiles<IsWhite>(brd, friendlyRooks, friendlyPawns, enemyPawns);
    
    // Knights in outposts
    int knightOutpostBonus = evaluateKnightOutposts<IsWhite>(brd, friendlyKnights, friendlyPawns, enemyPawns);
    
    // Development and piece activity in opening/middlegame
    int developmentScore = 0;
    if (mg_phase > 18) { // If we're in the opening or early middlegame
        developmentScore = evaluateDevelopment<IsWhite>(brd);
    }
    
    // Passed pawns evaluation
    int passedPawnScore = evaluatePassedPawns<IsWhite>(brd, friendlyPawns, enemyPawns);
    
    // Material imbalance
    int materialImbalanceScore = evaluateMaterialImbalance<IsWhite>(brd);
    
    // Space advantage
    int spaceAdvantage = evaluateSpaceAdvantage<IsWhite>(brd, friendlyPawns, friendlyPieces);
    
    // Connected rooks bonus
    int connectedRooksBonus = 0;
    if (__builtin_popcountll(friendlyRooks) >= 2) {
        // Check if rooks are on the same rank or file
        if (areRooksConnected(friendlyRooks)) {
            connectedRooksBonus = 20;
        }
    }
    
    // King tropism (distance of enemy pieces to our king)
    int kingTropismPenalty = evaluateKingTropism<IsWhite>(brd, kingSquare, enemyKnights, enemyBishops, enemyRooks, enemyQueens);
    
    // Piece coordination - density of friendly pieces protecting each other
    int pieceCoordinationScore = evaluatePieceCoordination<IsWhite>(brd);
    
    // Calculate total score
    int totalMgScore = mobilityScore + pawnStructureScore + kingSafetyScore + 
                      bishopPairBonus + rookOpenFileBonus + knightOutpostBonus + 
                      developmentScore + (passedPawnScore / 2) + materialImbalanceScore + 
                      spaceAdvantage + connectedRooksBonus - kingTropismPenalty + pieceCoordinationScore;
    
    int totalEgScore = mobilityScore + pawnStructureScore + (kingSafetyScore / 2) + 
                      (bishopPairBonus / 2) + rookOpenFileBonus + (knightOutpostBonus / 2) + 
                      passedPawnScore + materialImbalanceScore - kingTropismPenalty + 
                      (pieceCoordinationScore / 2);
    
    // Phase-dependent scoring*/
    int totalMgScore = pawnStructureScore + kingSafetyScore + bishopPairBonus;
    int totalEgScore = pawnStructureScore + (kingSafetyScore / 2) + (bishopPairBonus / 2);


    int finalScore = (mg_phase * totalMgScore + eg_phase * totalEgScore) / 24 + mobilityScore;
    
    return mobilityScore;
}


