#include <string>
#include <sstream>
#include <iostream>

extern int COUNTER_HISTORY_BONUS;
extern int FOLLOW_UP_BONUS;
extern int KILLER_MOVE_BONUS;
extern int DELTA_MARGIN;
extern int DELTA_INIT;
extern int RFP_MARGIN;
extern int FP_BASE;
extern int FP_ADD;
extern int WINDOW_INIT;
extern int WINDOW_MULT;
extern double HISTORY_AGE_FACTOR;

void setValueFromCommand(const std::string& command);
