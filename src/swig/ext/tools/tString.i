%{
#include "tString.h"
%}

/*
%typemap(out) tString {
    $result = rb_str_new2($1.c_str());
}

%typemap(in) tString {
    $1 = tString(StringValuePtr($input));
}
*/

class tString : public std::string
{
};

class tColoredString : public tString
{
    static tString RemoveColors( const char *c ); 
};