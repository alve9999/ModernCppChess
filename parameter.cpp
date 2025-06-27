#include "parameter.hpp"

int KILLER_MOVE_BONUS = 13000;
int COUNTER_HISTORY_BONUS = 6500;
int FOLLOW_UP_BONUS = 3000;
int DELTA_MARGIN = 800;
int DELTA_INIT = 200;
int RFP_MARGIN = 70;
int FP_BASE = 200;
int FP_ADD = 100;
int WINDOW_INIT = 15;
int WINDOW_MULT = 2;
double HISTORY_AGE_FACTOR = 1.1;

void setValueFromCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd, name;
    double value;

    iss >> cmd >> name >> value;
    if (cmd != "setvalue") return;

    if      (name == "KILLER_MOVE_BONUS")     KILLER_MOVE_BONUS = static_cast<int>(value);
    else if (name == "COUNTER_HISTORY_BONUS") COUNTER_HISTORY_BONUS = static_cast<int>(value);
    else if (name == "DELTA_MARGIN")          DELTA_MARGIN = static_cast<int>(value);
    else if (name == "DELTA_INIT")            DELTA_INIT = static_cast<int>(value);
    else if (name == "RFP_MARGIN")             RFP_MARGIN = static_cast<int>(value);
    else if (name == "FP_BASE")               FP_BASE = static_cast<int>(value);
    else if (name == "FP_ADD")                FP_ADD = static_cast<int>(value);
    else if (name == "WINDOW_INIT")           WINDOW_INIT = static_cast<int>(value);
    else if (name == "WINDOW_MULT")           WINDOW_MULT = static_cast<int>(value);
    else if (name == "FOLLOW_UP_BONUS")       FOLLOW_UP_BONUS = static_cast<int>(value);
    else if (name == "HISTORY_AGE_FACTOR")    HISTORY_AGE_FACTOR = value;
    else std::cerr << "Unknown parameter: " << name << "\n";
}
