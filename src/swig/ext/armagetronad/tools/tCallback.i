%{
#include "tCallback.h"
%}

class tCallbackRuby : public tListItem<tCallbackRuby> {
	VALUE block;
public:
	tCallbackRuby(tCallbackRuby *& anchor);
	static void Exec(tCallbackRuby *anchor);
};
