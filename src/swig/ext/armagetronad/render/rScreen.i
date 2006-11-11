%{
#include "rScreen.h"
%}

%import "tCallback.i"

%rename(PerFrameTask) rPerFrameTaskRuby;
class rPerFrameTaskRuby : public tCallbackRuby {
public:
	rPerFrameTaskRuby();
	static void DoPerFrameTasks();
};
