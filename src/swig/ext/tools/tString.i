%{
#include "tString.h"
#include <string>"
%}


%rename(String) tString;
class tString
{
};

%rename(ColoredString) tColoredString;
class tColoredString : public tString
{
%rename(remove_colors) RemoveColors;
    static tString RemoveColors( const char *c ); 
};

