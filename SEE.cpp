#include "SEE.hpp"


inline uint64_t getAttackers(const Board& brd, int square) noexcept {
    uint64_t occ = brd.Occ;
    uint64_t attackers = 0;
    
    uint64_t whitePawns = brd.WPawn;
    uint64_t blackPawns = brd.BPawn;
    
    attackers |= ((1ULL << square) >> 7) & ~File1 & whitePawns;
    attackers |= ((1ULL << square) >> 9) & ~File8 & whitePawns;
    
    attackers |= ((1ULL << square) << 7) & ~File8 & blackPawns;
    attackers |= ((1ULL << square) << 9) & ~File1 & blackPawns;
    
    attackers |= knightMasks[square] & (brd.WKnight | brd.BKnight);
    
    attackers |= kingMasks[square] & (brd.WKing | brd.BKing);
   
    uint64_t rookAttacks = getRmagic(square, occ);
    attackers |= rookAttacks & (brd.WRook | brd.BRook | 
                               brd.WQueen | brd.BQueen);
    
    uint64_t bishopAttacks = getBmagic(square, occ);
    attackers |= bishopAttacks & (brd.WBishop | brd.BBishop | 
                                 brd.WQueen | brd.BQueen);
    
    return attackers;
}

inline int getLeastValuableAttacker(const Board& brd, int square, bool isWhite, uint64_t& attackers, int& pieceType) noexcept {
    uint64_t sideAttackers = attackers & (isWhite ? brd.White : brd.Black);
    if (!sideAttackers) return -1;
    
    uint64_t pawnAttackers = sideAttackers & (isWhite ? brd.WPawn : brd.BPawn);
    if (pawnAttackers) {
        pieceType = 0;
        int attackerSquare = __builtin_ctzll(pawnAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    uint64_t knightAttackers = sideAttackers & (isWhite ? brd.WKnight : brd.BKnight);
    if (knightAttackers) {
        pieceType = 1;
        int attackerSquare = __builtin_ctzll(knightAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    uint64_t bishopAttackers = sideAttackers & (isWhite ? brd.WBishop : brd.BBishop);
    if (bishopAttackers) {
        pieceType = 2;
        int attackerSquare = __builtin_ctzll(bishopAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    uint64_t rookAttackers = sideAttackers & (isWhite ? brd.WRook : brd.BRook);
    if (rookAttackers) {
        pieceType = 3;
        int attackerSquare = __builtin_ctzll(rookAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    uint64_t queenAttackers = sideAttackers & (isWhite ? brd.WQueen : brd.BQueen);
    if (queenAttackers) {
        pieceType = 4;
        int attackerSquare = __builtin_ctzll(queenAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    uint64_t kingAttackers = sideAttackers & (isWhite ? brd.WKing : brd.BKing);
    if (kingAttackers) {
        pieceType = 5;
        int attackerSquare = __builtin_ctzll(kingAttackers);
        attackers &= ~(1ULL << attackerSquare);
        return attackerSquare;
    }
    
    return -1;
}


inline void updateAttackers(uint64_t& attackers, const Board& brd, uint64_t& occ, int fromSquare, int toSquare, int piece) noexcept {
    // fromSquare has already been cleared from occ before this call;
    // reveal any x-ray pieces that were hiding behind fromSquare by
    // re-scanning rook/bishop rays from toSquare through the updated occ.
    (void)toSquare; // rays are cast from toSquare but reveal pieces behind fromSquare

    if (piece == 3 || piece == 4 || piece == 5 || piece == 0) {
        uint64_t rookAttacks = getRmagic(toSquare, occ);
        attackers |= rookAttacks & (brd.WRook | brd.BRook | 
                                   brd.WQueen | brd.BQueen) & occ;
    }
    
    if (piece == 2 || piece == 4 || piece == 5 || piece == 0) {
        uint64_t bishopAttacks = getBmagic(toSquare, occ);
        attackers |= bishopAttacks & (brd.WBishop | brd.BBishop | 
                                     brd.WQueen | brd.BQueen) & occ;
    }
}

inline int getMovingPieceType(const Board& brd, int sq) noexcept {
    uint64_t fromBit = 1ULL << sq;
    
    if (brd.White & fromBit) {
        if (brd.WPawn & fromBit) return 0;
        if (brd.WKnight & fromBit) return 1;
        if (brd.WBishop & fromBit) return 2;
        if (brd.WRook & fromBit) return 3;
        if (brd.WQueen & fromBit) return 4;
        if (brd.WKing & fromBit) return 5;
    }
    else if (brd.Black & fromBit) {
        if (brd.BPawn & fromBit) return 0;
        if (brd.BKnight & fromBit) return 1;
        if (brd.BBishop & fromBit) return 2;
        if (brd.BRook & fromBit) return 3;
        if (brd.BQueen & fromBit) return 4;
        if (brd.BKing & fromBit) return 5;
    }
    
    return -1;
}

constexpr int seePieceValues[6] = {
    100, 320, 330, 500, 900, 20000
};

// Returns SEE score minus threshold. Positive means the capture wins material
// relative to threshold (e.g. threshold=0 means any winning/equal capture).
int SEE(const Board& brd, int from, int to, int threshold = 0) noexcept {
    int movingPieceType = getMovingPieceType(brd, from);
    if (movingPieceType == -1) return 0;
    
    bool isWhiteMoving = brd.White & (1ULL << from);
    int capturedPieceType = getMovingPieceType(brd, to);
    
    uint64_t occ = brd.Occ;
    uint64_t attackers = getAttackers(brd, to);

    int seeValues[32];
    int depth = 0;

    // Promotion: pawn becomes a queen on the target square.
    // Gain = queen value + whatever was captured there.
    if (movingPieceType == 0 && (to <= 7 || to >= 56)) {
        seeValues[0] = seePieceValues[4] + (capturedPieceType != -1 ? seePieceValues[capturedPieceType] : 0);
        movingPieceType = 4; // the piece on the square is now a queen
    }
    else if (capturedPieceType == -1) {
        seeValues[0] = 0;
    }
    else {
        seeValues[0] = seePieceValues[capturedPieceType];
    }

    // Remove the moving piece from occ/attackers, then reveal x-rays behind it.
    occ      &= ~(1ULL << from);
    attackers &= ~(1ULL << from);
    updateAttackers(attackers, brd, occ, from, to, movingPieceType);
    
    int lastPieceType = movingPieceType;
    bool side = !isWhiteMoving;

    while (attackers) {
        int nextPieceType = -1;
        int nextAttackerSquare = getLeastValuableAttacker(brd, to, side, attackers, nextPieceType);
        if (nextAttackerSquare == -1) break;
        
        depth++;
        // Standing pat: this side captures lastPieceType and risks nextPieceType.
        seeValues[depth] = seePieceValues[lastPieceType] - seeValues[depth - 1];
        lastPieceType = nextPieceType;
        
        occ      &= ~(1ULL << nextAttackerSquare);
        attackers &= ~(1ULL << nextAttackerSquare);
        updateAttackers(attackers, brd, occ, nextAttackerSquare, to, nextPieceType);
        
        side = !side;
    }

    // Negamax rollback: each side can choose not to capture (stand pat).
    for (int i = depth; i > 0; i--) {
        seeValues[i-1] = -std::max(-seeValues[i-1], seeValues[i]);
    }

    return seeValues[0] - threshold;
}
