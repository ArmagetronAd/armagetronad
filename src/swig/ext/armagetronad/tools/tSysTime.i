%{
#include "tSysTime.h"
%} 

%rename("timer_accurate?") tTimerIsAccurate();
bool tTimerIsAccurate();

%rename(system_time) tSysTimeFloat();
double tSysTimeFloat();

%rename(real_system_time) tRealSysTimeFloat();
double tRealSysTimeFloat();

%rename(advance_frame) tAdvanceFrame(int usecdelay=0);
void tAdvanceFrame(int usecdelay=0); 

%rename(delay) tDelay(int);
void tDelay(int);

%rename(delay_force) tDelayForce(int);
void tDelayForce(int);
