#include "board.hpp"
#include "eval.hpp"
#include "movegen.hpp"
#include "uci.hpp"
#include <chrono>
#include <iostream>
#include "SEE.hpp"
#include <memory>
#include "uci.hpp"
#include "ai.hpp"
#include "hash.hpp"
#include "move.hpp"
#include "eval.hpp"
#include "movegen.hpp"
int main() {
/*
    std::vector<std::string> fens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "6k1/5ppp/8/8/8/5Q2/5PPP/6K1 w - - 0 1",
        "6kq/5ppp/8/8/8/8/5PPP/6K1 w - - 0 1",
    };


    for (const auto& fen : fens) {
        auto brd = std::make_unique<Board>(loadFenBoard(fen));
        auto state = std::make_unique<BoardState>(parseBoardState(fen.c_str()));
        AccumulatorPair* accPair = (AccumulatorPair*)malloc(sizeof(AccumulatorPair));
        nnue_init(accPair, *brd);
        int side_to_move = state->IsWhite;
        int score = nnue_evaluate(accPair, side_to_move);

        std::cout << "FEN: " << fen << "\nScore: " << score << " cp\n\n";
    }

    return 0;*/
    uciRunGame();
}
