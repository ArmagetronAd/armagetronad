%{
#include "gRotation.h"
%}

%rename(RoundEvent) gRoundEventRuby;
class gRoundEventRuby : public tCallbackRuby {
public:
	gRoundEventRuby();
	static void DoRoundEvents();
};

%rename(MatchEvent) gMatchEventRuby;
class gMatchEventRuby : public tCallbackRuby {
public:
	gMatchEventRuby();
	static void DoMatchEvents();
};
