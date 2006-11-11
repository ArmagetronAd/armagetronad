%{
#include "tString.h"
%}

%import "std_string.i"

%rename(ColoredString) tColoredString;
class tColoredString: public tString
{
public:
    static tString RemoveColors( const tString c );
};
