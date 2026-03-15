#include <string>
#include <sstream>
#include <iostream>

extern int COUNTER_HISTORY_BONUS;
extern int FOLLOW_UP_BONUS;
extern int KILLER_MOVE_BONUS;
//extern int DELTA_MARGIN;
//extern int DELTA_INIT;
extern int RFP_MARGIN;
extern int FP_BASE;
extern int FP_ADD;
extern int WINDOW_INIT;
extern int WINDOW_MULT;
extern double HISTORY_AGE_FACTOR;
extern int RFP_DEPTH;
extern int TT_PROBE_MIN_DEPTH;
extern int NMP_BASE;
extern int NMP_DEPTH_DIV;
extern int NMP_SCORE_DIV;
extern int NMP_SCORE_MAX;
extern int TT_MOVE_BONUS;
extern int PV_MOVE_BONUS;
extern int FP_DEPTH;
extern int LMP_DEPTH_MAX;
extern int LMP_SCALE;
extern int LMR_DEPTH_MIN;
extern double LMR_BASE;
extern double LMR_DIV;
extern int LMR_HIST_MAX;
extern int LMR_HIST_DIV;
void setValueFromCommand(const std::string& command);
void printUCIOptions();
