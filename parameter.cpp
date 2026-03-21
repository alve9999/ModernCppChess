#include "parameter.hpp"

int KILLER_MOVE_BONUS = 10715;
int COUNTER_HISTORY_BONUS = 6001;
int FOLLOW_UP_BONUS = 3504;
// int DELTA_MARGIN = 800;
// int DELTA_INIT = 200;
int RFP_MARGIN = 52;
int FP_BASE = 244;
int FP_ADD = 114;
int WINDOW_INIT = 7;
int WINDOW_MULT = 2;
int SEE_MULTIPLIER = 74;
double HISTORY_AGE_FACTOR = 1.1;
int RFP_DEPTH = 4;
int TT_PROBE_MIN_DEPTH = 1;
int NMP_BASE = 3;
int NMP_DEPTH_DIV = 5;
int NMP_SCORE_DIV = 204;
int NMP_SCORE_MAX = 2;
int TT_MOVE_BONUS = 879949;
int PV_MOVE_BONUS = 2158520;
int FP_DEPTH = 2;
int LMP_DEPTH_MAX = 8;
int LMP_SCALE = 177;
int LMR_DEPTH_MIN = 2;
double LMR_BASE = 0.75;
double LMR_DIV = 1.30;
int LMR_HIST_MAX = 2;
int LMR_HIST_DIV = 4224;
int PROMOTE = 78490;
int CASTLE = 3737;
int EP_VAL = 9261;
int CAPTURE = 70305;

void setValueFromCommand(const std::string &command) {
    std::istringstream iss(command);
    std::string cmd, name;
    double value;

    iss >> cmd >> name >> value;
    if (cmd != "setvalue")
        return;

    if (name == "KILLER_MOVE_BONUS")
        KILLER_MOVE_BONUS = static_cast<int>(value);
    else if (name == "COUNTER_HISTORY_BONUS")
        COUNTER_HISTORY_BONUS = static_cast<int>(value);
    // else if (name == "DELTA_MARGIN")          DELTA_MARGIN =
    // static_cast<int>(value); else if (name == "DELTA_INIT") DELTA_INIT =
    // static_cast<int>(value);
    else if (name == "SEE_MULTIPLIER")
        SEE_MULTIPLIER = static_cast<int>(value);
    else if (name == "RFP_MARGIN")
        RFP_MARGIN = static_cast<int>(value);
    else if (name == "FP_BASE")
        FP_BASE = static_cast<int>(value);
    else if (name == "FP_ADD")
        FP_ADD = static_cast<int>(value);
    else if (name == "WINDOW_INIT")
        WINDOW_INIT = static_cast<int>(value);
    else if (name == "WINDOW_MULT")
        WINDOW_MULT = static_cast<int>(value);
    else if (name == "FOLLOW_UP_BONUS")
        FOLLOW_UP_BONUS = static_cast<int>(value);
    else if (name == "HISTORY_AGE_FACTOR")
        HISTORY_AGE_FACTOR = value;
    else if (name == "RFP_DEPTH")
        RFP_DEPTH = static_cast<int>(value);
    else if (name == "TT_PROBE_MIN_DEPTH")
        TT_PROBE_MIN_DEPTH = static_cast<int>(value);
    else if (name == "NMP_BASE")
        NMP_BASE = static_cast<int>(value);
    else if (name == "NMP_DEPTH_DIV")
        NMP_DEPTH_DIV = static_cast<int>(value);
    else if (name == "NMP_SCORE_DIV")
        NMP_SCORE_DIV = static_cast<int>(value);
    else if (name == "NMP_SCORE_MAX")
        NMP_SCORE_MAX = static_cast<int>(value);
    else if (name == "TT_MOVE_BONUS")
        TT_MOVE_BONUS = static_cast<int>(value);
    else if (name == "PV_MOVE_BONUS")
        PV_MOVE_BONUS = static_cast<int>(value);
    else if (name == "FP_DEPTH")
        FP_DEPTH = static_cast<int>(value);
    else if (name == "LMP_DEPTH_MAX")
        LMP_DEPTH_MAX = static_cast<int>(value);
    else if (name == "LMP_SCALE")
        LMP_SCALE = static_cast<int>(value);
    else if (name == "LMR_DEPTH_MIN")
        LMR_DEPTH_MIN = static_cast<int>(value);
    else if (name == "LMR_BASE")
        LMR_BASE = static_cast<int>(value);
    else if (name == "LMR_DIV")
        LMR_DIV = static_cast<int>(value);
    else if (name == "LMR_HIST_MAX")
        LMR_HIST_MAX = static_cast<int>(value);
    else if (name == "LMR_HIST_DIV")
        LMR_HIST_DIV = static_cast<int>(value);
    else if (name == "PROMOTE")
        PROMOTE = static_cast<int>(value);
    else if (name == "CASTLE")
        CASTLE = static_cast<int>(value);
    else if (name == "EP_VAL")
        EP_VAL = static_cast<int>(value);
    else if (name == "CAPTURE")
        CAPTURE = static_cast<int>(value);
    // else std::cerr << "Unknown parameter: " << name << "\n";
}
void printUCIOptions() {
    std::cout << "option name Hash type spin default " << 8 << " min 8 max 8\n";
    std::cout << "option name Threads type spin default " << 1
              << " min 1 max 1\n";
    std::cout << "option name KILLER_MOVE_BONUS type spin default "
              << KILLER_MOVE_BONUS << " min 0 max 1000000\n";
    std::cout << "option name COUNTER_HISTORY_BONUS type spin default "
              << COUNTER_HISTORY_BONUS << " min 0 max 1000000\n";
    std::cout << "option name SEE_MULTIPLIER type spin default "
              << SEE_MULTIPLIER << " min 0 max 1000\n";
    std::cout << "option name FOLLOW_UP_BONUS type spin default "
              << FOLLOW_UP_BONUS << " min 0 max 1000000\n";
    // std::cout << "option name DELTA_MARGIN type spin default " <<
    // DELTA_MARGIN << " min 0 max 10000\n"; std::cout << "option name
    // DELTA_INIT type spin default " << DELTA_INIT << " min 0 max 10000\n";
    std::cout << "option name RFP_MARGIN type spin default " << RFP_MARGIN
              << " min 0 max 500\n";
    std::cout << "option name FP_BASE type spin default " << FP_BASE
              << " min 0 max 1000\n";
    std::cout << "option name FP_ADD type spin default " << FP_ADD
              << " min 0 max 1000\n";
    std::cout << "option name WINDOW_INIT type spin default " << WINDOW_INIT
              << " min 0 max 100\n";
    std::cout << "option name WINDOW_MULT type spin default " << WINDOW_MULT
              << " min 0 max 10\n";
    std::cout << "option name HISTORY_AGE_FACTOR type string default "
              << HISTORY_AGE_FACTOR << "\n";
    std::cout << "option name RFP_DEPTH type spin default " << RFP_DEPTH
              << " min 0 max 20\n";
    std::cout << "option name TT_PROBE_MIN_DEPTH type spin default "
              << TT_PROBE_MIN_DEPTH << " min 0 max 20\n";
    std::cout << "option name NMP_BASE type spin default " << NMP_BASE
              << " min 0 max 20\n";
    std::cout << "option name NMP_DEPTH_DIV type spin default " << NMP_DEPTH_DIV
              << " min 1 max 20\n";
    std::cout << "option name NMP_SCORE_DIV type spin default " << NMP_SCORE_DIV
              << " min 1 max 1000\n";
    std::cout << "option name NMP_SCORE_MAX type spin default " << NMP_SCORE_MAX
              << " min 1 max 20\n";
    std::cout << "option name TT_MOVE_BONUS type spin default " << TT_MOVE_BONUS
              << " min 0 max 5000000\n";
    std::cout << "option name PV_MOVE_BONUS type spin default " << PV_MOVE_BONUS
              << " min 0 max 5000000\n";
    std::cout << "option name FP_DEPTH type spin default " << FP_DEPTH
              << " min 0 max 20\n";
    std::cout << "option name LMP_DEPTH_MAX type spin default " << LMP_DEPTH_MAX
              << " min 0 max 20\n";
    std::cout << "option name LMP_SCALE type spin default " << LMP_SCALE
              << " min 0 max 200\n";
    std::cout << "option name LMR_DEPTH_MIN type spin default " << LMR_DEPTH_MIN
              << " min 0 max 20\n";
    std::cout << "option name LMR_BASE type string default " << LMR_BASE
              << "\n";
    std::cout << "option name LMR_DIV type string default " << LMR_DIV << "\n";
    std::cout << "option name LMR_HIST_MAX type spin default " << LMR_HIST_MAX
              << " min 0 max 20\n";
    std::cout << "option name LMR_HIST_DIV type spin default " << LMR_HIST_DIV
              << " min 1 max 10000\n";
    std::cout << "option name PROMOTE type spin default " << PROMOTE
              << " min 0 max 1000000\n";
    std::cout << "option name CASTLE type spin default " << CASTLE
              << " min 0 max 1000000\n";
    std::cout << "option name EP_VAL type spin default " << EP_VAL
              << " min 0 max 1000000\n";
    std::cout << "option name CAPTURE type spin default " << CAPTURE
              << " min 0 max 1000000\n";
}
